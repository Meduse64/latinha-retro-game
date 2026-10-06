#include <Arduino.h>
#include "button_input.h"
#include "touch_input.h"
#include "bt_controller.h"
#include "hw_config.h"
#include <Wire.h>

static volatile uint16_t cur_btns = 0;
static bool pcf_detected = false;

// Diz o que esta ligado a um fio (solto, preso no GND ou com pull-up), para achar erro de ligacao.
static const char* line_state(int pin) {
    pinMode(pin, INPUT_PULLDOWN); delay(3); int d = digitalRead(pin);
    pinMode(pin, INPUT_PULLUP);   delay(3); int u = digitalRead(pin);
    pinMode(pin, INPUT);
    if (d == 0 && u == 1) return "solto";
    if (d == 0 && u == 0) return "GND";
    if (d == 1 && u == 1) return "alto";
    return "?";
}

void button_init() {
    Serial.printf("[INPUT] I2C: SDA(IO%d)=%s SCL(IO%d)=%s\n", BUTTON_I2C_SDA, line_state(BUTTON_I2C_SDA),
                  BUTTON_I2C_SCL, line_state(BUTTON_I2C_SCL));
    Wire.begin(BUTTON_I2C_SDA, BUTTON_I2C_SCL);
    Wire.setClock(100000);
    for (int tentativa = 0; tentativa < 3 && !pcf_detected; tentativa++) {
        if (tentativa) delay(30);
        Wire.beginTransmission((uint8_t)BUTTON_I2C_ADDR);
        pcf_detected = (Wire.endTransmission() == 0);
    }
    if (!pcf_detected) {
        Serial.print("[INPUT] Varredura I2C:");
        int n = 0;
        for (uint8_t a = 0x08; a < 0x78; a++) {
            Wire.beginTransmission(a);
            if (Wire.endTransmission() == 0) { Serial.printf(" 0x%02X", a); n++; }
        }
        Serial.println(n ? "" : " nenhum");
    }
    if (pcf_detected) {
        Serial.println("[INPUT] PCF8574 I2C button board detected at 0x20");
    } else {
        Serial.println("[INPUT] No I2C button board detected; touch active");
    }
}

static uint16_t read_pcf_buttons() {
    Wire.requestFrom((uint8_t)BUTTON_I2C_ADDR, (uint8_t)1);
    if (Wire.available() < 1) return 0;
    uint8_t raw = Wire.read();
    raw = ~raw;  // PCF8574 inputs are pulled high; pressed is low.

    uint16_t buttons = 0;
    if (raw & (1 << 0)) buttons |= GB_BTN_UP;
    if (raw & (1 << 1)) buttons |= GB_BTN_DOWN;
    if (raw & (1 << 2)) buttons |= GB_BTN_LEFT;
    if (raw & (1 << 3)) buttons |= GB_BTN_RIGHT;
    if (raw & (1 << 4)) buttons |= GB_BTN_A;
    if (raw & (1 << 5)) buttons |= GB_BTN_B;
    if (raw & (1 << 6)) buttons |= GB_BTN_START;
    if (raw & (1 << 7)) buttons |= GB_BTN_SELECT;
    return buttons;
}

void button_update() {
    touch_update();
    bt_controller_update();
    uint16_t bt_btns = bt_controller_get_buttons();
    uint16_t touch_btns = touch_get_buttons();
    if (pcf_detected || LANDSCAPE) {
        // Com botoes fisicos (ou deitado, onde nao ha controles na tela), as zonas de toque dos
        // controles nao valem: um toque em qualquer lugar da tela abre o menu de pausa.
        touch_btns = touch_is_pressed() ? GB_BTN_MENU : 0;
    } else if (bt_controller_is_connected()) {
        touch_btns &= GB_BTN_MENU;
    }
    uint16_t btns = touch_btns | bt_btns;
    if (pcf_detected) {
        btns |= read_pcf_buttons();
    }
    cur_btns = btns;
}

uint16_t button_get_buttons() {
    return cur_btns;
}

bool button_pcf_detected() {
    return pcf_detected;
}

