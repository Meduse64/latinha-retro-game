#pragma once
#include <stdint.h>

// ─── Pins ───────────────────────────────────────────────────────────────────
#define TFT_PIN_BL     21

// Orientacao da tela. LANDSCAPE=1 (padrao deste fork): console deitado, 320x240. LANDSCAPE=0: em pe,
// 240x320, como no CYDboy original (os controles na tela so existem em pe).
#ifndef LANDSCAPE
#define LANDSCAPE 1
#endif
#ifndef HAS_BT
#define HAS_BT 0
#endif
// Game Gear e Master System (SMS Plus): so compilam se a pasta src/smsplus existir. Ela vem do
// LatinhaColor pelo scripts/copiar_smsplus.ps1 e nao vai para o repositorio (licenca).
#ifndef HAS_SMS
#if defined(__has_include)
#if __has_include("smsplus/shared.h")
#define HAS_SMS 1
#endif
#endif
#ifndef HAS_SMS
#define HAS_SMS 0
#endif
#endif
#if LANDSCAPE
#define SCREEN_W       320
#define SCREEN_H       240
#define TFT_ROTATION   1      // se a imagem sair de cabeca para baixo, troque por 3
#define TOUCH_NVS_NS   "touch_v2l"
#else
#define SCREEN_W       240
#define SCREEN_H       320
#define TFT_ROTATION   2
#define TOUCH_NVS_NS   "touch_v2"
#endif

#define TOUCH_PIN_CS    33
#define TOUCH_PIN_IRQ   36
#define TOUCH_PIN_MOSI  32
#define TOUCH_PIN_MISO  39
#define TOUCH_PIN_CLK   25

#define SD_PIN_CS 5
#define SD_PIN_MOSI 23
#define SD_PIN_MISO 19
#define SD_PIN_SCK 18

#define BUTTON_I2C_ADDR 0x20
#define BUTTON_I2C_SDA  22   // conector CN1 (o original usava 16, que no CYD e o LED RGB)
#define BUTTON_I2C_SCL  27   // conector CN1

#define LED_R_PIN 4
#define LED_G_PIN -1
#define LED_B_PIN -1

// ─── GameBoy ────────────────────────────────────────────────────────────────
#define GB_SCREEN_W 160
#define GB_SCREEN_H 144

// Game area: 160x144 scaled 1.5x to 240x216 (exact 10:9 Game Boy aspect ratio), centered on
// the screen. Deitado, sobram faixas pretas de 40 px dos lados e 12 px em cima e embaixo.
#define GAME_W 240
#define GAME_H 216
#if LANDSCAPE
#define GAME_X ((SCREEN_W - GAME_W) / 2)
#define GAME_Y ((SCREEN_H - GAME_H) / 2)
#else
#define GAME_X 0
#define GAME_Y 0
#endif
#define CTRL_Y 216
#define CTRL_H 104

// ─── Touch Zones (y=216..320 control bar) ───────────────────────────────────
// D-pad left (larger 68x68 cross with 24px arms)
#define DPAD_CX    45
#define DPAD_CY   268
#define DPAD_R     34

// A = right-upper, B = right-lower (larger radius 20)
#define BTN_A_X   214
#define BTN_A_Y   246
#define BTN_A_R    20

#define BTN_B_X   174
#define BTN_B_Y   282
#define BTN_B_R    20

// Start / Select center
#define BTN_ST_X  138
#define BTN_ST_Y  282
#define BTN_ST_W   32
#define BTN_ST_H   22

#define BTN_SE_X  102
#define BTN_SE_Y  282
#define BTN_SE_W   32
#define BTN_SE_H   22

// Menu top-right
#define BTN_M_X   226
#define BTN_M_Y    16
#define BTN_M_R    12
