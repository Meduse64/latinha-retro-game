// App Agenda - proximos eventos do Google Calendar, portado do projeto Meteo (Meteo_Rev12_WROOM.ino).
// Usa o "endereco secreto no formato iCal" do calendario (GOOGLE_ICS_URL em secrets.h): nao precisa de
// login nem de chave. O arquivo e lido em fluxo, linha a linha, sem guardar tudo na memoria.
// Entende eventos repetidos (FREQ=YEARLY/MONTHLY/WEEKLY/DAILY, INTERVAL, UNTIL, COUNT, BYDAY semanal);
// nao trata EXDATE (ocorrencias excluidas).
#include "app_agenda.h"
#include "ui_kit.h"
#include "net.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef GOOGLE_ICS_URL
#define GOOGLE_ICS_URL ""
#endif

namespace {

const int MAXEV = 8;
struct Evento { time_t quando; bool diaTodo; char titulo[44]; };
struct Recorrencia { char freq; int intervalo; time_t ate; int contagem; uint8_t dias; bool valida; };

Evento eventos[MAXEV];
int nEv = 0;
bool have = false, lastFailed = false;
uint32_t lastFetch = 0;
char updated[6] = "--:--";

// timegm: segundos desde 1970 para uma data em UTC
time_t timegmCasa(const struct tm* t) {
    int y = t->tm_year + 1900, m = t->tm_mon + 1, d = t->tm_mday;
    y -= m <= 2;
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    long dias = (long)era * 146097L + (long)doe - 719468L;
    return (time_t)(dias * 86400L + t->tm_hour * 3600L + t->tm_min * 60L + t->tm_sec);
}

// "\," "\;" "\n" do iCal viram virgula, ponto e virgula e espaco
void ics_unescape(char* s) {
    char* o = s;
    for (char* p = s; *p; p++) {
        if (*p == '\\' && p[1]) { p++; *o++ = (*p == 'n' || *p == 'N') ? ' ' : *p; }
        else *o++ = *p;
    }
    *o = 0;
}

void considerarEvento(time_t quando, const char* titulo, bool diaTodo) {
    time_t fim = diaTodo ? quando + 86400 : quando;       // dia inteiro: vale ate o fim do dia
    if (fim <= time(nullptr)) return;                      // ja passou
    if (nEv == MAXEV && quando >= eventos[MAXEV - 1].quando) return;
    int pos;
    if (nEv < MAXEV) { pos = nEv; nEv++; } else pos = MAXEV - 1;
    while (pos > 0 && eventos[pos - 1].quando > quando) { eventos[pos] = eventos[pos - 1]; pos--; }
    eventos[pos].quando = quando;
    eventos[pos].diaTodo = diaTodo;
    strlcpy(eventos[pos].titulo, titulo, sizeof(eventos[pos].titulo));
    ics_unescape(eventos[pos].titulo);
    ui_strip_accents(eventos[pos].titulo);
}

// DTSTART no formato ICS: AAAAMMDD, AAAAMMDDTHHMMSS ou AAAAMMDDTHHMMSSZ (UTC). Eventos com TZID
// sao tratados como hora local (boa aproximacao para uma agenda pessoal no mesmo fuso).
time_t interpretarDataICS(const String& linha, bool& diaTodoSaida) {
    int idx = linha.lastIndexOf(':');
    if (idx < 0) return 0;
    String valor = linha.substring(idx + 1);
    valor.trim();
    if (valor.length() < 8) return 0;
    bool utc = valor.endsWith("Z");
    struct tm tmv = {};
    tmv.tm_year = valor.substring(0, 4).toInt() - 1900;
    tmv.tm_mon = valor.substring(4, 6).toInt() - 1;
    tmv.tm_mday = valor.substring(6, 8).toInt();
    diaTodoSaida = !(valor.length() > 8 && valor.charAt(8) == 'T');
    if (!diaTodoSaida) {
        tmv.tm_hour = valor.substring(9, 11).toInt();
        tmv.tm_min = valor.substring(11, 13).toInt();
        tmv.tm_sec = valor.substring(13, 15).toInt();
    }
    tmv.tm_isdst = -1;
    return utc ? timegmCasa(&tmv) : mktime(&tmv);
}

Recorrencia lerRRule(const String& l) {
    Recorrencia r = { 0, 1, 0, 0, 0, false };
    int idx = l.indexOf(':');
    if (idx < 0) return r;
    String s = l.substring(idx + 1);
    s.trim();
    s += ';';
    int ini = 0;
    while (ini < (int)s.length()) {
        int fim = s.indexOf(';', ini);
        if (fim < 0) fim = s.length();
        String par = s.substring(ini, fim);
        int eq = par.indexOf('=');
        if (eq > 0) {
            String k = par.substring(0, eq), v = par.substring(eq + 1);
            if (k == "FREQ") {
                if (v == "YEARLY") r.freq = 'Y'; else if (v == "MONTHLY") r.freq = 'M';
                else if (v == "WEEKLY") r.freq = 'W'; else if (v == "DAILY") r.freq = 'D';
            } else if (k == "INTERVAL") { r.intervalo = max(1, (int)v.toInt()); }
            else if (k == "COUNT") { r.contagem = v.toInt(); }
            else if (k == "UNTIL") { bool dt; String tmp = "X:" + v; r.ate = interpretarDataICS(tmp, dt); }
            else if (k == "BYDAY") {
                static const char* nomes[7] = { "SU", "MO", "TU", "WE", "TH", "FR", "SA" };
                int p0 = 0;
                while (p0 < (int)v.length()) {
                    int p1 = v.indexOf(',', p0);
                    if (p1 < 0) p1 = v.length();
                    String tk = v.substring(p0, p1);
                    String dois = tk.length() >= 2 ? tk.substring(tk.length() - 2) : tk;
                    for (int d = 0; d < 7; d++) if (dois == nomes[d]) r.dias |= (1 << d);
                    p0 = p1 + 1;
                }
            }
        }
        ini = fim + 1;
    }
    r.valida = r.freq != 0;
    return r;
}

// Proxima ocorrencia de um evento repetitivo que ainda nao terminou (0 se acabou ou nao ha).
time_t proximaOcorrencia(time_t inicio, bool diaTodo, const Recorrencia& r) {
    time_t agora = time(nullptr);
    time_t limite = diaTodo ? agora - 86399 : agora;           // dia inteiro: ainda vale durante o dia
    struct tm base; localtime_r(&inicio, &base);
    time_t cand = inicio;
    long k = 0;

    if (r.freq == 'W' && r.dias != 0 && r.intervalo == 1) {    // semanal com dias da semana (ex: seg e qua)
        time_t ref = limite > inicio ? limite : inicio;
        struct tm s; localtime_r(&ref, &s);
        s.tm_hour = base.tm_hour; s.tm_min = base.tm_min; s.tm_sec = base.tm_sec;
        for (int d = 0; d <= 8; d++) {
            struct tm t = s; t.tm_mday += d; t.tm_isdst = -1;
            time_t c = mktime(&t);
            struct tm n; localtime_r(&c, &n);
            if (c >= inicio && c >= limite && (r.dias & (1 << n.tm_wday))) { cand = c; break; }
            cand = 0;
        }
        if (cand == 0) return 0;
        if (r.ate > 0 && cand > r.ate) return 0;
        return cand;
    }

    if (cand >= limite) return (r.ate > 0 && cand > r.ate) ? 0 : inicio;   // ainda nao aconteceu
    const long periodo = r.freq == 'D' ? 86400L * r.intervalo : (r.freq == 'W' ? 604800L * r.intervalo : 0);
    if (periodo > 0) { k = (long)((limite - inicio) / periodo) - 1; if (k < 0) k = 0; }   // salta perto de hoje
    int guarda = 0;
    do {
        k++;
        struct tm t = base;
        switch (r.freq) {
            case 'Y': t.tm_year = base.tm_year + (int)(k * r.intervalo); break;
            case 'M': t.tm_mon  = base.tm_mon  + (int)(k * r.intervalo); break;
            case 'W': t.tm_mday = base.tm_mday + (int)(7 * k * r.intervalo); break;
            case 'D': t.tm_mday = base.tm_mday + (int)(k * r.intervalo); break;
        }
        t.tm_isdst = -1;
        cand = mktime(&t);
    } while (cand < limite && ++guarda < 40000);
    if (cand < limite) return 0;
    if (r.contagem > 0 && k + 1 > r.contagem) return 0;        // a contagem ja acabou
    if (r.ate > 0 && cand > r.ate) return 0;
    return cand;
}

// Busca e le UM calendario ICS em fluxo. Devolve true so se o arquivo chegou inteiro (END:VCALENDAR).
bool buscarUmaAgenda(const char* url) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    // HTTP/1.0: o Google nao usa "chunked". Com chunked, os marcadores de tamanho dos pedacos caiam no
    // meio das linhas do calendario e corrompiam datas e titulos.
    http.useHTTP10(true);
    int codigo = http.GET();
    if (codigo != 200) { Serial.printf("[AGENDA] HTTP %d\n", codigo); http.end(); return false; }

