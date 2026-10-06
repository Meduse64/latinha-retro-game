#include "ui_home.h"
#include "ui_kit.h"
#include "ui_saver.h"
#include "net.h"
#include "app_tempo.h"
#include "app_onibus.h"
#include "app_agenda.h"
#include "touch_input.h"

namespace {

const int TW = 148, TH = 88, TX0 = 8, TY0 = UI_HEADER_H + 6, GAPX = 8, GAPY = 6;
const char* const NAMES[4] = { "Retro", "Tempo", "Onibus", "Agenda" };
const uint16_t ACCENT[4] = { K_TEAL, K_AMBER, K_LBLUE, K_RED };

// Cada bloco: icone a esquerda, texto grande ao lado e uma linha de detalhe embaixo.
// A linha de baixo desfila quando nao cabe (pausa, anda 40 px/s, repete com uma folga).
const int ICON_X = 10, ICON_Y = 6;
const int BIG_X = 64, BIG_W = 78, BIG_Y = 30;
const int SUB_X = 10, SUB_Y = 70, SUB_W = TW - 20, SUB_H = 18;
const int SCROLL_GAP = 40, SCROLL_SPEED = 40;
const uint32_t SCROLL_PAUSE = 1500;

struct Info {
    char big[24];        // texto grande
    char small[80];      // linha de baixo
    uint16_t bigColor;
    bool degree;         // temperatura: desenha o grau e o C depois do numero
};
Info s_info[4];
int s_subW[4];
bool s_scroll[4];
uint32_t s_t0[4];
int s_lastOff[4];
int s_sel = 0;
TFT_eSprite* s_spr = nullptr;

// ---- icones (48x48) feitos com formas simples ----
void iconRetro(int x, int y) {
    tft.fillRoundRect(x, y + 10, 48, 28, 10, 0x5AEB);
    tft.fillRect(x + 8, y + 21, 12, 5, K_WHITE);
    tft.fillRect(x + 11, y + 18, 5, 11, K_WHITE);
    tft.fillCircle(x + 33, y + 22, 3, K_RED);
    tft.fillCircle(x + 40, y + 27, 3, K_CYAN);
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
    else if (i == 1) tempo_icon(x, y);        // o icone muda com o tempo de agora
    else if (i == 2) iconOnibus(x, y);
    else iconAgenda(x, y);
}

void tilePos(int i, int* x, int* y) {
    *x = TX0 + (i % 2) * (TW + GAPX);
    *y = TY0 + (i / 2) * (TH + GAPY);
}

void getInfo(int i, Info& f) {
    f.degree = false;
    f.bigColor = K_ICE;
    f.small[0] = 0;
    snprintf(f.big, sizeof(f.big), "%s", NAMES[i]);          // sem dados: fica o nome do app
    uint16_t c = K_ICE;
    if (i == 0) {
        snprintf(f.small, sizeof(f.small), "GB  GG  SMS");
    } else if (i == 1) {
        if (tempo_tile(f.big, sizeof(f.big), f.small, sizeof(f.small), &c)) { f.bigColor = c; f.degree = true; }
        else snprintf(f.small, sizeof(f.small), "Previsao do Rio");
    } else if (i == 2) {
        if (onibus_tile(f.big, sizeof(f.big), f.small, sizeof(f.small), &c)) f.bigColor = c;
        else snprintf(f.small, sizeof(f.small), "Parada Pontal");
    } else {
        if (!agenda_tile(f.big, sizeof(f.big), f.small, sizeof(f.small))) snprintf(f.small, sizeof(f.small), "Google Calendar");
    }
}

void drawBig(int x, int y, const Info& f, bool sel, uint16_t bg) {
    const int bx = x + BIG_X, by = y + BIG_Y;
    const uint16_t col = (f.bigColor == K_ICE && sel) ? K_WHITE : f.bigColor;
    if (f.degree) {                                          // "24" + grau + "C" (a fonte nao tem o simbolo de grau)
        int w = tft.textWidth(f.big, 4);
        ui_text(bx, by, f.big, col, bg, 4, ML_DATUM);
        tft.drawCircle(bx + w + 4, by - 10, 3, col);
        ui_text(bx + w + 10, by, "C", col, bg, 4, ML_DATUM);
        return;
    }
    const uint8_t font = tft.textWidth(f.big, 4) <= BIG_W ? 4 : 2;   // se nao couber, fonte menor
    ui_text(bx, by, f.big, col, bg, font, ML_DATUM);
}

// off so conta quando o texto desfila
void drawSub(int i, int off) {
    int x, y; tilePos(i, &x, &y);
    uint16_t bg = (i == s_sel) ? K_CARDSEL : K_CARD;
    if (!s_scroll[i]) {
        ui_fit_text(x + SUB_X, y + SUB_Y, SUB_W, s_info[i].small, ACCENT[i], bg, ML_DATUM);
        return;
    }
    s_spr->fillSprite(bg);
    s_spr->setTextColor(ACCENT[i], bg);
    s_spr->setTextDatum(ML_DATUM);
    s_spr->drawString(s_info[i].small, -off, SUB_H / 2, 2);
    s_spr->drawString(s_info[i].small, -off + s_subW[i] + SCROLL_GAP, SUB_H / 2, 2);
    s_spr->pushSprite(x + SUB_X, y + SUB_Y - SUB_H / 2);
    s_lastOff[i] = off;
}

void stepScroll(uint32_t now) {
    for (int i = 0; i < 4; i++) {
        if (!s_scroll[i]) continue;
        const uint32_t cycle = s_subW[i] + SCROLL_GAP;
        const uint32_t travel = cycle * 1000UL / SCROLL_SPEED;
        const uint32_t ph = (now - s_t0[i]) % (SCROLL_PAUSE + travel);
        const int off = ph < SCROLL_PAUSE ? 0 : (int)((ph - SCROLL_PAUSE) * SCROLL_SPEED / 1000);
        if (off != s_lastOff[i]) drawSub(i, off);
    }
}

void drawTile(int i, bool sel) {
    int x, y; tilePos(i, &x, &y);
    uint16_t bg = sel ? K_CARDSEL : K_CARD;
    tft.fillRoundRect(x, y, TW, TH, 9, bg);
    tft.drawRoundRect(x, y, TW, TH, 9, sel ? ACCENT[i] : K_BORDER);
    if (sel) tft.drawRoundRect(x + 1, y + 1, TW - 2, TH - 2, 8, ACCENT[i]);
    icon(i, x + ICON_X, y + ICON_Y);
    drawBig(x, y, s_info[i], sel, bg);
    drawSub(i, 0);
}

// Prepara o texto de baixo (largura, se desfila) e recomeca o desfile
void setupSub(int i) {
    s_subW[i] = tft.textWidth(s_info[i].small, 2);
    s_scroll[i] = s_spr && s_subW[i] > SUB_W;
    s_t0[i] = millis();
    s_lastOff[i] = -1;
}

void drawAll(int sel) {
    struct tm t;
    char clk[8] = "--:--";
    if (net_local_time(&t)) snprintf(clk, sizeof(clk), "%02d:%02d", t.tm_hour, t.tm_min);
    ui_header("Latinha", clk, false);
    tft.fillRect(0, UI_HEADER_H + 1, SCREEN_W, SCREEN_H - UI_HEADER_H - 1, K_BG);
    s_sel = sel;
    for (int i = 0; i < 4; i++) {
        getInfo(i, s_info[i]);
        setupSub(i);
        drawTile(i, i == sel);
    }
    ui_footer("SELECT ou toque aqui: calibrar toque");
}

// Atualiza a hora e so os blocos cujo texto mudou (o texto que desfila nao recomeca a toa)
void refreshInfo(int sel) {
    struct tm t;
    char clk[8] = "--:--";
    if (net_local_time(&t)) snprintf(clk, sizeof(clk), "%02d:%02d", t.tm_hour, t.tm_min);
    ui_header("Latinha", clk, false);
    for (int i = 0; i < 4; i++) {
        Info f;
        getInfo(i, f);
        if (strcmp(f.big, s_info[i].big) == 0 && strcmp(f.small, s_info[i].small) == 0) continue;
        s_info[i] = f;
        setupSub(i);
        drawTile(i, i == sel);
    }
}

void calibrate(int sel) {
    touch_run_calibration();
    drawAll(sel);
    ui_wait_release();
}

int run() {
    int sel = 0;
    tft.fillScreen(K_BG);
    drawAll(sel);
    ui_wait_release();
    UiIn in;
    uint32_t lastClk = millis(), lastScroll = 0;
    while (true) {
        ui_poll(in);
        if (in.woke) { drawAll(sel); continue; }
        int ns = sel;
        if (in.pressed & (GB_BTN_LEFT | GB_BTN_RIGHT)) ns = sel ^ 1;
        if (in.pressed & (GB_BTN_UP | GB_BTN_DOWN)) ns = sel ^ 2;
        if (in.pressed & GB_BTN_SELECT) { calibrate(sel); continue; }
        if (in.tap && in.ty >= SCREEN_H - UI_FOOTER_H) { calibrate(sel); continue; }
        if (in.tap && in.ty < UI_HEADER_H && in.tx < 170) {      // toque em "Latinha": olhos na hora
            saver_run(true);
            drawAll(sel);
            continue;
        }
        if (in.tap) {
            for (int i = 0; i < 4; i++) {
                int x, y; tilePos(i, &x, &y);
                if (in.tx >= x && in.tx < x + TW && in.ty >= y && in.ty < y + TH) return i;
            }
        }
        if (in.pressed & GB_BTN_A) return sel;
        if (ns != sel) { sel = ns; drawAll(sel); }
        if (millis() - lastClk > 15000) { lastClk = millis(); refreshInfo(sel); }   // hora e resumos
        if (millis() - lastScroll > 30) { lastScroll = millis(); stepScroll(lastScroll); }
        delay(10);
    }
}

}  // namespace

int home_show() {
    s_spr = new TFT_eSprite(&tft);
    s_spr->setColorDepth(16);
    if (!s_spr->createSprite(SUB_W, SUB_H)) { delete s_spr; s_spr = nullptr; }   // sem memoria: so corta o texto
    int r = run();
    if (s_spr) { s_spr->deleteSprite(); delete s_spr; s_spr = nullptr; }
    return r;
}
