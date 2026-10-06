#include "ui_kit.h"
#include "button_input.h"
#include "ui_saver.h"

static uint8_t s_prev = 0;
static bool s_prevTouch = false;

void ui_poll(UiIn& in) {
    button_update();
    uint8_t b = (uint8_t)(button_get_buttons() & 0xFF);   // o bit do menu (toque) fica de fora
    in.b = b;
    in.pressed = b & ~s_prev;
    s_prev = b;
    bool t = touch_is_pressed();
    in.tap = t && !s_prevTouch;
    in.held = t;
    s_prevTouch = t;
    if (t) { in.tx = touch_get_x(); in.ty = touch_get_y(); } else { in.tx = in.ty = -1; }
    in.woke = false;
    if (b || t) saver_activity();
    else if (saver_due()) {
        saver_run(false);
        s_prev = 0; s_prevTouch = false;                 // o que aconteceu nos olhos nao vale para o app
        in.b = in.pressed = 0; in.tap = in.held = false; in.tx = in.ty = -1;
        in.woke = true;
    }
}

void ui_wait_release() {
    UiIn in;
    do { ui_poll(in); delay(10); } while (in.b || in.held);
    delay(60);
    ui_poll(in);
}

void ui_text(int x, int y, const char* s, uint16_t fg, uint16_t bg, uint8_t font, uint8_t datum) {
    tft.setTextDatum(datum);
    tft.setTextColor(fg, bg);
    tft.drawString(s, x, y, font);
}

void ui_fit_text(int x, int y, int maxW, const char* s, uint16_t fg, uint16_t bg, uint8_t datum) {
    uint8_t font = 2;
    if (tft.textWidth(s, font) > maxW) font = 1;
    char buf[64];
    strncpy(buf, s, sizeof(buf) - 3);
    buf[sizeof(buf) - 3] = 0;
    if (tft.textWidth(buf, font) > maxW) {
        int n = strlen(buf);
        const int dots = tft.textWidth("..", font);
        while (n > 1 && tft.textWidth(buf, font) + dots > maxW) buf[--n] = 0;
        strcat(buf, "..");
    }
    ui_text(x, y, buf, fg, bg, font, datum);
}

void ui_header(const char* title, const char* right, bool back) {
    tft.fillRect(0, 0, SCREEN_W, UI_HEADER_H, K_HEADER);
    tft.drawFastHLine(0, UI_HEADER_H, SCREEN_W, K_CYAN);
    int x = 10;
    if (back) {
        ui_text(8, UI_HEADER_H / 2, "<", K_LBLUE, K_HEADER, 4, ML_DATUM);
        x = 30;
    }
    ui_text(x, UI_HEADER_H / 2, title, K_CYAN, K_HEADER, 4, ML_DATUM);
    if (right) ui_text(SCREEN_W - 8, UI_HEADER_H / 2, right, K_ICE, K_HEADER, 2, MR_DATUM);
}

void ui_footer(const char* hint) {
    int y = SCREEN_H - UI_FOOTER_H;
    tft.fillRect(0, y, SCREEN_W, UI_FOOTER_H, K_HEADER);
    tft.drawFastHLine(0, y, SCREEN_W, K_BORDER);
    ui_text(SCREEN_W / 2, y + UI_FOOTER_H / 2 + 1, hint, K_GRAY, K_HEADER, 2, MC_DATUM);
}

void ui_msg(const char* l1, const char* l2, uint16_t color) {
    const int w = 240, h = l2 ? 70 : 50;
    const int x = (SCREEN_W - w) / 2, y = (SCREEN_H - h) / 2;
    tft.fillRoundRect(x, y, w, h, 8, K_CARD);
    tft.drawRoundRect(x, y, w, h, 8, color);
    ui_text(SCREEN_W / 2, y + (l2 ? 22 : h / 2), l1, color, K_CARD, 2, MC_DATUM);
    if (l2) ui_text(SCREEN_W / 2, y + 46, l2, K_ICE, K_CARD, 2, MC_DATUM);
}

bool ui_hit(const UiIn& in, int x, int y, int w, int h) {
    return in.tap && in.tx >= x && in.tx < x + w && in.ty >= y && in.ty < y + h;
}

// UTF-8 -> ASCII. Cobre as letras acentuadas do portugues (e outras do Latin-1); o resto vira "?".
void ui_strip_accents(char* s) {
    static const char ACC_UP[]  = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTs";   // U+00C0..U+00DF
    static const char ACC_LO[] = "aaaaaaaceeeeiiiidnooooo/ouuuuyty";   // U+00E0..U+00FF
    char* o = s;
    for (const unsigned char* p = (const unsigned char*)s; *p; ) {
        if (*p < 0x80) { *o++ = *p++; }
        else if (*p == 0xC3 && p[1] >= 0x80 && p[1] <= 0xBF) {
            *o++ = p[1] < 0xA0 ? ACC_UP[p[1] - 0x80] : ACC_LO[p[1] - 0xA0];
            p += 2;
        } else {                                   // outro caractere UTF-8: pula os bytes de continuacao
            *o++ = '?';
            p++;
            while ((*p & 0xC0) == 0x80) p++;
        }
    }
    *o = 0;
}
