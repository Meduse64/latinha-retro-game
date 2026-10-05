// ============================================================
//  Teste de hardware do CYD (ESP32-2432S028R) - fase 1 do porte do LatinhaColor
//
//  Mostra, numa tela logica de 160 x 128 (a mesma do LatinhaColor) ampliada x1,5
//  para 240 x 192 com o CYD em pe (240 x 320):
//    - faixas de cor e tres quadrados R G B (confira se as cores estao certas)
//    - as 9 teclas do LatinhaColor acendendo quando apertadas
//    - o byte lido do PCF8574 e a tensao do ADKeyboard (IO35)
//    - cartao SD (tamanho e quantos jogos em /roms/gb, gbc, gg, sms)
//    - memoria livre e tempo de envio de cada quadro a tela
//
//  Bibliotecas (Arduino IDE > Gerenciar Bibliotecas): Adafruit GFX Library,
//  Adafruit ILI9341. Placa: "ESP32 Dev Module". Monitor serial: 115200.
// ============================================================
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "config_cyd.h"

// ---------- Cores (RGB565), as mesmas do LatinhaColor ----------
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
const uint16_t C_BG = 0x0000, C_FG = 0xFFFF;
const uint16_t C_BAR = rgb(20, 40, 90), C_GRAY = rgb(120, 120, 120), C_DGRAY = rgb(45, 45, 50);
const uint16_t C_AMBER = rgb(255, 190, 0), C_CYAN = rgb(0, 220, 255), C_GREEN = rgb(40, 210, 90);
const uint16_t C_RED = rgb(240, 60, 60), C_BLUE = rgb(70, 150, 255), C_ORANGE = rgb(255, 130, 30);
const uint16_t C_PINK = rgb(255, 100, 180), C_YELLOW = rgb(255, 230, 40);

// ---------- Tela ----------
const int SW = 160, SH = 128;          // tela logica
const int OUT_W = 240, OUT_H = 192;    // ampliada x1,5 (160 x 1,5 = 240 = largura do CYD em pe)

SPIClass tftSpi(HSPI);
SPIClass sdSpi(VSPI);
Adafruit_ILI9341 tft(&tftSpi, TFT_DC, TFT_CS, TFT_RST);
GFXcanvas16* gfxCanvas = nullptr;
#define gfx (*gfxCanvas)

// Manda a tela logica para o CYD ampliada x1,5, uma linha por vez (sem buffer grande):
//  - colunas: de cada 2 pixels de origem saem 3 (a, a, b)
//  - linhas: as linhas pares de origem saem 2 vezes e as impares 1 (128 -> 192)
void flush() {
  static uint16_t line[OUT_W];
  const uint16_t* src = gfx.getBuffer();
  tft.startWrite();
  tft.setAddrWindow(0, 0, OUT_W, OUT_H);
  for (int y = 0; y < SH; y++) {
    const uint16_t* s = src + y * SW;
    for (int k = 0; k < SW / 2; k++) {
      uint16_t a = s[2 * k], b = s[2 * k + 1];
      line[3 * k] = a;
      line[3 * k + 1] = a;
      line[3 * k + 2] = b;
    }
    tft.writePixels(line, OUT_W);
    if ((y & 1) == 0) tft.writePixels(line, OUT_W);
  }
  tft.endWrite();
}

// ---------- Teclas ----------
enum Key : uint8_t { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK, KEY_BACK, KEY_A, KEY_B, KEY_HOME, KEY_COUNT };
enum BtnEvt : uint8_t { EV_NONE, EV_DOWN, EV_CLICK, EV_LONG, EV_REPEAT };
const char* const KEYNAME[KEY_COUNT] = { "CIMA", "BAIXO", "ESQ", "DIR", "OK", "VOLTAR", "A", "B", "MENU" };

struct KeyState { bool down, longFired; uint32_t downAt, lastRep, lastChange; };
KeyState keys[KEY_COUNT];
uint16_t clicks[KEY_COUNT];

#if USE_PCF8574
// pino do PCF8574 -> tecla (ver PORTE_CYD.md, secao 3)
const Key PCF_MAP[8] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK, KEY_BACK, KEY_B, KEY_A };
#endif
bool pcfOk = false;
uint8_t pcfAddr = PCF_ADDR;            // endereco achado na inicializacao (ver pcfInit)
uint8_t pcfRaw = 0xFF;
int adMv = 0, adSw = 0;

