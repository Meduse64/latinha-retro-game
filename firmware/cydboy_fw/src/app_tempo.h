#pragma once
#include <stddef.h>
#include <stdint.h>
// App Tempo: previsao do Rio de Janeiro pelo Open-Meteo (gratis, sem cadastro), em 3 abas.
void app_tempo_run();
// Dados do bloco da tela de inicio: big = "24" (temperatura), small = "Poucas nuvens"; false se ainda sem dados
bool tempo_tile(char* big, size_t nb, char* small, size_t ns, uint16_t* color);
void tempo_icon(int x, int y);   // icone do tempo de agora (48 px); sol com nuvem se ainda sem dados
