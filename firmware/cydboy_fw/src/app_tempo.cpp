// App Tempo - portado do LatinhaColor (app_tempo.h) para a tela deitada de 320x240.
// Fonte dos dados: Open-Meteo (https://open-meteo.com), gratis e sem chave.
#include "app_tempo.h"
#include "ui_kit.h"
#include "net.h"
#include <ArduinoJson.h>

#define CITY_LAT "-22.91"
#define CITY_LON "-43.17"
#define CITY_TZ  "America%2FSao_Paulo"

namespace {

enum Kind { KS_SUN, KS_PARTLY, KS_CLOUD, KS_FOG, KS_RAIN, KS_SHOWER, KS_SNOW, KS_STORM };
const int NDAYS = 7, NHOURS = 24;
const char* const WDAY[7] = { "dom", "seg", "ter", "qua", "qui", "sex", "sab" };
const char* const COMPASS[8] = { "N", "NE", "L", "SE", "S", "SO", "O", "NO" };

struct Day { int code; float tmax, tmin; int rain; char rise[6], set[6]; uint8_t wday, mday, mon; float windMax, gustMax; int windDir; };
struct Hour { uint8_t hour; float temp; int rain; int code; float wind; int windDir; };
Day days[NDAYS];
Hour hours[NHOURS];
int nHours = 0;
float curTemp = 0, curWind = 0, curFeel = 0, curGust = 0;
int curHum = 0, curCode = 0, curWindDir = 0;
bool have = false, lastFailed = false;
uint32_t lastFetch = 0;
char updated[6] = "--:--";

const char* compass(int deg) { return COMPASS[((deg % 360 + 360 + 22) / 45) % 8]; }   // de onde o vento vem
uint16_t tempColor(float t) { return t < 18 ? K_LBLUE : t < 26 ? K_WHITE : t < 31 ? K_ORANGE : K_RED; }

Kind kindOf(int c) {
    if (c == 0) return KS_SUN;
    if (c <= 2) return KS_PARTLY;
    if (c == 3) return KS_CLOUD;
    if (c == 45 || c == 48) return KS_FOG;
    if (c >= 95) return KS_STORM;
    if ((c >= 71 && c <= 77) || c == 85 || c == 86) return KS_SNOW;
    if (c >= 80) return KS_SHOWER;
    return KS_RAIN;
}
const char* kindText(Kind k) {
    switch (k) {
        case KS_SUN:    return "Ceu limpo";
        case KS_PARTLY: return "Poucas nuvens";
        case KS_CLOUD:  return "Nublado";
        case KS_FOG:    return "Neblina";
        case KS_SHOWER: return "Pancadas de chuva";
        case KS_SNOW:   return "Neve";
        case KS_STORM:  return "Tempestade";
        default:        return "Chuva";
    }
}

// ---- icones; s = escala (1 = ~16 px, 3 = ~48 px) ----
const uint16_t C_CL = 0xDF1C, C_CD = 0x8C70;   // nuvem clara e escura
void cloud(int x, int y, int s, uint16_t c) {
    tft.fillCircle(x + 4 * s, y + 5 * s, 3 * s, c);
    tft.fillCircle(x + 8 * s, y + 3 * s, 4 * s, c);
    tft.fillCircle(x + 12 * s, y + 5 * s, 3 * s, c);
    tft.fillRect(x + 4 * s, y + 5 * s, 8 * s, 3 * s, c);
}
void sun(int cx, int cy, int r, int s) {
    for (int a = 0; a < 8; a++) {
        float t = a * PI / 4;
        tft.drawLine(cx + cosf(t) * (r + s), cy + sinf(t) * (r + s), cx + cosf(t) * (r + 3 * s), cy + sinf(t) * (r + 3 * s), K_ORANGE);
    }
    tft.fillCircle(cx, cy, r, K_YELLOW);
}
void drawIcon(int x, int y, Kind k, int s) {
    switch (k) {
        case KS_SUN: sun(x + 8 * s, y + 7 * s, 4 * s, s); break;
        case KS_PARTLY: sun(x + 6 * s, y + 5 * s, 3 * s, s); cloud(x + 2 * s, y + 5 * s, s, C_CL); break;
        case KS_CLOUD: cloud(x, y + 2 * s, s, C_CL); break;
        case KS_FOG:
            for (int i = 0; i < 4; i++) tft.fillRect(x + (i % 2) * 2 * s, y + 3 * s + i * 3 * s, 14 * s, s, C_CD);
            break;
        case KS_SNOW:
            cloud(x, y, s, C_CL);
            for (int i = 0; i < 3; i++) tft.fillCircle(x + 4 * s + i * 4 * s, y + 12 * s + (i % 2) * 2 * s, s, K_WHITE);
            break;
        case KS_STORM:
            cloud(x, y, s, C_CD);
            tft.fillTriangle(x + 8 * s, y + 8 * s, x + 5 * s, y + 13 * s, x + 8 * s, y + 12 * s, K_YELLOW);
            tft.fillTriangle(x + 8 * s, y + 11 * s, x + 11 * s, y + 11 * s, x + 6 * s, y + 16 * s, K_YELLOW);
            break;
        default:   // chuva e pancadas
            cloud(x, y, s, k == KS_SHOWER ? C_CD : C_CL);
            for (int i = 0; i < 3; i++) tft.drawLine(x + 5 * s + i * 3 * s, y + 10 * s, x + 4 * s + i * 3 * s, y + 13 * s, K_LBLUE);
            break;
    }
}

// seta para onde o vento vai (a direcao meteorologica diz de onde ele vem)
void windArrow(int cx, int cy, int deg, int len, uint16_t c) {
    float a = (deg + 180) * PI / 180.0f;
    float dx = sinf(a), dy = -cosf(a);
    int hx = cx + dx * len / 2, hy = cy + dy * len / 2, tx = cx - dx * len / 2, ty = cy - dy * len / 2;
    tft.drawLine(tx, ty, hx, hy, c);
    float l = 3.5f;
    tft.fillTriangle(hx, hy, hx - dx * l * 1.4f + dy * l, hy - dy * l * 1.4f - dx * l,
                     hx - dx * l * 1.4f - dy * l, hy - dy * l * 1.4f + dx * l, c);
}

void degree(int x, int y, uint16_t c) { tft.drawCircle(x, y, 2, c); }   // o simbolo de grau nao existe na fonte

// ---- busca ----
bool fetchOnce() {
    String url = String("https://api.open-meteo.com/v1/forecast?latitude=") + CITY_LAT + "&longitude=" + CITY_LON +
                 "&current=temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,"
                 "wind_speed_10m,wind_direction_10m,wind_gusts_10m"
                 "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset,"
                 "wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant"
                 "&hourly=temperature_2m,precipitation_probability,weather_code,wind_speed_10m,wind_direction_10m"
                 "&forecast_hours=24"
                 "&timezone=" CITY_TZ "&forecast_days=7";
    String body;
    if (!net_http_get(url.c_str(), body)) return false;

    JsonDocument doc;
    if (deserializeJson(doc, body)) return false;
    JsonObject cur = doc["current"];
    curTemp = cur["temperature_2m"] | 0.0f;
    curFeel = cur["apparent_temperature"] | curTemp;
    curHum = cur["relative_humidity_2m"] | 0;
    curCode = cur["weather_code"] | 0;
    curWind = cur["wind_speed_10m"] | 0.0f;
    curGust = cur["wind_gusts_10m"] | curWind;
    curWindDir = cur["wind_direction_10m"] | 0;
    JsonObject d = doc["daily"];
    for (int i = 0; i < NDAYS; i++) {
        const char* dt = d["time"][i] | "2000-01-01";
        int y = 2000, mo = 1, md = 1;
        sscanf(dt, "%d-%d-%d", &y, &mo, &md);
        struct tm tt = {};
        tt.tm_year = y - 1900; tt.tm_mon = mo - 1; tt.tm_mday = md; tt.tm_hour = 12;
        mktime(&tt);
        days[i].wday = tt.tm_wday; days[i].mday = md; days[i].mon = constrain(mo - 1, 0, 11);
        days[i].code = d["weather_code"][i] | 0;
        days[i].tmax = d["temperature_2m_max"][i] | 0.0f;
        days[i].tmin = d["temperature_2m_min"][i] | 0.0f;
        days[i].rain = d["precipitation_probability_max"][i] | 0;
        days[i].windMax = d["wind_speed_10m_max"][i] | 0.0f;
        days[i].gustMax = d["wind_gusts_10m_max"][i] | days[i].windMax;
        days[i].windDir = d["wind_direction_10m_dominant"][i] | 0;
        const char* sr = d["sunrise"][i] | "0000-00-00T--:--";
        const char* ss = d["sunset"][i] | "0000-00-00T--:--";
        strlcpy(days[i].rise, strlen(sr) >= 16 ? sr + 11 : "--:--", 6);
        strlcpy(days[i].set, strlen(ss) >= 16 ? ss + 11 : "--:--", 6);
    }
    JsonObject h = doc["hourly"];
    nHours = 0;
    for (int i = 0; i < NHOURS; i++) {
        const char* ht = h["time"][i] | "";
        if (strlen(ht) < 13) break;
        hours[i].hour = atoi(ht + 11);
        hours[i].temp = h["temperature_2m"][i] | 0.0f;
        hours[i].rain = h["precipitation_probability"][i] | 0;
        hours[i].code = h["weather_code"][i] | 0;
        hours[i].wind = h["wind_speed_10m"][i] | 0.0f;
        hours[i].windDir = h["wind_direction_10m"][i] | 0;
        nHours++;
    }
    have = true;
    lastFetch = millis();
    struct tm t;
    if (net_local_time(&t)) snprintf(updated, sizeof(updated), "%02d:%02d", t.tm_hour, t.tm_min);
    return true;
}

bool stale() { return !have || millis() - lastFetch > 30UL * 60UL * 1000UL; }

bool refresh() {
    ui_msg("Atualizando o tempo...", "liga o Wi-Fi");
    bool ok = false;
    if (net_connect()) {
        net_ensure_time();
        ok = fetchOnce();
    }
    net_off();
    lastFailed = !ok;
    return ok;
}

// ---- telas ----
const int Y_TABS = UI_HEADER_H + 3, H_TABS = 24;
const int Y0 = Y_TABS + H_TABS + 5;              // o conteudo comeca abaixo das abas
const int Y1 = SCREEN_H - UI_FOOTER_H - 2;
const char* const TABS[3] = { "AGORA", "HORAS", "7 DIAS" };

void drawTabs(int tab) {
    for (int i = 0; i < 3; i++) {
        int x = 4 + i * 105;
        bool on = (i == tab);
        tft.fillRoundRect(x, Y_TABS, 102, H_TABS, 5, on ? K_CARDSEL : K_CARD);
        tft.drawRoundRect(x, Y_TABS, 102, H_TABS, 5, on ? K_CYAN : K_BORDER);
        ui_text(x + 51, Y_TABS + H_TABS / 2, TABS[i], on ? K_CYAN : K_ICE, on ? K_CARDSEL : K_CARD, 2, MC_DATUM);
    }
}

void card(int x, int y, int w, int h, const char* label, const char* value, uint16_t vc = K_WHITE) {
    tft.fillRoundRect(x, y, w, h, 5, K_CARD);
    ui_text(x + 6, y + h / 2, label, K_GRAY, K_CARD, 2, ML_DATUM);
    ui_text(x + w - 6, y + h / 2, value, vc, K_CARD, 2, MR_DATUM);
}

void drawNow() {
    Kind k = kindOf(curCode);
    drawIcon(14, Y0 + 6, k, 3);
    char b[32];
    snprintf(b, sizeof(b), "%d", (int)roundf(curTemp));
    ui_text(76, Y0 + 2, b, tempColor(curTemp), K_BG, 6, TL_DATUM);
    int w = tft.textWidth(b, 6);
    degree(76 + w + 6, Y0 + 8, tempColor(curTemp));
    ui_text(76 + w + 12, Y0 + 6, "C", tempColor(curTemp), K_BG, 4, TL_DATUM);
    ui_text(14, Y0 + 66, kindText(k), K_WHITE, K_BG, 2, TL_DATUM);
    snprintf(b, sizeof(b), "sensacao %d C", (int)roundf(curFeel));
    ui_text(14, Y0 + 88, b, K_GRAY, K_BG, 2, TL_DATUM);
    snprintf(b, sizeof(b), "atualizado %s", updated);
    ui_text(14, Y0 + 112, b, K_DGRAY + 0x2104, K_BG, 2, TL_DATUM);

    const int x = 168, cw = 146, ch = 28;
    int y = Y0;
    snprintf(b, sizeof(b), "%d%%", curHum);
    card(x, y, cw, ch, "Umidade", b); y += ch + 3;
    snprintf(b, sizeof(b), "%d %s", (int)roundf(curWind), compass(curWindDir));
    card(x, y, cw, ch, "Vento km/h", b, curWind < 15 ? K_GREEN : curWind < 30 ? K_AMBER : curWind < 50 ? K_ORANGE : K_RED);
    windArrow(x + 86, y + ch / 2, curWindDir, 14, K_ICE); y += ch + 3;
    snprintf(b, sizeof(b), "%d", (int)roundf(curGust));
    card(x, y, cw, ch, "Rajada km/h", b); y += ch + 3;
    snprintf(b, sizeof(b), "%d%%", days[0].rain);
    card(x, y, cw, ch, "Chuva hoje", b, days[0].rain >= 50 ? K_LBLUE : K_WHITE); y += ch + 3;
    snprintf(b, sizeof(b), "%s  %s", days[0].rise, days[0].set);
    card(x, y, cw, ch, "Sol", b);
}

const int ROW_H = 26, ROWS_H = 6;
void drawHours(int top) {
    char b[32];
    for (int i = 0; i < ROWS_H; i++) {
        int idx = top + i;
        int y = Y0 + i * ROW_H;
        tft.fillRoundRect(4, y, 278, ROW_H - 2, 4, (i & 1) ? K_BG : K_CARD);
        if (idx >= nHours) continue;
        const Hour& h = hours[idx];
        snprintf(b, sizeof(b), "%02dh", h.hour);
        ui_text(10, y + (ROW_H - 2) / 2, b, K_ICE, (i & 1) ? K_BG : K_CARD, 2, ML_DATUM);
        drawIcon(52, y + 4, kindOf(h.code), 1);
        snprintf(b, sizeof(b), "%d", (int)roundf(h.temp));
        ui_text(92, y + (ROW_H - 2) / 2, b, tempColor(h.temp), (i & 1) ? K_BG : K_CARD, 2, ML_DATUM);
        degree(92 + tft.textWidth(b, 2) + 5, y + 8, tempColor(h.temp));
        snprintf(b, sizeof(b), "%d%%", h.rain);
        ui_text(150, y + (ROW_H - 2) / 2, b, h.rain >= 50 ? K_LBLUE : K_GRAY, (i & 1) ? K_BG : K_CARD, 2, ML_DATUM);
        snprintf(b, sizeof(b), "%d %s", (int)roundf(h.wind), compass(h.windDir));
        ui_text(206, y + (ROW_H - 2) / 2, b, h.wind < 15 ? K_GREEN : h.wind < 30 ? K_AMBER : K_ORANGE, (i & 1) ? K_BG : K_CARD, 2, ML_DATUM);
    }
    // setas de rolagem
    const int ax = 288, aw = 28, ah = 74;
    tft.fillRoundRect(ax, Y0, aw, ah, 5, K_CARD);
    tft.drawRoundRect(ax, Y0, aw, ah, 5, top > 0 ? K_CYAN : K_BORDER);
    tft.fillTriangle(ax + aw / 2, Y0 + 26, ax + 7, Y0 + 46, ax + aw - 7, Y0 + 46, top > 0 ? K_CYAN : K_BORDER);
    tft.fillRoundRect(ax, Y0 + ah + 6, aw, ah, 5, K_CARD);
    bool more = top + ROWS_H < nHours;
    tft.drawRoundRect(ax, Y0 + ah + 6, aw, ah, 5, more ? K_CYAN : K_BORDER);
    tft.fillTriangle(ax + aw / 2, Y0 + ah + 6 + 46, ax + 7, Y0 + ah + 6 + 26, ax + aw - 7, Y0 + ah + 6 + 26, more ? K_CYAN : K_BORDER);
}

void drawDays() {
    char b[32];
    const int rh = 22;
    for (int i = 0; i < NDAYS; i++) {
        int y = Y0 + i * rh;
        uint16_t bg = (i & 1) ? K_BG : K_CARD;
        tft.fillRoundRect(4, y, 312, rh - 2, 4, bg);
        const Day& d = days[i];
        snprintf(b, sizeof(b), "%s %d", i == 0 ? "hoje" : WDAY[d.wday], d.mday);
        ui_text(10, y + (rh - 2) / 2, b, K_ICE, bg, 2, ML_DATUM);
        drawIcon(94, y + 2, kindOf(d.code), 1);
        snprintf(b, sizeof(b), "%d", (int)roundf(d.tmax));
        ui_text(128, y + (rh - 2) / 2, b, tempColor(d.tmax), bg, 2, ML_DATUM);
        snprintf(b, sizeof(b), "%d", (int)roundf(d.tmin));
        ui_text(166, y + (rh - 2) / 2, b, K_GRAY, bg, 2, ML_DATUM);
        snprintf(b, sizeof(b), "%d%%", d.rain);
        ui_text(206, y + (rh - 2) / 2, b, d.rain >= 50 ? K_LBLUE : K_GRAY, bg, 2, ML_DATUM);
        snprintf(b, sizeof(b), "%d %s", (int)roundf(d.windMax), compass(d.windDir));
        ui_text(252, y + (rh - 2) / 2, b, d.windMax < 15 ? K_GREEN : d.windMax < 30 ? K_AMBER : K_ORANGE, bg, 2, ML_DATUM);
    }
}

void drawAll(int tab, int top) {
    ui_header("Tempo", lastFailed ? "sem rede" : updated);
    tft.fillRect(0, UI_HEADER_H + 1, SCREEN_W, SCREEN_H - UI_HEADER_H - 1 - UI_FOOTER_H, K_BG);
    drawTabs(tab);
    if (!have) {
        ui_text(SCREEN_W / 2, Y0 + 50, "Sem dados do tempo", K_AMBER, K_BG, 4, MC_DATUM);
        ui_text(SCREEN_W / 2, Y0 + 84, lastFailed ? "Confira o Wi-Fi em secrets.h" : "Aperte A para atualizar", K_ICE, K_BG, 2, MC_DATUM);
    } else if (tab == 0) drawNow();
    else if (tab == 1) drawHours(top);
    else drawDays();
    ui_footer(tab == 1 ? "abas - setas rolam - A atualiza - B volta" : "toque nas abas - A atualiza - B volta");
}

}  // namespace

