#pragma once
#include <stddef.h>
// App Tempo: previsao do Rio de Janeiro pelo Open-Meteo (gratis, sem cadastro), em 3 abas.
void app_tempo_run();
bool tempo_summary(char* out, size_t n);   // "27 C Nublado" para a tela de inicio; false se ainda sem dados
