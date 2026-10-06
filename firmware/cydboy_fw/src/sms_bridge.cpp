#include "sms_bridge.h"

#if HAS_SMS
#include "display.h"
#include "sd_manager.h"
#include <Arduino.h>
#include <SD.h>
#include <esp_partition.h>

extern "C" {
#include "smsplus/shared.h"
extern uint8 *cacheStore;                       // cache de desenhos do SMS Plus (32 KB, render.c)
void* latinha_sms_piece(int i, unsigned* len);  // latinha_state.c
void latinha_sms_before_load(void);
void latinha_sms_after_load(void);
}

// O SMS Plus le a memoria do jogo por esta funcao
extern "C" char unalChar(const char* adr) { return *adr; }

static uint8_t s_sys = SYS_SMS;
static const esp_partition_t* s_part = nullptr;
static esp_partition_mmap_handle_t s_mapH = 0;
static bool s_mapped = false;
static const uint8_t* s_rom = nullptr;
static uint32_t s_romSize = 0;
static uint8_t s_dummy[0x2000];                 // onde o Sega "escreve" na ROM (descartado)
static uint8_t* s_sram = nullptr;               // RAM de bateria (32 KB), so se o jogo ligar
static char s_path[MAX_PATHLEN] = {0};
static char s_err[48] = "";

const char* smsb_error() { return s_err; }

// ─── ROM: do cartao SD para a particao de dados da flash ────────────────────
// A particao guarda um cabecalho de 4 KB e depois a ROM. O cabecalho e gravado por ultimo, e
// diz de qual arquivo e a copia (caminho, tamanho e data): escolhendo de novo o mesmo jogo, a
// copia, que leva alguns segundos, e pulada.
struct RomHdr { char magic[4]; uint32_t size, pathHash, mtime; };
#define HDR_SZ 4096

static uint32_t fnv(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
    return h;
}

static void progress(uint32_t done, uint32_t total) {
    int w = SCREEN_W - 80;
    tft.drawRect(40, SCREEN_H - 36, w, 8, 0x4A69);
    tft.fillRect(41, SCREEN_H - 35, (int)((uint64_t)(w - 2) * done / total), 6, 0x07E0);
}