bool tempo_tile(char* big, size_t nb, char* small, size_t ns, uint16_t* color) {
    if (!have) return false;
    snprintf(big, nb, "%d", (int)roundf(curTemp));
    snprintf(small, ns, "%s", kindText(kindOf(curCode)));
    *color = tempColor(curTemp);
    return true;
}

void tempo_icon(int x, int y) { drawIcon(x, y, have ? kindOf(curCode) : KS_PARTLY, 3); }

void app_tempo_run() {
    ui_wait_release();
    int tab = 0, top = 0;
    tft.fillScreen(K_BG);
    ui_header("Tempo", "...");
    if (stale()) refresh();
    drawAll(tab, top);

    UiIn in;
    uint32_t lastCheck = millis();
    while (true) {
        ui_poll(in);
        if ((in.pressed & (GB_BTN_B | GB_BTN_START)) || (in.tap && in.ty < UI_HEADER_H)) return;

        int newTab = tab;
        if (in.tap && in.ty >= Y_TABS && in.ty < Y_TABS + H_TABS) {
            int t = (in.tx - 4) / 105;
            if (t >= 0 && t < 3) newTab = t;
        }
        if (in.pressed & GB_BTN_RIGHT) newTab = (tab + 1) % 3;
        if (in.pressed & GB_BTN_LEFT) newTab = (tab + 2) % 3;
        bool redraw = in.woke;
        if (newTab != tab) { tab = newTab; top = 0; redraw = true; }

        if (tab == 1 && have) {
            int maxTop = max(0, nHours - ROWS_H);
            int nt = top;
            if (in.pressed & GB_BTN_UP) nt = max(0, top - 1);
            if (in.pressed & GB_BTN_DOWN) nt = min(maxTop, top + 1);
            if (in.tap && in.tx >= 288) {
                if (in.ty < Y0 + 74) nt = max(0, top - ROWS_H);
                else if (in.ty >= Y0 + 80) nt = min(maxTop, top + ROWS_H);
            }
            if (nt != top) { top = nt; redraw = true; }
        }

        bool wantRefresh = (in.pressed & GB_BTN_A) || ui_hit(in, 0, SCREEN_H - UI_FOOTER_H, SCREEN_W, UI_FOOTER_H) ||
                           (millis() - lastCheck > 5000 && have && stale());
        if (millis() - lastCheck > 5000) lastCheck = millis();
        if (wantRefresh) { refresh(); redraw = true; }

        if (redraw) drawAll(tab, top);
        delay(15);
    }
}
