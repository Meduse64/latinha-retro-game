#include "display.h"
#include "hw_config.h"
#include <Arduino.h>
#include <Preferences.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
// Arduino-ESP32 3.x: a API do LEDC mudou (ledcSetup, ledcAttachPin e ledcDetachPin sairam).
// Estas funcoes recriam a API antiga em cima da nova, so para o backlight.
static bool s_bl_pwm = false;
static uint32_t s_ledc_freq[16];
static uint8_t s_ledc_res[16];
static inline void ledcSetup(uint8_t ch, uint32_t freq, uint8_t res) { s_ledc_freq[ch] = freq; s_ledc_res[ch] = res; }
static inline void ledcAttachPin(uint8_t pin, uint8_t ch) { ledcAttachChannel(pin, s_ledc_freq[ch], s_ledc_res[ch], ch); s_bl_pwm = true; }
static inline void ledcDetachPin(uint8_t pin) { if (s_bl_pwm) { ledcDetach(pin); s_bl_pwm = false; } }
#define ledcWrite(ch, v) ledcWriteChannel((ch), (v))
#endif

static void display_load_size_mode();

TFT_eSPI tft = TFT_eSPI();
static uint16_t scaled[320 * 2];

void display_init() {
    pinMode(TFT_PIN_BL, OUTPUT);
    digitalWrite(TFT_PIN_BL, HIGH);
    tft.init();
    tft.invertDisplay(true);
    // Some ST77xx displays expect swapped byte order (RGB/BGR). Enable
    // swap here to match palette byte-order when needed.
    tft.setSwapBytes(true);
    // Use rotation 2 so the USB connector is at the top in portrait mode.
    tft.setRotation(TFT_ROTATION);
    tft.fillScreen(TFT_BLACK);
    ledcSetup(0, 40000, 8);
    display_set_backlight(255);
    display_load_size_mode();
    Serial.printf("[TFT] %dx%d OK\n", tft.width(), tft.height());
}

void display_set_backlight(uint8_t level) {
    if (level == 255) {
        ledcDetachPin(TFT_PIN_BL);
        pinMode(TFT_PIN_BL, OUTPUT);
        digitalWrite(TFT_PIN_BL, HIGH);
    } else if (level == 0) {
        ledcDetachPin(TFT_PIN_BL);
        pinMode(TFT_PIN_BL, OUTPUT);
        digitalWrite(TFT_PIN_BL, LOW);
    } else {
        ledcAttachPin(TFT_PIN_BL, 0);
        ledcWrite(0, level);
    }
}
void display_clear(uint16_t color) { tft.fillScreen(color); }

// ─── Tamanho da imagem do jogo ──────────────────────────────────────────────
// 0 = Normal (1:1), 1 = Ajustado (x1,5), 2 = Cheia (x1,67, sem distorcer, so deitado),
// 3 = Esticada (320x240, distorce, so deitado). A imagem fica centralizada e o resto
// da tela fica preto: so a area do jogo e enviada ao SPI.
static uint8_t s_size_mode = 1;

const char* display_size_mode_name(uint8_t m) {
    switch (m) {
        case 0: return "Normal 1:1";
        case 2: return "Cheia x1,67";
        case 3: return "Esticada";
        default: return "Ajustado x1,5";
    }
}
uint8_t display_get_size_mode() { return s_size_mode; }
void display_set_size_mode(uint8_t m) {
    if (m >= DISPLAY_SIZE_MODES) m = 1;
    s_size_mode = m;
    Preferences p;
    if (p.begin("cyd_disp", false)) { p.putUChar("size", m); p.end(); }
}
static void display_load_size_mode() {
    Preferences p;
    if (p.begin("cyd_disp", true)) {
        uint8_t m = p.getUChar("size", 1);
        p.end();
        if (m < DISPLAY_SIZE_MODES) s_size_mode = m;
    }
#if !LANDSCAPE
    if (s_size_mode > 1) s_size_mode = 1;   // em pe so cabem o Normal e o Ajustado
#endif
}

