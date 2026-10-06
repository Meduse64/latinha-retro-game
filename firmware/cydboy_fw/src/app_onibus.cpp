// App Onibus - portado do LatinhaColor (app_onibus.h) para a tela deitada de 320x240.
// Horarios fixos da parada Pontal (VIPBUS). Atualize aqui se a tabela mudar (e no projeto Meteo).
#include "app_onibus.h"
#include "ui_kit.h"
#include "net.h"

namespace {

struct HoraMin { uint8_t h, m; };
struct Linha {
    const char* nome; uint16_t cor;
    const HoraMin* util; int nUtil;
    const HoraMin* sab;  int nSab;
    const HoraMin* dom;  int nDom;
};

const HoraMin METRO_UTIL[] = {{5,25},{5,50},{6,20},{6,50},{7,0},{7,15},{7,30},{8,0},{8,30},{9,0},{9,15},{9,30},{10,15},{11,0},{11,30},{11,50},{12,40},{13,20},{13,40},{14,0},{14,45},{15,0},{15,30},{16,0},{16,30},{17,0},{17,30},{17,50},{18,20},{19,0},{19,30},{20,15},{21,15},{22,15}};
const HoraMin METRO_SAB[]  = {{6,0},{7,0},{8,0},{9,0},{11,0},{13,10},{15,0},{17,20},{18,40},{20,0},{21,0},{22,0}};
const HoraMin METRO_DOM[]  = {{6,0},{8,0},{10,0},{12,0},{14,0},{16,0},{18,0},{20,0},{22,0}};
const HoraMin ALLENDE_UTIL[] = {{6,25},{6,35},{11,15},{12,20},{13,15},{14,15},{15,40},{16,40},{17,40}};
const HoraMin ALLENDE_SAB[]  = {{10,0},{12,0},{13,0},{14,0},{16,0},{17,0},{18,0}};
const HoraMin ALLENDE_DOM[]  = {{8,30},{15,0}};

#define N_(a) (int)(sizeof(a) / sizeof(HoraMin))
const Linha LINHAS[2] = {
    { "Metro",     K_AMBER, METRO_UTIL,   N_(METRO_UTIL),   METRO_SAB,   N_(METRO_SAB),   METRO_DOM,   N_(METRO_DOM) },
    { "S.Allende", K_CYAN,  ALLENDE_UTIL, N_(ALLENDE_UTIL), ALLENDE_SAB, N_(ALLENDE_SAB), ALLENDE_DOM, N_(ALLENDE_DOM) },
};
#undef N_

void tabelaDoDia(const Linha& l, int wd, const HoraMin*& tab, int& n) {
    if (wd == 0)      { tab = l.dom; n = l.nDom; }
    else if (wd == 6) { tab = l.sab; n = l.nSab; }
    else              { tab = l.util; n = l.nUtil; }
}

// Ate max saidas daqui para frente (hoje e amanha). falta[] = minutos que faltam.
int proximas(const Linha& l, const struct tm& agora, int max, int* falta, HoraMin* hm) {
    int agoraMin = agora.tm_hour * 60 + agora.tm_min, k = 0;
    for (int d = 0; d < 2 && k < max; d++) {
        const HoraMin* tab; int n;
        tabelaDoDia(l, (agora.tm_wday + d) % 7, tab, n);
        for (int i = 0; i < n && k < max; i++) {
            int m = tab[i].h * 60 + tab[i].m + d * 1440;
            if (m >= agoraMin) { falta[k] = m - agoraMin; hm[k] = tab[i]; k++; }
        }
    }
    return k;
}

void faltaText(char* b, size_t n, int min) {
    if (min == 0) snprintf(b, n, "agora!");
    else if (min < 60) snprintf(b, n, "em %d min", min);
    else snprintf(b, n, "em %dh%02d", min / 60, min % 60);
}
uint16_t faltaColor(int min) { return min <= 5 ? K_RED : min <= 15 ? K_AMBER : K_GREEN; }

const int CARD_X = 8, CARD_W = 150, CARD_H = 88, CARD_Y0 = UI_HEADER_H + 6;
const int PX = 166, PW = SCREEN_W - 166 - 8;

void draw(int sel, const struct tm& t) {
    char b[32], f[20];
    for (int i = 0; i < 2; i++) {
        const Linha& l = LINHAS[i];
        int y = CARD_Y0 + i * (CARD_H + 4);
        int falta[2]; HoraMin hm[2];
        int n = proximas(l, t, 2, falta, hm);
        bool on = (i == sel);
        tft.fillRoundRect(CARD_X, y, CARD_W, CARD_H, 7, on ? K_CARDSEL : K_CARD);
        tft.drawRoundRect(CARD_X, y, CARD_W, CARD_H, 7, on ? l.cor : K_BORDER);
        if (on) tft.drawRoundRect(CARD_X + 1, y + 1, CARD_W - 2, CARD_H - 2, 6, l.cor);
        uint16_t bg = on ? K_CARDSEL : K_CARD;
        ui_text(CARD_X + 10, y + 12, l.nome, l.cor, bg, 2, ML_DATUM);
        if (!n) { ui_text(CARD_X + 10, y + 50, "sem saidas", K_RED, bg, 2, ML_DATUM); continue; }
        if (n > 1) { snprintf(b, sizeof(b), "seg %02d:%02d", hm[1].h, hm[1].m); ui_text(CARD_X + CARD_W - 8, y + 12, b, K_GRAY, bg, 2, MR_DATUM); }
        snprintf(b, sizeof(b), "%02d:%02d", hm[0].h, hm[0].m);
        ui_text(CARD_X + 10, y + 22, b, K_WHITE, bg, 6, TL_DATUM);
        faltaText(f, sizeof(f), falta[0]);
        ui_text(CARD_X + 10, y + CARD_H - 12, f, faltaColor(falta[0]), bg, 2, ML_DATUM);
    }
    // saidas da linha escolhida
    tft.fillRect(PX - 2, UI_HEADER_H + 2, PW + 4, SCREEN_H - UI_HEADER_H - 2 - UI_FOOTER_H - 2, K_BG);
    const Linha& l = LINHAS[sel];
    snprintf(b, sizeof(b), "Proximas: %s", l.nome);
    ui_text(PX, CARD_Y0 + 6, b, l.cor, K_BG, 2, ML_DATUM);
    int falta[6]; HoraMin hm[6];
    int n = proximas(l, t, 6, falta, hm);
    for (int i = 0; i < n; i++) {
        int y = CARD_Y0 + 22 + i * 26;
        uint16_t bg = (i & 1) ? K_BG : K_CARD;
        tft.fillRoundRect(PX, y, PW, 24, 4, bg);
        snprintf(b, sizeof(b), "%02d:%02d", hm[i].h, hm[i].m);
        ui_text(PX + 8, y + 12, b, i == 0 ? K_WHITE : K_ICE, bg, 2, ML_DATUM);
        faltaText(f, sizeof(f), falta[i]);
        ui_text(PX + PW - 8, y + 12, f, faltaColor(falta[i]), bg, 2, MR_DATUM);
    }
    if (!n) ui_text(PX, CARD_Y0 + 50, "sem saidas", K_RED, K_BG, 2, ML_DATUM);
}

}  // namespace

