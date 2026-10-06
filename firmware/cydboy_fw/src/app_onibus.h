#pragma once
#include <stddef.h>
// App Onibus: proximas saidas da parada Pontal (VIPBUS), com horarios fixos (os mesmos do projeto Meteo).
void app_onibus_run();
bool onibus_summary(char* out, size_t n);   // "Metro em 8 min" para a tela de inicio; false sem hora