// Uma linha de 160 pixels (Game Boy ou Game Gear) -> imagem centralizada, no tamanho escolhido
static void IRAM_ATTR push160(uint8_t y, const uint16_t* col) {
    const uint8_t mode = s_size_mode;
    int w, imgh, rows;
    switch (mode) {
        case 0:   // 1:1
            w = 160; imgh = 144; rows = 1;
            memcpy(scaled, col, GB_SCREEN_W * sizeof(uint16_t));
            break;
        case 2:   // x1,67 (5/3): 267x240
            w = 267; imgh = 240;
            for (int dx = 0; dx < w; dx++) scaled[dx] = col[(dx * 3) / 5];
            rows = ((y + 1) * 5 + 2) / 3 - (y * 5 + 2) / 3;
            break;
        case 3:   // esticada: x2 na largura, x1,67 na altura = 320x240
            w = 320; imgh = 240;
            for (int x = 0; x < GB_SCREEN_W; x++) { scaled[2 * x] = col[x]; scaled[2 * x + 1] = col[x]; }
            rows = ((y + 1) * 5 + 2) / 3 - (y * 5 + 2) / 3;
            break;
        default:  // x1,5: 240x216
            w = 240; imgh = 216;
            for (int x = 0, idx = 0; x < GB_SCREEN_W; x += 2) {
                scaled[idx++] = col[x];
                scaled[idx++] = col[x];
                scaled[idx++] = col[x + 1];
            }
            rows = (y & 1) ? 2 : 1;
            break;
    }
    if (rows == 2) memcpy(scaled + w, scaled, w * sizeof(uint16_t));

    if (y == 0) {
        tft.startWrite();
        tft.setAddrWindow((SCREEN_W - w) / 2, (SCREEN_H - imgh) / 2, w, imgh);
    }
    tft.pushPixels(scaled, w * rows);
    if (y == GB_SCREEN_H - 1) {
        tft.endWrite();
    }
}

// Game Boy: indices 0..3 + paleta
void IRAM_ATTR display_push_gb_line(uint8_t y, const uint8_t* px, const uint16_t* pal, uint8_t mask) {
    if (y >= GB_SCREEN_H) return;
    uint16_t col[GB_SCREEN_W];
    for (int x = 0; x < GB_SCREEN_W; x++) col[x] = pal[px[x] & mask];
    push160(y, col);
}

