// ============================================================
//  Teste de hardware do CYD (ESP32-2432S028R) - fase 1 do porte do LatinhaColor
//
//  Mostra, numa tela logica de 160 x 128 (a mesma do LatinhaColor) ampliada x1,5
//  para 240 x 192 com o CYD em pe (240 x 320):
//    - faixas de cor e tres quadrados R G B (confira se as cores estao certas)
//    - as 9 teclas do LatinhaColor acendendo quando apertadas
//    - o toque na tela (XPT2046): posicao, valores brutos e uma cruz amarela que segue o dedo;
//      um toque em qualquer lugar acende a tecla MENU
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
#include "teclas.h"

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
const char* const KEYNAME[KEY_COUNT] = { "CIMA", "BAIXO", "ESQ", "DIR", "OK", "VOLTAR", "A", "B", "MENU" };

KeyState keys[KEY_COUNT];
uint16_t clicks[KEY_COUNT];

#if USE_PCF8574
// pino do PCF8574 -> tecla (ver PORTE_CYD.md, secao 3)
const Key PCF_MAP[8] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK, KEY_BACK, KEY_B, KEY_A };
#endif
bool pcfOk = false;
uint8_t pcfAddr = PCF_ADDR;            // endereco achado na inicializacao (ver pcfInit)
uint8_t pcfRaw = 0xFF;
String i2cInfo = "sem varredura";      // resultado da varredura do I2C, repetido no Monitor Serial
char i2cShort[40] = "";                // o que esta em SDA e SCL, mostrado na tela
bool i2cSwapped = false;               // achou o modulo com SDA e SCL invertidos
int adMv = 0, adSw = 0;

// Toque (XPT2046, lido por software; as funcoes ficam mais abaixo)
KeyState touchKey;
bool touchDown = false;                // dedo na tela, ja com debounce
int touchX = 0, touchY = 0;            // posicao na tela: 0 a 239 e 0 a 319
int touchRawX = 0, touchRawY = 0;
uint16_t touchClicks = 0;

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
  if (digitalRead(CYD_BOOT_PIN) == LOW) raw[KEY_HOME] = true;
#endif
#if USE_TOUCH && USE_TOUCH_AS_HOME
  if (touchDown) raw[KEY_HOME] = true;
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

