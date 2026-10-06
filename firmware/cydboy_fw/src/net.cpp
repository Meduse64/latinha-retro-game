#include "net.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#define WIFI_SSID ""
#define WIFI_PASS ""
#endif

#define TZ_RIO "<-03>3"          // Rio de Janeiro: UTC-3, sem horario de verao
static bool s_tzSet = false;
static uint32_t s_lastSync = 0;  // millis() da ultima sincronizacao

static void set_tz() {
    if (s_tzSet) return;
    setenv("TZ", TZ_RIO, 1);
    tzset();
    s_tzSet = true;
}

bool net_connect(uint32_t timeout_ms) {
    if (WiFi.status() == WL_CONNECTED) return true;
    if (!WIFI_SSID[0]) return false;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeout_ms) delay(100);
    Serial.printf("[NET] Wi-Fi %s em %u ms\n", WiFi.status() == WL_CONNECTED ? "conectado" : "FALHOU", (unsigned)(millis() - t0));
    return WiFi.status() == WL_CONNECTED;
}

void net_off() {
    if (WiFi.getMode() == WIFI_OFF) return;
    WiFi.disconnect(true, false);
    WiFi.mode(WIFI_OFF);
}

bool net_time_valid() { return time(nullptr) > 1700000000; }

bool net_sync_time() {
    set_tz();
    configTzTime(TZ_RIO, "pool.ntp.org", "time.google.com");
    uint32_t t0 = millis();
    while (!net_time_valid() && millis() - t0 < 10000) delay(100);
    if (net_time_valid()) s_lastSync = millis() | 1;
    Serial.printf("[NET] NTP %s\n", net_time_valid() ? "ok" : "FALHOU");
    return net_time_valid();
}

bool net_local_time(struct tm* t) {
    set_tz();
    if (!net_time_valid()) return false;
    time_t n = time(nullptr);
    localtime_r(&n, t);
    return true;
}

// O relogio interno deriva uns minutos por dia: reacerta se faz mais de 12 horas.
bool net_ensure_time() {
    bool precisa = !net_time_valid() || s_lastSync == 0 || millis() - s_lastSync > 12UL * 3600UL * 1000UL;
    if (!net_connect()) return net_time_valid();
    if (precisa) net_sync_time();
    return net_time_valid();
}

bool net_http_get(const char* url, String& out) {
    WiFiClientSecure client;
    client.setInsecure();                 // sem checagem do certificado: so le dados publicos
    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(client, url)) return false;
    int code = http.GET();
    if (code != 200) { Serial.printf("[NET] HTTP %d\n", code); http.end(); return false; }
    out = http.getString();
    http.end();
    return true;
}