    WiFiClient* stream = http.getStreamPtr();
    bool viuFim = false, dentroEvento = false, recorrente = false, diaTodo = false;
    Recorrencia regra = { 0, 1, 0, 0, 0, false };
    time_t dtStart = 0;
    char resumo[48] = "";

    auto processarLinha = [&](const String& l) {
        if (l.startsWith("END:VCALENDAR")) viuFim = true;
        if (l.startsWith("BEGIN:VEVENT")) {
            dentroEvento = true; recorrente = false; regra = { 0, 1, 0, 0, 0, false }; dtStart = 0; resumo[0] = 0;
        } else if (l.startsWith("END:VEVENT")) {
            if (dentroEvento && dtStart > 0 && resumo[0]) {
                if (!recorrente) considerarEvento(dtStart, resumo, diaTodo);
                else if (regra.valida) {
                    time_t prox = proximaOcorrencia(dtStart, diaTodo, regra);
                    if (prox > 0) considerarEvento(prox, resumo, diaTodo);
                }
            }
            dentroEvento = false;
        } else if (dentroEvento) {
            if (l.startsWith("RRULE")) { recorrente = true; regra = lerRRule(l); }
            else if (l.startsWith("SUMMARY")) {
                int idx = l.indexOf(':');
                if (idx > 0) strlcpy(resumo, l.c_str() + idx + 1, sizeof(resumo));
            } else if (l.startsWith("DTSTART")) {
                dtStart = interpretarDataICS(l, diaTodo);
            }
        }
    };