#if USE_TOUCH
// Le um canal de 12 bits do XPT2046 (0xD0 = X bruto, 0x90 = Y bruto), com SPI feito a mao.
uint16_t xptRead(uint8_t cmd) {
  digitalWrite(TOUCH_CS, LOW);
  for (int i = 7; i >= 0; i--) {           // byte de comando, bit mais alto primeiro
    digitalWrite(TOUCH_MOSI, (cmd >> i) & 1);
    digitalWrite(TOUCH_CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(TOUCH_CLK, LOW);
    delayMicroseconds(1);
  }
  uint16_t v = 0;
  for (int i = 0; i < 16; i++) {           // 1 bit vazio, 12 de dado e 3 de sobra
    digitalWrite(TOUCH_CLK, HIGH);
    delayMicroseconds(1);
    v = (v << 1) | digitalRead(TOUCH_MISO);
    digitalWrite(TOUCH_CLK, LOW);
    delayMicroseconds(1);
  }
  digitalWrite(TOUCH_CS, HIGH);
  return (v >> 3) & 0x0FFF;
}

void touchPoll() {
  bool pressed = false;
  if (digitalRead(TOUCH_IRQ) == LOW) {     // o chip puxa o IRQ para baixo enquanto ha um dedo
    xptRead(0xD0);                         // primeira leitura descartada
    uint32_t sx = 0, sy = 0;
    for (int i = 0; i < 4; i++) { sx += xptRead(0xD0); sy += xptRead(0x90); }
    int rx = sx / 4, ry = sy / 4;
    if (rx > 100 && rx < 4000 && ry > 100 && ry < 4000) {   // fora disso e ruido
      pressed = true;
      touchRawX = rx;
      touchRawY = ry;
      long px, py;
      if (TOUCH_SWAP_XY) {
        px = map(ry, TOUCH_RAWY_MIN, TOUCH_RAWY_MAX, 0, 239);
        py = map(rx, TOUCH_RAWX_MIN, TOUCH_RAWX_MAX, 0, 319);
      } else {
        px = map(rx, TOUCH_RAWX_MIN, TOUCH_RAWX_MAX, 0, 239);
        py = map(ry, TOUCH_RAWY_MIN, TOUCH_RAWY_MAX, 0, 319);
      }
      px = constrain(px, 0, 239);
      py = constrain(py, 0, 319);
      touchX = TOUCH_INV_X ? 239 - px : px;
      touchY = TOUCH_INV_Y ? 319 - py : py;
    }
  }
  BtnEvt ev = pollKey(touchKey, pressed);
  touchDown = touchKey.down;
  if (ev == EV_DOWN) Serial.printf("toque raw %d,%d -> tela %d,%d\n", touchRawX, touchRawY, touchX, touchY);
  if (ev == EV_CLICK) touchClicks++;
}
#endif

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
// Diz o que esta ligado a um fio, ligando um pull-down e um pull-up internos (cerca de 45 kohm cada).
const char* lineState(int pin) {
  pinMode(pin, INPUT_PULLDOWN); delay(3); int d = digitalRead(pin);
  pinMode(pin, INPUT_PULLUP);   delay(3); int u = digitalRead(pin);
  pinMode(pin, INPUT);
  if (d == 0 && u == 1) return "solto";          // nada o segura: sem fio, ou modulo sem energia
  if (d == 0 && u == 0) return "GND";            // preso no GND: fio em curto ou ligado ao pino errado
  if (d == 1 && u == 1) return "alto";           // pull-up externo (modulo alimentado) ou 3V3
  return "?";
}

bool pcfProbe(uint8_t a) {
  Wire.beginTransmission(a);
  return Wire.endTransmission() == 0;
}

// Procura o PCF8574 nos enderecos 0x20 a 0x27 e 0x38 a 0x3F. Tenta primeiro o da configuracao.
bool pcfFind() {
  if (pcfTry(PCF_ADDR)) { pcfOk = true; pcfAddr = PCF_ADDR; return true; }
  for (uint8_t a = 0x20; a <= 0x3F; a++) {
    if (a == PCF_ADDR || (a > 0x27 && a < 0x38)) continue;
    if (pcfTry(a)) { pcfOk = true; pcfAddr = a; return true; }
  }
  return false;
}

// Diagnostico do I2C, para achar erro de ligacao sem regravar: diz o que esta em cada fio e, se os dois
// estiverem altos, procura o PCF8574, tambem com SDA e SCL trocados. Roda no boot e, enquanto o
// PCF8574 nao for achado, a cada 0,5 s (os fios podem ser mudados com o sketch rodando).
void i2cDiagnose(bool full) {
  Wire.end();
  const char* s = lineState(I2C_SDA);
  const char* c = lineState(I2C_SCL);
  snprintf(i2cShort, sizeof(i2cShort), "I2C SDA:%s SCL:%s", s, c);
  String info = String("SDA(IO") + I2C_SDA + ")=" + s + " SCL(IO" + I2C_SCL + ")=" + c;
  i2cSwapped = false;
  if (!strcmp(s, "alto") && !strcmp(c, "alto")) {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setTimeOut(5);
    int n = 0;
    info += " achados:";
    for (uint8_t a = full ? 1 : 0x20; a < (full ? 127 : 0x40); a++) {
      if ((!full && a > 0x27 && a < 0x38) || !pcfProbe(a)) continue;
      char t[8]; snprintf(t, sizeof(t), " 0x%02X", a); info += t; n++;
    }
    if (n == 0) {                          // nada: tenta com os dois fios trocados
      Wire.end();
      Wire.begin(I2C_SCL, I2C_SDA);
      Wire.setTimeOut(5);
      for (uint8_t a = 0x20; a < 0x40; a++) {
        if ((a > 0x27 && a < 0x38) || !pcfProbe(a)) continue;
        i2cSwapped = true;
        char t[40]; snprintf(t, sizeof(t), " | com SDA e SCL TROCADOS: 0x%02X", a); info += t;
        break;
      }
      if (!i2cSwapped) info += " nenhum";
      Wire.end();
      Wire.begin(I2C_SDA, I2C_SCL);
    }
    pcfFind();
  } else {
    info += " (nao varre: os dois fios precisam estar altos)";
  }
  if (!i2cInfo.equals(info)) { i2cInfo = info; Serial.println(info); }   // so imprime quando muda
}

void pcfInit() { i2cDiagnose(true); }
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
  else snprintf(b, sizeof(b), "%s", i2cSwapped ? "SDA e SCL TROCADOS" : "PCF8574: nao achado");
  txt(2, y, b, pcfOk ? C_GREEN : C_RED);
#else
  txt(2, y, "PCF8574 desligado", C_GRAY);
#endif
  y += 10;
#if USE_ADKEYBOARD
  snprintf(b, sizeof(b), "ADK IO35: %d mV  SW%d", adMv, adSw);
  txt(2, y, b, C_CYAN);
#else
  txt(2, y, (USE_PCF8574 && !pcfOk) ? i2cShort : "ADKeyboard desligado", (USE_PCF8574 && !pcfOk) ? C_AMBER : C_GRAY);
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
  y += 10;
#if USE_TOUCH
  if (touchDown) snprintf(b, sizeof(b), "Toque %d,%d (%d,%d)", touchX, touchY, touchRawX, touchRawY);
  else snprintf(b, sizeof(b), "Toque a tela (%u)", touchClicks);
  txt(2, y, b, touchDown ? C_YELLOW : C_GRAY);
  if (touchDown && touchY < OUT_H) {       // cruz na parte de cima (a de baixo e desenhada em updateBandCross)
    int cx = touchX * SW / 240, cy = touchY * SH / OUT_H;
    gfx.drawFastHLine(cx - 6, cy, 13, C_YELLOW);
    gfx.drawFastVLine(cx, cy - 6, 13, C_YELLOW);
  }
#else
  txt(2, y, "Toque desligado", C_GRAY);
#endif
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
#if USE_TOUCH
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(10, OUT_H + 120); tft.print("Toque aqui: cruz amarela");
#endif
}

