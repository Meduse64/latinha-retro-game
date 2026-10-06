#pragma once
#include <stddef.h>
#include <stdint.h>
// App Onibus: proximas saidas da parada Pontal (VIPBUS), com horarios fixos (os mesmos do projeto Meteo).
void app_onibus_run();
// Dados do bloco da tela de inicio: big = linha que sai primeiro ("Metro" ou "S.All"), small = "em 8 min"; false sem hora
bool onibus_tile(char* big, size_t nb, char* small, size_t ns, uint16_t* color);