void readRaw(bool raw[KEY_COUNT]) {
  for (int i = 0; i < KEY_COUNT; i++) raw[i] = false;
#if USE_PCF8574
  if (pcfOk) {
    Wire.requestFrom(pcfAddr, (uint8_t)1);
    if (Wire.available()) pcfRaw = Wire.read();
    for (int b = 0; b < 8; b++) if (!(pcfRaw & (1 << b))) raw[PCF_MAP[b]] = true;   // 0 = apertado
  }
#endif
#if USE_ADKEYBOARD
  adMv = analogReadMilliVolts(KEY_PIN);
  adSw = adMv < KB_SW1_MAX ? 1 : adMv < KB_SW2_MAX ? 2 : adMv < KB_SW3_MAX ? 3 : adMv < KB_SW4_MAX ? 4 : adMv < KB_SW5_MAX ? 5 : 0;
  if (adSw == 1) raw[KEY_BACK] = true;     // azul esquerda
  if (adSw == 2) raw[KEY_A] = true;        // branco cima
  if (adSw == 3) raw[KEY_B] = true;        // branco baixo
  if (adSw == 4) raw[KEY_OK] = true;       // azul direita
  if (adSw == 5) raw[KEY_HOME] = true;     // amarelo
#endif
#if USE_BOOT_AS_HOME
  if (digitalRead(BOOT_PIN) == LOW) raw[KEY_HOME] = true;
#endif
}

BtnEvt pollKey(KeyState& b, bool raw) {   // o mesmo do LatinhaColor
  uint32_t ms = millis();
  if (raw != b.down) {
    if (ms - b.lastChange < DEBOUNCE_MS) return EV_NONE;
    b.lastChange = ms;
    b.down = raw;
    if (raw) { b.downAt = ms; b.longFired = false; return EV_DOWN; }
    return b.longFired ? EV_NONE : EV_CLICK;
  }
  if (b.down) {
    if (!b.longFired && ms - b.downAt >= LONG_MS) { b.longFired = true; b.lastRep = ms; return EV_LONG; }
    if (b.longFired && ms - b.lastRep >= REPEAT_MS) { b.lastRep = ms; return EV_REPEAT; }
  }
  return EV_NONE;
}

// ---------- Texto (o mesmo do LatinhaColor) ----------
int textW(const char* s, uint8_t sz = 1) { return strlen(s) * 6 * sz; }
void txt(int x, int y, const char* s, uint16_t c = C_FG, uint8_t sz = 1) {
  gfx.setTextSize(sz); gfx.setTextColor(c); gfx.setCursor(x, y); gfx.print(s);
}
void txtC(int y, const char* s, uint16_t c = C_FG, uint8_t sz = 1) { txt((SW - textW(s, sz)) / 2, y, s, c, sz); }

#if USE_PCF8574
// Procura o PCF8574 (0x20 a 0x27) ou o PCF8574A (0x38 a 0x3F). Tenta primeiro o endereco da configuracao.
bool pcfTry(uint8_t a) {
  Wire.beginTransmission(a);
  Wire.write(0xFF);                        // todos os pinos como entrada (nivel alto)
  return Wire.endTransmission() == 0;
}
void pcfInit() {
  Wire.begin(I2C_SDA, I2C_SCL);
  if (pcfTry(PCF_ADDR)) { pcfOk = true; pcfAddr = PCF_ADDR; return; }
  for (uint8_t a = 0x20; a <= 0x3F; a++) {
    if (a == PCF_ADDR || (a > 0x27 && a < 0x38)) continue;
    if (pcfTry(a)) { pcfOk = true; pcfAddr = a; return; }
  }
}
#endif

// ---------- Cartao SD ----------
const char* const DIRS[4] = { "/roms/gb", "/roms/gbc", "/roms/gg", "/roms/sms" };
const char* const DIRNAME[4] = { "gb", "gbc", "gg", "sms" };
bool sdOk = false;
uint32_t sdMB = 0;
int dirCount[4] = { -1, -1, -1, -1 };    // -1 = pasta nao existe

int countFiles(const char* path) {
  File d = SD.open(path);
  if (!d || !d.isDirectory()) return -1;
  int n = 0;
  for (File f = d.openNextFile(); f; f = d.openNextFile()) {
    if (!f.isDirectory()) n++;
    f.close();
  }
  d.close();
  return n;
}
void sdInit() {
  sdSpi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  sdOk = SD.begin(SD_CS, sdSpi, 25000000);
  if (!sdOk) return;
  sdMB = (uint32_t)(SD.cardSize() / (1024ULL * 1024ULL));
  for (int i = 0; i < 4; i++) dirCount[i] = countFiles(DIRS[i]);
}

// ---------- Tela de teste ----------
uint32_t flushUs = 0;

