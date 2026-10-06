#include "ui_saver.h"
#include "ui_kit.h"
#include "button_input.h"
#include "net.h"
#include <esp_sleep.h>
#include <driver/gpio.h>

static uint32_t s_last = 0;

void saver_activity() { s_last = millis(); }
bool saver_due() { return millis() - s_last >= SAVER_AFTER_MS; }

namespace {

// Os dois olhos sao iguais: desenha um no sprite e envia duas vezes (sem piscar a tela).
const int EX[2] = { 92, 228 }, EY = 112;           // centro de cada olho na tela
const int RX = 60, RY = 78, IR = 30, PR = 14;       // olho (elipse), iris e pupila
const int AX = 22, AY = 38;                         // quanto a iris pode andar
const int SW = 2 * RX + 4, SH = 2 * RY + 4;         // tamanho do sprite
const uint16_t C_IRIS = 0x3DFE;

TFT_eSprite* s_spr = nullptr;

// lid: 0 = aberto ... 1 = fechado
void frame(int ox, int oy, float lid) {
    const int cx = SW / 2, cy = SH / 2;
    s_spr->fillSprite(0x0000);
    s_spr->fillEllipse(cx, cy, RX, RY, 0xFFFF);
    s_spr->fillCircle(cx + ox, cy + oy, IR, C_IRIS);
    s_spr->fillCircle(cx + ox, cy + oy, PR, 0x0000);
    s_spr->fillCircle(cx + ox - 10, cy + oy - 11, 5, 0xFFFF);
    if (lid > 0.01f) {
        int h = (int)(RY * lid);
        s_spr->fillRect(0, 0, SW, cy - RY + h, 0x0000);
        s_spr->fillRect(0, cy + RY - h, SW, SH - (cy + RY - h), 0x0000);
        if (lid > 0.99f) s_spr->fillRoundRect(10, cy - 2, SW - 20, 5, 2, 0xFFFF);
    }
    for (int i = 0; i < 2; i++) s_spr->pushSprite(EX[i] - SW / 2, EY - SH / 2);
}

void blink(int ox, int oy) {
    static const float L[] = { 0.4f, 0.8f, 1.0f, 1.0f, 0.7f, 0.35f, 0.0f };
    for (float l : L) { frame(ox, oy, l); delay(22); }
}

void sleepyClose(int ox, int oy) {
    for (int s = 1; s <= 14; s++) { frame(ox, oy, s / 14.0f); delay(55); }
    delay(350);
}

bool anyInput() {
    button_update();
    return (button_get_buttons() & 0xFF) || touch_is_pressed();
}

// A placa acorda so pelo toque na tela (o pino de interrupcao do toque, IO36). Se ele estiver
// em baixo agora (dedo na tela), nao da para dormir: o toque acordaria na hora.
bool canSleep() {
    net_off();
    pinMode(TOUCH_PIN_IRQ, INPUT);
    delay(5);
    const bool ok = digitalRead(TOUCH_PIN_IRQ) == HIGH;
    if (!ok) Serial.println("[SLEEP] pino do toque em baixo, nao dorme agora");
    return ok;
}

void goToSleep() {
    display_set_backlight(0);
    tft.fillScreen(0x0000);
    tft.writecommand(0x28);                 // display off
    tft.writecommand(0x10);                 // sleep in
    pinMode(26, OUTPUT); digitalWrite(26, LOW);   // amplificador do alto-falante quieto
    gpio_hold_en((gpio_num_t)TFT_PIN_BL);   // backlight apagado e LED desligado durante o sono
    gpio_hold_en((gpio_num_t)LED_R_PIN);
    gpio_deep_sleep_hold_en();
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_PIN_IRQ, 0);
    Serial.println("[SLEEP] deep sleep (acorda com toque)");
    Serial.flush();
    esp_deep_sleep_start();
}

}  // namespace

void saver_run(bool manual) {
    // Automatico: entra aos 5 min e dorme aos 10 (5 min de olhos). Manual (toque em "Latinha"):
    // os olhos ficam ate alguem tocar, sem dormir.
    const uint32_t limit = SLEEP_AFTER_MS - SAVER_AFTER_MS;
    uint8_t bl = 255;
    { uint8_t p, f; bool a, c; if (!touch_load_settings(&p, &f, &bl, &a, &c)) bl = 255; }
    display_set_backlight(bl > 110 ? 110 : bl);        // olhos com menos brilho
    tft.fillScreen(0x0000);

    s_spr = new TFT_eSprite(&tft);
    s_spr->setColorDepth(16);
    if (!s_spr->createSprite(SW, SH)) { delete s_spr; s_spr = nullptr; }
    Serial.printf("[SAVER] olhos (%s), sprite %s\n", manual ? "manual" : "automatico", s_spr ? "ok" : "sem memoria");

    float gx = 0, gy = 0, tx = 0, ty = 0;
    int cx = 0, cy = 0;
    uint32_t t0 = millis(), nextLook = t0 + 700, nextBlink = t0 + 2500;
    if (s_spr) frame(0, 0, 0);
    while (!anyInput()) {
        uint32_t now = millis();
        if (!manual && now - t0 >= limit) {
            if (canSleep()) {
                if (s_spr) sleepyClose(cx, cy);
                goToSleep();                           // nao volta
            }
            t0 = now;                                  // sem como acordar: continua nos olhos
        }
        if (s_spr) {
            if (now >= nextLook) {
                if (random(100) < 20) { tx = ty = 0; }
                else {
                    float a = random(0, 628) / 100.0f, m = random(40, 101) / 100.0f;
                    tx = cosf(a) * AX * m; ty = sinf(a) * AY * m;
                }
                nextLook = now + random(700, 2600);
            }
            gx += (tx - gx) * 0.4f; gy += (ty - gy) * 0.4f;
            int nx = (int)lroundf(gx), ny = (int)lroundf(gy);
            if (nx != cx || ny != cy) { cx = nx; cy = ny; frame(cx, cy, 0); }
            if (now >= nextBlink) { blink(cx, cy); nextBlink = millis() + random(2500, 6500); }
        }
        delay(20);
    }
    if (s_spr) { s_spr->deleteSprite(); delete s_spr; s_spr = nullptr; }
    display_set_backlight(bl);
    while (anyInput()) delay(15);                      // o toque que acordou nao conta como clique
    delay(80);
    tft.fillScreen(0x0000);
    saver_activity();
}