bool onibus_summary(char* out, size_t n) {
    struct tm t;
    if (!net_local_time(&t)) return false;
    int falta[1]; HoraMin hm[1];
    if (proximas(LINHAS[0], t, 1, falta, hm) < 1) return false;
    char f[20]; faltaText(f, sizeof(f), falta[0]);
    snprintf(out, n, "Metro %s", f);
    return true;
}

void app_onibus_run() {
    ui_wait_release();
    tft.fillScreen(K_BG);
    ui_header("Onibus", "Pontal");
    if (!net_time_valid()) {
        ui_msg("Acertando a hora...", "liga o Wi-Fi");
        net_ensure_time();
        net_off();
    }
    ui_header("Onibus", "Pontal");
    tft.fillRect(0, UI_HEADER_H + 1, SCREEN_W, SCREEN_H - UI_HEADER_H - 1 - UI_FOOTER_H, K_BG);
    ui_footer("toque numa linha · cima/baixo · B volta");

    int sel = 0;
    int lastMin = -1;
    bool redraw = true;
    UiIn in;
    while (true) {
        ui_poll(in);
        if ((in.pressed & (GB_BTN_B | GB_BTN_START)) || (in.tap && in.ty < UI_HEADER_H)) return;
        if (in.pressed & (GB_BTN_UP | GB_BTN_DOWN)) { sel ^= 1; redraw = true; }
        if (in.tap) {
            for (int i = 0; i < 2; i++) {
                int y = CARD_Y0 + i * (CARD_H + 4);
                if (in.tx >= CARD_X && in.tx < CARD_X + CARD_W && in.ty >= y && in.ty < y + CARD_H && sel != i) { sel = i; redraw = true; }
            }
        }
        struct tm t;
        if (!net_local_time(&t)) {
            if (redraw) {
                ui_text(SCREEN_W / 2, 110, "Sem a hora certa", K_AMBER, K_BG, 4, MC_DATUM);
                ui_text(SCREEN_W / 2, 140, "Confira o Wi-Fi em secrets.h", K_ICE, K_BG, 2, MC_DATUM);
                redraw = false;
            }
        } else {
            int min = t.tm_hour * 60 + t.tm_min;
            if (redraw || min != lastMin) { draw(sel, t); lastMin = min; redraw = false; }
        }
        delay(20);
    }
}