bool smsb_open(const char* path, uint8_t sys) {
    s_err[0] = 0;
    s_sys = sys;
    strncpy(s_path, path, sizeof(s_path) - 1);
    s_path[sizeof(s_path) - 1] = 0;

    if (!s_part) s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
    if (!s_part) { snprintf(s_err, sizeof(s_err), "sem particao de jogos"); return false; }

    File f = SD.open(path, FILE_READ);
    if (!f) { snprintf(s_err, sizeof(s_err), "nao abriu a ROM"); return false; }
    uint32_t size = f.size();
    uint32_t mtime = (uint32_t)f.getLastWrite();
    if (size < 0x4000 || HDR_SZ + size > s_part->size) {
        f.close();
        snprintf(s_err, sizeof(s_err), size < 0x4000 ? "ROM invalida" : "ROM grande demais");
        return false;
    }

    RomHdr h;
    esp_partition_read(s_part, 0, &h, sizeof(h));
    uint32_t ph = fnv(path);
    bool same = !memcmp(h.magic, "SROM", 4) && h.size == size && h.pathHash == ph && h.mtime == mtime;
    if (same) {
        Serial.printf("[SMS] ROM ja esta na flash: %s (%u KB)\n", path, (unsigned)(size / 1024));
    } else {
        Serial.printf("[SMS] Copiando %s (%u KB) para a flash...\n", path, (unsigned)(size / 1024));
        uint32_t t0 = millis();
        uint32_t span = ((HDR_SZ + size + 4095) / 4096) * 4096;
        if (esp_partition_erase_range(s_part, 0, span) != ESP_OK) {
            f.close(); snprintf(s_err, sizeof(s_err), "erro ao apagar a flash"); return false;
        }
        uint8_t* buf = (uint8_t*)malloc(4096);
        if (!buf) { f.close(); snprintf(s_err, sizeof(s_err), "sem memoria"); return false; }
        uint32_t off = 0;
        bool ok = true;
        while (off < size) {
            size_t n = f.read(buf, 4096);
            if (n == 0) { ok = false; break; }
            if (esp_partition_write(s_part, HDR_SZ + off, buf, (n + 3) & ~3u) != ESP_OK) { ok = false; break; }
            off += n;
            if ((off & 0xFFFF) == 0 || off >= size) progress(off, size);
        }
        free(buf);
        if (!ok) { f.close(); snprintf(s_err, sizeof(s_err), "erro ao copiar a ROM"); return false; }
        RomHdr nh = { { 'S', 'R', 'O', 'M' }, size, ph, mtime };
        uint8_t hb[16]; memcpy(hb, &nh, sizeof(nh));
        esp_partition_write(s_part, 0, hb, sizeof(hb));         // cabecalho por ultimo
        Serial.printf("[SMS] Copia pronta em %u ms\n", (unsigned)(millis() - t0));
    }
    f.close();

    // mapeia da posicao 0 e soma o cabecalho (o mapeamento da ESP32 e em paginas de 64 KB)
    if (s_mapped) { esp_partition_munmap(s_mapH); s_mapped = false; }
    const void* base = nullptr;
    if (esp_partition_mmap(s_part, 0, HDR_SZ + size, ESP_PARTITION_MMAP_DATA, &base, &s_mapH) != ESP_OK) {
        snprintf(s_err, sizeof(s_err), "erro ao mapear a ROM");
        return false;
    }
    s_mapped = true;
    s_rom = (const uint8_t*)base + HDR_SZ;
    s_romSize = size;
    return true;
}

// ─── Iniciar e parar ────────────────────────────────────────────────────────
bool smsb_start() {
    free(cacheStore); cacheStore = nullptr;
    free(s_sram); s_sram = nullptr;
    cacheStore = (uint8*)malloc(512 * 64);
    if (!cacheStore) { snprintf(s_err, sizeof(s_err), "sem memoria"); return false; }
    bitmap.data = nullptr;                      // as linhas vao direto para a tela (latinha_sms_line)
    bitmap.width = 256; bitmap.height = 192; bitmap.pitch = 256; bitmap.depth = 8;
    sms.dummy = s_dummy;
    sms.sram = nullptr;
    cart.rom = (uint8*)s_rom;
    cart.pages = (s_romSize + 0x3FFF) / 0x4000;
    cart.type = (s_sys == SYS_GG) ? TYPE_GG : TYPE_SMS;
    emu_system_init(0);                         // 0 = sem som
    Serial.printf("[SMS] %s, %u paginas de 16 KB. Heap livre: %u\n", s_sys == SYS_GG ? "Game Gear" : "Master System",
                  (unsigned)cart.pages, (unsigned)ESP.getFreeHeap());
    return true;
}

void smsb_close() {
    free(cacheStore); cacheStore = nullptr;
    free(s_sram); s_sram = nullptr;
    if (s_mapped) { esp_partition_munmap(s_mapH); s_mapped = false; }
    s_rom = nullptr; s_romSize = 0;
}

void smsb_reset() { system_reset(); }

// Mapeia os botoes do CYDboy (1 = apertado) para os do SMS Plus.
// B = botao 1, A = botao 2, Start = Start (Game Gear) ou Pause (Master System).
void smsb_run_frame(uint8_t jpad, bool draw) {
    int p = 0;
    if (jpad & 0x04) p |= INPUT_UP;
    if (jpad & 0x08) p |= INPUT_DOWN;
    if (jpad & 0x02) p |= INPUT_LEFT;
    if (jpad & 0x01) p |= INPUT_RIGHT;
    if (jpad & 0x20) p |= INPUT_BUTTON1;
    if (jpad & 0x10) p |= INPUT_BUTTON2;
    input.pad[0] = p;
    input.system = (jpad & 0x80) ? (s_sys == SYS_GG ? INPUT_START : INPUT_PAUSE) : 0;
    sms_frame(draw ? 0 : 1);
}