    String linhaLogica = "";
    size_t totalLido = 0;
    const size_t LIMITE_BYTES = 400000;
    uint32_t inicio = millis();
    while ((http.connected() || stream->available()) && totalLido < LIMITE_BYTES && millis() - inicio < 30000) {
        if (!stream->available()) { delay(5); continue; }
        String bruta = stream->readStringUntil('\n');
        totalLido += bruta.length();
        if (bruta.endsWith("\r")) bruta.remove(bruta.length() - 1);
        if (bruta.length() > 0 && (bruta[0] == ' ' || bruta[0] == '\t')) {   // linha ICS "dobrada" (continuacao)
            linhaLogica += bruta.substring(1);
            continue;
        }
        if (linhaLogica.length() > 0) processarLinha(linhaLogica);
        linhaLogica = bruta;
    }
    if (linhaLogica.length() > 0) processarLinha(linhaLogica);
    http.end();
    if (!viuFim) Serial.printf("[AGENDA] leitura incompleta (%u bytes, sem END:VCALENDAR)\n", (unsigned)totalLido);
    return viuFim;
}

bool buscarAgenda() {
    if (!GOOGLE_ICS_URL[0]) return false;
    if (!net_time_valid()) return false;          // sem a hora certa o filtro de "futuros" mostraria eventos antigos
    // Guarda a lista: se a busca falhar ou vier pela metade, ela volta.
    Evento copia[MAXEV]; memcpy(copia, eventos, sizeof(eventos));
    const int nCopia = nEv;
    nEv = 0;
    const bool ok = buscarUmaAgenda(GOOGLE_ICS_URL);
    if (!ok) { memcpy(eventos, copia, sizeof(eventos)); nEv = nCopia; }
    Serial.printf("[AGENDA] %s, eventos futuros: %d\n", ok ? "ok" : "FALHOU", nEv);
    return ok;
}

