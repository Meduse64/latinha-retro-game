#include "ui_home.h"
#include "ui_kit.h"
#include "net.h"
#include "app_tempo.h"
#include "app_onibus.h"
#include "app_agenda.h"

namespace {

const int TW = 148, TH = 88, TX0 = 8, TY0 = UI_HEADER_H + 6, GAPX = 8, GAPY = 6;
const char* const NAMES[4] = { "Retro", "Tempo", "Onibus", "Agenda" };
const uint16_t ACCENT[4] = { K_TEAL, K_AMBER, K_LBLUE, K_RED };

// ---- icones (48x48) feitos com formas simples ----
void iconRetro(int x, int y) {
    tft.fillRoundRect(x, y + 10, 48, 28, 10, 0x5AEB);
    tft.fillRect(x + 8, y + 21, 12, 5, K_WHITE);
    tft.fillRect(x + 11, y + 18, 5, 11, K_WHITE);
    tft.fillCircle(x + 33, y + 22, 3, K_RED);
    tft.fillCircle(x + 40, y + 27, 3, K_CYAN);
}
void iconTempo(int x, int y) {
    tft.fillCircle(x + 18, y + 17, 9, K_YELLOW);
    for (int a = 0; a < 8; a++) {
        float t = a * PI / 4;
        tft.drawLine(x + 18 + cosf(t) * 12, y + 17 + sinf(t) * 12, x + 18 + cosf(t) * 17, y + 17 + sinf(t) * 17, K_ORANGE);
    }
    tft.fillCircle(x + 22, y + 34, 7, 0xDF1C);
    tft.fillCircle(x + 32, y + 30, 9, 0xDF1C);
    tft.fillCircle(x + 41, y + 35, 6, 0xDF1C);
    tft.fillRect(x + 22, y + 34, 19, 7, 0xDF1C);
}
void iconOnibus(int x, int y) {
    tft.fillRoundRect(x + 3, y + 6, 42, 32, 6, K_AMBER);
    tft.fillRect(x + 8, y + 12, 14, 11, 0x7DFF);
    tft.fillRect(x + 26, y + 12, 14, 11, 0x7DFF);
    tft.fillRect(x + 3, y + 28, 42, 3, K_ORANGE);
    tft.fillCircle(x + 13, y + 40, 5, K_DGRAY);
    tft.fillCircle(x + 35, y + 40, 5, K_DGRAY);
}
void iconAgenda(int x, int y) {
    tft.fillRoundRect(x + 5, y + 7, 38, 36, 5, K_WHITE);
    tft.fillRoundRect(x + 5, y + 7, 38, 11, 5, K_RED);
    tft.fillRect(x + 5, y + 13, 38, 5, K_RED);
    tft.fillRect(x + 13, y + 2, 4, 9, K_GRAY);
    tft.fillRect(x + 31, y + 2, 4, 9, K_GRAY);
    for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) tft.fillRect(x + 10 + c * 8, y + 22 + r * 6, 5, 3, c == 1 && r == 1 ? K_RED : K_GRAY);
}
void icon(int i, int x, int y) {
    if (i == 0) iconRetro(x, y);
    else if (i == 1) iconTempo(x, y);
    else if (i == 2) iconOnibus(x, y);
    else iconAgenda(x, y);
}

void tilePos(int i, int* x, int* y) {
    *x = TX0 + (i % 2) * (TW + GAPX);
    *y = TY0 + (i / 2) * (TH + GAPY);
}

void drawTile(int i, bool sel, const char* sub) {
    int x, y; tilePos(i, &x, &y);
    uint16_t bg = sel ? K_CARDSEL : K_CARD;
    tft.fillRoundRect(x, y, TW, TH, 9, bg);
    tft.drawRoundRect(x, y, TW, TH, 9, sel ? ACCENT[i] : K_BORDER);
    if (sel) tft.drawRoundRect(x + 1, y + 1, TW - 2, TH - 2, 8, ACCENT[i]);
    icon(i, x + 10, y + 6);
    ui_text(x + 66, y + 30, NAMES[i], sel ? K_WHITE : K_ICE, bg, 4, ML_DATUM);
    tft.setViewport(x + 8, y + 58, TW - 16, 24, false);
    ui_text(x + 10, y + 70, sub, ACCENT[i], bg, 2, ML_DATUM);
    tft.resetViewport();
}

void subtitle(int i, char* b, size_t n) {
    b[0] = 0;
    if (i == 0) snprintf(b, n, "GB  GG  SMS");
    else if (i == 1) { if (!tempo_summary(b, n)) snprintf(b, n, "Previsao do Rio"); }
    else if (i == 2) { if (!onibus_summary(b, n)) snprintf(b, n, "Parada Pontal"); }
    else { if (!agenda_summary(b, n)) snprintf(b, n, "Google Calendar"); }
}

void drawAll(int sel) {
    char b[48];
    struct tm t;
    char clk[8] = "--:--";
    if (net_local_time(&t)) snprintf(clk, sizeof(clk), "%02d:%02d", t.tm_hour, t.tm_min);
    ui_header("Latinha", clk, false);
    tft.fillRect(0, UI_HEADER_H + 1, SCREEN_W, SCREEN_H - UI_HEADER_H - 1, K_BG);
    for (int i = 0; i < 4; i++) { subtitle(i, b, sizeof(b)); drawTile(i, i == sel, b); }
    ui_footer("SELECT ou toque aqui: calibrar toque");
}

void calibrate(int sel) {
    touch_run_calibration();
    drawAll(sel);
    ui_wait_release();
}

}  // namespace

int home_show() {
    int sel = 0;
    tft.fillScreen(K_BG);
    drawAll(sel);
    ui_wait_release();
    UiIn in;
    uint32_t lastClk = millis();
    while (true) {
        ui_poll(in);
        int ns = sel;
        if (in.pressed & (GB_BTN_LEFT | GB_BTN_RIGHT)) ns = sel ^ 1;
        if (in.pressed & (GB_BTN_UP | GB_BTN_DOWN)) ns = sel ^ 2;
        if (in.pressed & GB_BTN_SELECT) { calibrate(sel); continue; }
        if (in.tap && in.ty >= SCREEN_H - UI_FOOTER_H) { calibrate(sel); continue; }
        if (in.tap) {
            for (int i = 0; i < 4; i++) {
                int x, y; tilePos(i, &x, &y);
                if (in.tx >= x && in.tx < x + TW && in.ty >= y && in.ty < y + TH) return i;
            }
        }
        if (in.pressed & GB_BTN_A) return sel;
        if (ns != sel) { sel = ns; drawAll(sel); }
        if (millis() - lastClk > 15000) { lastClk = millis(); drawAll(sel); }   // atualiza a hora
        delay(15);
    }
}