// SMS Plus entrega cada pixel em 3-3-2 (RRRGGGBB); esta tabela leva a cor para a tela
static uint16_t pal332[256];
static bool pal332_ready = false;
static void make_pal332() {
    for (int c = 0; c < 256; c++) {
        uint8_t r = ((c >> 5) & 7) * 255 / 7, g = ((c >> 2) & 7) * 255 / 7, b = (c & 3) * 85;
        pal332[c] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    pal332_ready = true;
}

// Game Gear: a janela visivel e de 160x144, igual a do Game Boy
void IRAM_ATTR display_push_gg_line(int y, const uint8_t* s) {
    if (y < 0 || y >= GB_SCREEN_H) return;
    if (!pal332_ready) make_pal332();
    uint16_t col[GB_SCREEN_W];
    for (int x = 0; x < GB_SCREEN_W; x++) col[x] = pal332[s[x]];
    push160((uint8_t)y, col);
}

// Master System: 256x192 (4:3). Deitado: 1:1 (256x192), x1,125 (288x216) ou x1,25 (320x240).
// Em pe nao cabe nem 1:1 (256 > 240): reduz para 240x180.
void IRAM_ATTR display_push_sms_line(int y, const uint8_t* s) {
    if (y < 0 || y >= 192) return;
    if (!pal332_ready) make_pal332();
    int w, imgh, r0, r1;
#if LANDSCAPE
    const uint8_t mode = s_size_mode;
    if (mode == 0) {                      // 1:1
        w = 256; imgh = 192; r0 = y; r1 = y + 1;
        for (int x = 0; x < 256; x++) scaled[x] = pal332[s[x]];
    } else if (mode == 1) {               // x1,125 (9/8)
        w = 288; imgh = 216;
        r0 = (9 * y + 7) / 8; r1 = (9 * (y + 1) + 7) / 8;
        for (int dx = 0; dx < w; dx++) scaled[dx] = pal332[s[(dx * 8) / 9]];
    } else {                              // x1,25 (5/4): 320x240, sem distorcer
        w = 320; imgh = 240;
        r0 = (5 * y + 3) / 4; r1 = (5 * (y + 1) + 3) / 4;
        for (int dx = 0; dx < w; dx++) scaled[dx] = pal332[s[(dx * 4) / 5]];
    }
#else
    w = 240; imgh = 180;                  // 15/16: some 1 linha e 1 coluna a cada 16
    if ((y & 15) == 15) return;
    r0 = y - y / 16; r1 = r0 + 1;
    for (int dx = 0; dx < w; dx++) scaled[dx] = pal332[s[(dx * 16) / 15]];
#endif
    int rows = r1 - r0;
    if (rows < 1) return;
    if (rows == 2) memcpy(scaled + w, scaled, w * sizeof(uint16_t));
    if (y == 0) {
        tft.startWrite();
        tft.setAddrWindow((SCREEN_W - w) / 2, (SCREEN_H - imgh) / 2, w, imgh);
    }
    tft.pushPixels(scaled, w * rows);
    if (y == 191) tft.endWrite();
}

// ─── Control bar (y=216..320) ───────────────────────────────────────────────
#define DARK_CTRL_BG       0x08A4
#define DARK_POD_BG        0x10E5
#define DARK_LINE_CYAN     0x07FF
#define DARK_DPAD_BODY     0x1949
#define DARK_DPAD_HI       0x3DFE
#define DARK_DPAD_SHADOW   0x0863
#define DARK_BTN_A         0x0698
#define DARK_BTN_B         0x24BD
#define DARK_BTN_HI        0x07FF
#define DARK_BTN_SHADOW    0x0863
#define DARK_RUBBER_BTN    0x1949
#define DARK_TEXT_CYAN     0x3DFE

void display_draw_controls() {
#if LANDSCAPE
    return;   // deitado nao ha controles na tela: so botoes fisicos
#endif
    tft.fillRect(0, CTRL_Y, SCREEN_W, CTRL_H, DARK_CTRL_BG);
    tft.drawFastHLine(0, CTRL_Y, SCREEN_W, DARK_LINE_CYAN);
    tft.drawFastHLine(0, CTRL_Y + 1, SCREEN_W, 0x10E5);

    // D-pad (68x68, 24px wide arms)
    int cx = DPAD_CX, cy = DPAD_CY;
    // Drop shadow
    tft.fillRoundRect(cx - 12 + 1, cy - 34 + 2, 24, 68, 4, DARK_DPAD_SHADOW);
    tft.fillRoundRect(cx - 34 + 1, cy - 12 + 2, 68, 24, 4, DARK_DPAD_SHADOW);
    // Body
    tft.fillRoundRect(cx - 12, cy - 34, 24, 68, 4, DARK_DPAD_BODY);
    tft.fillRoundRect(cx - 34, cy - 12, 68, 24, 4, DARK_DPAD_BODY);
    tft.drawRoundRect(cx - 12, cy - 34, 24, 68, 4, DARK_DPAD_HI);
    tft.drawRoundRect(cx - 34, cy - 12, 68, 24, 4, DARK_DPAD_HI);

    // Center dimple
    tft.fillCircle(cx, cy, 7, DARK_DPAD_SHADOW);
    tft.drawCircle(cx, cy, 7, DARK_LINE_CYAN);

    // Direction arrows
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(DARK_LINE_CYAN, DARK_DPAD_BODY);
    tft.drawString("^", cx, cy - 20, 2);
    tft.drawString("v", cx, cy + 20, 2);
    tft.drawString("<", cx - 20, cy, 2);
    tft.drawString(">", cx + 20, cy, 2);

    // Angled recessed pod for A/B buttons
    tft.fillRoundRect(BTN_B_X - BTN_B_R - 4, BTN_A_Y - BTN_A_R - 2, 68, 56, 12, DARK_POD_BG);
    tft.drawRoundRect(BTN_B_X - BTN_B_R - 4, BTN_A_Y - BTN_A_R - 2, 68, 56, 12, 0x21AA);

    // B button (Royal Blue)
    tft.fillCircle(BTN_B_X + 1, BTN_B_Y + 2, BTN_B_R, DARK_BTN_SHADOW);
    tft.fillCircle(BTN_B_X, BTN_B_Y, BTN_B_R, DARK_BTN_B);
    tft.drawCircle(BTN_B_X, BTN_B_Y, BTN_B_R, DARK_BTN_HI);
    tft.setTextColor(TFT_WHITE, DARK_BTN_B);
    tft.drawString("B", BTN_B_X, BTN_B_Y, 2);

    // A button (Turquoise)
    tft.fillCircle(BTN_A_X + 1, BTN_A_Y + 2, BTN_A_R, DARK_BTN_SHADOW);
    tft.fillCircle(BTN_A_X, BTN_A_Y, BTN_A_R, DARK_BTN_A);
    tft.drawCircle(BTN_A_X, BTN_A_Y, BTN_A_R, DARK_BTN_HI);
    tft.setTextColor(TFT_BLACK, DARK_BTN_A);
    tft.drawString("A", BTN_A_X, BTN_A_Y, 2);

    // Printed labels below buttons
    tft.setTextColor(DARK_TEXT_CYAN, DARK_CTRL_BG);
    tft.drawString("B", BTN_B_X + 12, BTN_B_Y + 16, 1);
    tft.drawString("A", BTN_A_X + 12, BTN_A_Y + 16, 1);

    // START & SELECT buttons
    tft.fillRoundRect(BTN_SE_X - BTN_SE_W/2, BTN_SE_Y - BTN_SE_H/2, BTN_SE_W, BTN_SE_H, 4, DARK_RUBBER_BTN);
    tft.drawRoundRect(BTN_SE_X - BTN_SE_W/2, BTN_SE_Y - BTN_SE_H/2, BTN_SE_W, BTN_SE_H, 4, DARK_DPAD_HI);
    tft.drawString("SELECT", BTN_SE_X, BTN_SE_Y + 15, 1);

    tft.fillRoundRect(BTN_ST_X - BTN_ST_W/2, BTN_ST_Y - BTN_ST_H/2, BTN_ST_W, BTN_ST_H, 4, DARK_RUBBER_BTN);
    tft.drawRoundRect(BTN_ST_X - BTN_ST_W/2, BTN_ST_Y - BTN_ST_H/2, BTN_ST_W, BTN_ST_H, 4, DARK_DPAD_HI);
    tft.drawString("START", BTN_ST_X, BTN_ST_Y + 15, 1);

    // MENU button (top-right overlay on game screen)
    tft.fillCircle(BTN_M_X, BTN_M_Y, BTN_M_R, 0x08A4);
    tft.drawCircle(BTN_M_X, BTN_M_Y, BTN_M_R, DARK_LINE_CYAN);
    tft.setTextColor(DARK_LINE_CYAN, 0x08A4);
    tft.drawString("||", BTN_M_X, BTN_M_Y, 2);
}

void display_clear_controls() {
    tft.fillRect(0, CTRL_Y, SCREEN_W, CTRL_H, TFT_BLACK);
}