void drawTest() {
  char b[40];
  gfx.fillScreen(C_BG);
  gfx.fillRect(0, 0, SW, 12, C_BAR);
  txt(3, 2, "TESTE CYD");
  snprintf(b, sizeof(b), "%lu ms", (unsigned long)(flushUs / 1000));
  txt(116, 2, b, C_AMBER);

  const uint16_t cs[7] = { C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_CYAN, C_BLUE, C_PINK };
  for (int i = 0; i < 7; i++) gfx.fillRect(i * 23, 14, 23, 5, cs[i]);

  // as 9 teclas
  for (int i = 0; i < KEY_COUNT; i++) {
    int x = 2 + (i % 3) * 53, y = 22 + (i / 3) * 14;
    bool on = keys[i].down;
    gfx.fillRoundRect(x, y, 51, 12, 3, on ? C_GREEN : C_DGRAY);
    txt(x + 3, y + 2, KEYNAME[i], on ? C_BG : C_FG);
    snprintf(b, sizeof(b), "%u", clicks[i]);
    txt(x + 51 - textW(b) - 2, y + 2, b, on ? C_BG : C_GRAY);
  }

  int y = 66;
#if USE_PCF8574
  if (pcfOk) snprintf(b, sizeof(b), "PCF8574 0x%02X: 0x%02X", pcfAddr, pcfRaw);
  else snprintf(b, sizeof(b), "PCF8574: nao achado");
  txt(2, y, b, pcfOk ? C_GREEN : C_RED);
#else
  txt(2, y, "PCF8574 desligado", C_GRAY);
#endif
  y += 10;
#if USE_ADKEYBOARD
  snprintf(b, sizeof(b), "ADK IO35: %d mV  SW%d", adMv, adSw);
  txt(2, y, b, C_CYAN);
#else
  txt(2, y, "ADKeyboard desligado", C_GRAY);
#endif
  y += 10;
  if (sdOk) {
    snprintf(b, sizeof(b), "SD: %lu MB", (unsigned long)sdMB);
    txt(2, y, b, C_GREEN);
    y += 10;
    for (int i = 0; i < 4; i++) {
      int x = 2 + i * 40;
      if (dirCount[i] < 0) snprintf(b, sizeof(b), "%s -", DIRNAME[i]);
      else snprintf(b, sizeof(b), "%s %d", DIRNAME[i], dirCount[i]);
      txt(x, y, b, dirCount[i] < 0 ? C_GRAY : C_AMBER);
    }
  } else {
    txt(2, y, "SD: nao montou", C_RED);
    y += 10;
    txt(2, y, "FAT32? cartao no slot?", C_GRAY);
  }
  y += 10;
  snprintf(b, sizeof(b), "livre %lu KB", (unsigned long)(ESP.getFreeHeap() / 1024));
  txt(2, y, b, C_FG);
}

void drawBand() {                          // faixa de baixo, em resolucao nativa (240 x 128)
  tft.fillRect(0, OUT_H, 240, 320 - OUT_H, rgb(20, 20, 30));
  tft.fillRect(10, OUT_H + 10, 60, 40, ILI9341_RED);
  tft.fillRect(90, OUT_H + 10, 60, 40, ILI9341_GREEN);
  tft.fillRect(170, OUT_H + 10, 60, 40, ILI9341_BLUE);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(30, OUT_H + 56); tft.print("R");
  tft.setCursor(110, OUT_H + 56); tft.print("G");
  tft.setCursor(190, OUT_H + 56); tft.print("B");
  tft.setTextSize(1);
  tft.setTextColor(rgb(150, 150, 150));
  tft.setCursor(10, OUT_H + 84); tft.print("Os quadrados devem ser vermelho,");
  tft.setCursor(10, OUT_H + 96); tft.print("verde e azul. Se nao forem,");
  tft.setCursor(10, OUT_H + 108); tft.print("mude TFT_INVERT em config_cyd.h");
}

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(BOOT_PIN, INPUT_PULLUP);

  gfxCanvas = new GFXcanvas16(SW, SH);
  gfx.setTextWrap(false);

  tftSpi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin(TFT_HZ);
  tft.setRotation(0);                      // em pe: 240 x 320
  tft.invertDisplay(TFT_INVERT);
  tft.fillScreen(ILI9341_BLACK);
  drawBand();

#if USE_PCF8574
  pcfInit();
#endif
  sdInit();

  Serial.printf("Memoria livre: %u bytes, tela logica %s, PCF8574 %s (0x%02X), SD %s\n", ESP.getFreeHeap(),
                gfx.getBuffer() ? "OK" : "SEM MEMORIA", pcfOk ? "ok" : "nao achado", pcfAddr, sdOk ? "ok" : "nao montou");
}

void loop() {
  static uint32_t lastDraw = 0, lastLog = 0;
  uint32_t ms = millis();

  bool raw[KEY_COUNT];
  readRaw(raw);
  for (int i = 0; i < KEY_COUNT; i++) {
    BtnEvt ev = pollKey(keys[i], raw[i]);
    if (ev == EV_CLICK) clicks[i]++;
    if (ev == EV_DOWN) Serial.printf("tecla %s\n", KEYNAME[i]);
  }

  if (ms - lastDraw >= 50) {
    lastDraw = ms;
    drawTest();
    uint32_t t0 = micros();
    flush();
    flushUs = micros() - t0;
  }
  if (ms - lastLog >= 2000) {
    lastLog = ms;
    Serial.printf("pcf=0x%02X adk=%dmV envio=%luus livre=%u\n", pcfRaw, adMv, (unsigned long)flushUs, ESP.getFreeHeap());
  }
  delay(1);
}