bool stale() { return !have || millis() - lastFetch > 15UL * 60UL * 1000UL; }

bool refresh() {
    ui_msg("Atualizando a agenda...", "liga o Wi-Fi");
    bool ok = false;
    if (net_connect()) {
        net_ensure_time();
        ok = buscarAgenda();
    }
    net_off();
    lastFailed = !ok;
    if (ok) {
        have = true;
        lastFetch = millis();
        struct tm t;
        if (net_local_time(&t)) snprintf(updated, sizeof(updated), "%02d:%02d", t.tm_hour, t.tm_min);
    }
    return ok;
}

// "hoje 14:30", "amanha", "Seg 12 Mai 14:30"
void quando(const Evento& e, char* buf, size_t n) {
    static const char* DIAS[]  = { "Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab" };
    static const char* MESES[] = { "Jan", "Fev", "Mar", "Abr", "Mai", "Jun", "Jul", "Ago", "Set", "Out", "Nov", "Dez" };
    time_t agora = time(nullptr);
    struct tm hoje, amanha, t;
    localtime_r(&agora, &hoje);
    time_t am = agora + 86400; localtime_r(&am, &amanha);
    localtime_r(&e.quando, &t);
    char hora[8] = "";
    if (!e.diaTodo) snprintf(hora, sizeof(hora), " %02d:%02d", t.tm_hour, t.tm_min);
    if (t.tm_year == hoje.tm_year && t.tm_yday == hoje.tm_yday) snprintf(buf, n, "hoje%s", hora);
    else if (t.tm_year == amanha.tm_year && t.tm_yday == amanha.tm_yday) snprintf(buf, n, "amanha%s", hora);
    else snprintf(buf, n, "%s %d %s%s", DIAS[t.tm_wday], t.tm_mday, MESES[t.tm_mon], hora);
}

const int ROW_H = 36, ROWS = 5, Y0 = UI_HEADER_H + 4;

