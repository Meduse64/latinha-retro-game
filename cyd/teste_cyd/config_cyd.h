#pragma once
// ============================================================
//  Configuracao do teste de hardware - CYD (ESP32-2432S028R)
//  Os pinos da tela, do cartao SD e do alto-falante sao fixos da placa.
//  Mude so as opcoes de "Entradas" conforme o que voce ligou.
// ============================================================

// ---------- Tela ILI9341 240 x 320 (fixa na placa) ----------
#define TFT_SCK   14
#define TFT_MISO  12
#define TFT_MOSI  13
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   -1     // o reset da tela e ligado ao reset da placa
#define TFT_BL    21     // luz de fundo
#define TFT_HZ    40000000UL   // se aparecer ruido na imagem, baixe para 27000000UL
#define TFT_INVERT false       // se as cores aparecerem invertidas, troque para true

// ---------- Cartao microSD (fixo na placa) ----------
#define SD_SCK    18
#define SD_MISO   19
#define SD_MOSI   23
#define SD_CS     5

// ---------- Outros pinos da placa ----------
#define SPK_PIN   26     // saida de som (alto-falante)
#define BOOT_PIN  0      // botao BOOT da placa

// ---------- Entradas ----------
// 1) Expansor PCF8574 (8 botoes) no conector CN1: SDA = IO22, SCL = IO27
#define USE_PCF8574   1
#define I2C_SDA       22
#define I2C_SCL       27
#define PCF_ADDR      0x20   // depende dos jumpers A0/A1/A2 do modulo

// 2) ADKeyboard (5 botoes num pino so) no IO35, conector P3.
//    Alimente o modulo com o 3V3 do conector CN1.
#define USE_ADKEYBOARD 0
#define KEY_PIN       35
// tensoes medidas no LatinhaColor (mV): SW1 142, SW2 470, SW3 984, SW4 1605, SW5 2570, solto 3150
// Recalibre olhando o valor "ADK mV" da tela de teste.
#define KB_SW1_MAX    300
#define KB_SW2_MAX    727
#define KB_SW3_MAX    1295
#define KB_SW4_MAX    2087
#define KB_SW5_MAX    2860

// 3) Botao BOOT da placa funcionando como MENU (KEY_HOME)
#define USE_BOOT_AS_HOME 1

// ---------- Tempos ----------
#define DEBOUNCE_MS  30
#define LONG_MS      600
#define REPEAT_MS    120