#if USE_TOUCH
int crossX = -1, crossY = -1;              // cruz desenhada na faixa de baixo (-1 = nenhuma)
void updateBandCross() {
  int nx = -1, ny = -1;
  if (touchDown && touchY >= OUT_H) { nx = touchX; ny = touchY; }
  if (nx == crossX && ny == crossY) return;
  if (crossX >= 0) drawBand();             // apaga a cruz anterior redesenhando a faixa
  crossX = nx;
  crossY = ny;
  if (nx >= 0) {
    tft.drawFastHLine(nx - 8, ny, 17, ILI9341_YELLOW);
    tft.drawFastVLine(nx, ny - 8, 17, ILI9341_YELLOW);
  }
}
#endif

// ---------- Setup / loop ----------
void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(CYD_BOOT_PIN, INPUT_PULLUP);

  gfxCanvas = new GFXcanvas16(SW, SH);
  gfx.setTextWrap(false);

  tftSpi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin(TFT_HZ);
  tft.setRotation(0);                      // em pe: a biblioteca passa a tratar a tela como 240 x 320
  {                                        // mas o painel do CYD aparece de lado com o MADCTL da biblioteca
    uint8_t madctl = TFT_MADCTL;           // (e com vermelho e azul trocados): usamos o nosso, ver config_cyd.h
    tft.sendCommand(ILI9341_MADCTL, &madctl, 1);
  }
  tft.invertDisplay(TFT_INVERT);
  tft.fillScreen(ILI9341_BLACK);
  drawBand();

#if USE_TOUCH
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(TOUCH_CLK, OUTPUT);
  digitalWrite(TOUCH_CLK, LOW);
  pinMode(TOUCH_MOSI, OUTPUT);
  pinMode(TOUCH_MISO, INPUT);
  pinMode(TOUCH_IRQ, INPUT);
#endif
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

#if USE_TOUCH
  touchPoll();
#endif
#if USE_PCF8574
  static uint32_t lastI2c = 0;
  if (!pcfOk && ms - lastI2c >= 500) { lastI2c = ms; i2cDiagnose(false); }   // refaz o diagnostico ate achar
#endif
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
#if USE_TOUCH
    updateBandCross();
#endif
  }
  if (ms - lastLog >= 2000) {
    lastLog = ms;
    Serial.printf("pcf=0x%02X adk=%dmV envio=%luus livre=%u | I2C %s\n", pcfRaw, adMv, (unsigned long)flushUs, ESP.getFreeHeap(),
                  i2cInfo.c_str());
  }
  delay(1);
}
