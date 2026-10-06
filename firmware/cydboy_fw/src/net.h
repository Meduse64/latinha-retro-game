#pragma once
// Wi-Fi, hora (NTP) e busca HTTPS. O Wi-Fi so fica ligado enquanto um app busca dados:
// nos jogos ele fica desligado, para sobrar RAM e CPU.
#include <Arduino.h>
#include <time.h>

bool net_connect(uint32_t timeout_ms = 15000);   // liga o Wi-Fi e entra na rede de secrets.h
void net_off();                                  // desliga o Wi-Fi
bool net_time_valid();                           // o relogio ja foi acertado?
bool net_sync_time();                            // acerta o relogio por NTP (precisa do Wi-Fi)
bool net_local_time(struct tm* t);               // hora local (fuso do Rio) ou false
bool net_ensure_time();                          // liga o Wi-Fi e acerta a hora se preciso; deixa o Wi-Fi ligado
bool net_http_get(const char* url, String& out); // GET por HTTPS, corpo inteiro em out