// ─── Chamados pelo SMS Plus ─────────────────────────────────────────────────
// Cada linha pronta (256 pixels em 3-3-2) vai direto para a tela
uint32_t g_sms_render_us = 0;                   // tempo gasto enviando linhas a tela (para o [PERF])
extern "C" void latinha_sms_line(const uint8_t* s, int line) {
    uint32_t t0 = micros();
    if (s_sys == SYS_GG) display_push_gg_line(line - 24, s + 48);   // linhas 24..167, colunas 48..207
    else display_push_sms_line(line, s);
    g_sms_render_us += micros() - t0;
}

// O jogo pede a RAM de bateria so quando a liga. Carrega o save do cartao SD, se existir.
extern "C" uint8* latinha_alloc_sram(void) {
    if (!s_sram) {
        s_sram = (uint8_t*)malloc(0x8000);
        if (!s_sram) return nullptr;            // sem memoria: o jogo roda, mas sem save
        memset(s_sram, 0, 0x8000);
        char sp[MAX_PATHLEN];
        sd_get_save_path(s_path, sp, sizeof(sp));
        File f = SD.open(sp, FILE_READ);
        if (f) { f.read(s_sram, 0x8000); f.close(); Serial.printf("[SMS] Save carregado: %s\n", sp); }
    }
    return s_sram;
}

uint8_t* smsb_cart_ram(uint32_t* size) {
    if (size) *size = s_sram ? 0x8000 : 0;
    return s_sram;
}

// ─── Estado salvo (a "foto" do jogo) ────────────────────────────────────────
struct StateHdr { char magic[8]; uint32_t total, romSize; uint8_t sys, pad[3]; };

static uint32_t state_total() {
    uint32_t t = 0; unsigned len; void* q;
    for (int i = 0; (q = latinha_sms_piece(i, &len)); i++) t += len;
    return t;
}

bool smsb_save_state(const char* rom_path) {
    char sp[MAX_PATHLEN];
    sd_get_state_path(rom_path, sp, sizeof(sp));
    if (SD.exists(sp)) SD.remove(sp);
    File f = SD.open(sp, FILE_WRITE);
    if (!f) return false;
    StateHdr h; memset(&h, 0, sizeof(h));
    memcpy(h.magic, "CYDSMS1", 8);
    h.total = state_total(); h.romSize = s_romSize; h.sys = s_sys;
    bool ok = f.write((const uint8_t*)&h, sizeof(h)) == sizeof(h);
    unsigned len; void* q;
    for (int i = 0; ok && (q = latinha_sms_piece(i, &len)); i++) ok = f.write((const uint8_t*)q, len) == len;
    f.close();
    Serial.printf("[SMS] Estado %s: %s (%u bytes)\n", ok ? "salvo" : "FALHOU", sp, (unsigned)h.total);
    return ok;
}

bool smsb_load_state(const char* rom_path) {
    char sp[MAX_PATHLEN];
    sd_get_state_path(rom_path, sp, sizeof(sp));
    File f = SD.open(sp, FILE_READ);
    if (!f) return false;
    StateHdr h;
    if (f.read((uint8_t*)&h, sizeof(h)) != sizeof(h) || memcmp(h.magic, "CYDSMS1", 8) ||
        h.total != state_total() || h.romSize != s_romSize || h.sys != s_sys) { f.close(); return false; }
    latinha_sms_before_load();
    unsigned len; void* q; bool ok = true;
    for (int i = 0; ok && (q = latinha_sms_piece(i, &len)); i++) ok = f.read((uint8_t*)q, len) == len;
    latinha_sms_after_load();
    f.close();
    return ok;
}
#endif