void drawAll(int top) {
    ui_header("Agenda", lastFailed ? "sem rede" : updated);
    tft.fillRect(0, UI_HEADER_H + 1, SCREEN_W, SCREEN_H - UI_HEADER_H - 1 - UI_FOOTER_H, K_BG);
    if (!GOOGLE_ICS_URL[0]) {
        ui_text(SCREEN_W / 2, 100, "Falta o endereco da agenda", K_AMBER, K_BG, 4, MC_DATUM);
        ui_text(SCREEN_W / 2, 134, "Ponha GOOGLE_ICS_URL em secrets.h", K_ICE, K_BG, 2, MC_DATUM);
    } else if (!have) {
        ui_text(SCREEN_W / 2, 100, "Sem dados da agenda", K_AMBER, K_BG, 4, MC_DATUM);
        ui_text(SCREEN_W / 2, 134, lastFailed ? "Confira o Wi-Fi e o endereco" : "Aperte A para atualizar", K_ICE, K_BG, 2, MC_DATUM);
    } else if (nEv == 0) {
        ui_text(SCREEN_W / 2, 110, "Sem eventos proximos", K_GREEN, K_BG, 4, MC_DATUM);
    } else {
        char b[40];
        for (int i = 0; i < ROWS; i++) {
            int idx = top + i;
            int y = Y0 + i * ROW_H;
            uint16_t bg = (i & 1) ? K_BG : K_CARD;
            tft.fillRoundRect(4, y, 278, ROW_H - 2, 5, bg);
            if (idx >= nEv) continue;
            ui_fit_text(10, y + 10, 266, eventos[idx].titulo, K_WHITE, bg, ML_DATUM);
            quando(eventos[idx], b, sizeof(b));
            ui_text(10, y + 19, b, K_AMBER, bg, 2, TL_DATUM);
        }
        const int ax = 288, aw = 28, ah = 84;
        bool maisCima = top > 0, maisBaixo = top + ROWS < nEv;
        tft.fillRoundRect(ax, Y0, aw, ah, 5, K_CARD);
        tft.drawRoundRect(ax, Y0, aw, ah, 5, maisCima ? K_CYAN : K_BORDER);
        tft.fillTriangle(ax + aw / 2, Y0 + 30, ax + 7, Y0 + 52, ax + aw - 7, Y0 + 52, maisCima ? K_CYAN : K_BORDER);
        tft.fillRoundRect(ax, Y0 + ah + 6, aw, ah, 5, K_CARD);
        tft.drawRoundRect(ax, Y0 + ah + 6, aw, ah, 5, maisBaixo ? K_CYAN : K_BORDER);
        tft.fillTriangle(ax + aw / 2, Y0 + ah + 6 + 52, ax + 7, Y0 + ah + 6 + 30, ax + aw - 7, Y0 + ah + 6 + 30, maisBaixo ? K_CYAN : K_BORDER);
    }
    ui_footer("A atualiza - setas rolam - B volta");
}

}  // namespace

bool agenda_tile(char* big, size_t nb, char* small, size_t ns) {
    static const char* const MESES_C[] = { "Jan", "Fev", "Mar", "Abr", "Mai", "Jun", "Jul", "Ago", "Set", "Out", "Nov", "Dez" };
    if (!have) return false;
    if (nEv == 0) { snprintf(big, nb, "Livre"); snprintf(small, ns, "Sem eventos proximos"); return true; }
    struct tm t;
    localtime_r(&eventos[0].quando, &t);
    snprintf(big, nb, "%d %s", t.tm_mday, MESES_C[t.tm_mon]);       // sempre a data, mesmo se for hoje ou amanha
    if (eventos[0].diaTodo) snprintf(small, ns, "%s", eventos[0].titulo);
    else snprintf(small, ns, "%02d:%02d %s", t.tm_hour, t.tm_min, eventos[0].titulo);
    return true;
}

void app_agenda_run() {
    ui_wait_release();
    tft.fillScreen(K_BG);
    ui_header("Agenda", "...");
    if (stale() && GOOGLE_ICS_URL[0]) refresh();
    int top = 0;
    drawAll(top);

    UiIn in;
    uint32_t lastCheck = millis();
    while (true) {
        ui_poll(in);
        if ((in.pressed & (GB_BTN_B | GB_BTN_START)) || (in.tap && in.ty < UI_HEADER_H)) return;
        bool redraw = in.woke;
        int maxTop = max(0, nEv - ROWS);
        int nt = top;
        if (in.pressed & GB_BTN_UP) nt = max(0, top - 1);
        if (in.pressed & GB_BTN_DOWN) nt = min(maxTop, top + 1);
        if (in.tap && in.tx >= 288) {
            if (in.ty < Y0 + 86) nt = max(0, top - ROWS);
            else nt = min(maxTop, top + ROWS);
        }
        if (nt != top) { top = nt; redraw = true; }
        bool quer = (in.pressed & GB_BTN_A) || ui_hit(in, 0, SCREEN_H - UI_FOOTER_H, SCREEN_W, UI_FOOTER_H) ||
                    (millis() - lastCheck > 5000 && have && stale());
        if (millis() - lastCheck > 5000) lastCheck = millis();
        if (quer && GOOGLE_ICS_URL[0]) { refresh(); top = 0; redraw = true; }
        if (redraw) drawAll(top);
        delay(15);
    }
}
