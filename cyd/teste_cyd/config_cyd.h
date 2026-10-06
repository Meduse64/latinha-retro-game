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
// MADCTL do ILI9341 (orientacao e ordem das cores). Com o valor da biblioteca (0x48) este CYD mostrou a
// imagem de lado e com vermelho e azul trocados. O 0x20 FOI CONFERIDO NESTE CYD (foto): imagem em pe, sem
// espelho, com o conector USB em cima e as cores certas. O 0xE0 deve dar o mesmo girado 180 graus (USB em
// baixo). O bit 0x08 liga a ordem BGR (cores trocadas). Se outro CYD sair espelhado ou de lado, experimente
// 0x60, 0xA0, 0x00, 0x40, 0x80 ou 0xC0.
#define TFT_MADCTL 0x20

// ---------- Cartao microSD (fixo na placa) ----------
#define SD_SCK    18
#define SD_MISO   19
#define SD_MOSI   23
#define SD_CS     5

// ---------- Outros pinos da placa ----------
#define SPK_PIN   26     // saida de som (alto-falante)
#define CYD_BOOT_PIN 0   // botao BOOT da placa (BOOT_PIN ja existe no nucleo ESP32)

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

// 4) Toque resistivo XPT2046 (pinos fixos da placa). Lido por software, sem biblioteca,
//    porque os dois SPI do ESP32 ja estao ocupados (tela em HSPI, cartao SD em VSPI).
#define USE_TOUCH      1
#define TOUCH_CLK      25
#define TOUCH_MOSI     32
#define TOUCH_MISO     39
#define TOUCH_CS       33
#define TOUCH_IRQ      36
// Calibracao: valores brutos (0 a 4095) nas bordas. RAWX cobre o lado comprido da tela (320)
// e RAWY o lado curto (240). Toque os 4 cantos e veja os valores "raw" na tela.
#define TOUCH_RAWX_MIN 200
#define TOUCH_RAWX_MAX 3700
#define TOUCH_RAWY_MIN 240
#define TOUCH_RAWY_MAX 3800
#define TOUCH_SWAP_XY  1       // 1 = o X bruto corre ao longo da altura da tela (CYD em pe)
#define TOUCH_INV_X    0       // troque para 1 se a cruz andar ao contrario na horizontal
#define TOUCH_INV_Y    0       // idem na vertical

// 5) Um toque em qualquer lugar da tela vale como a tecla MENU (o plano do menu de pausa)
#define USE_TOUCH_AS_HOME 1

// ---------- Tempos ----------
#define DEBOUNCE_MS  30
#define LONG_MS      600
#define REPEAT_MS    120
