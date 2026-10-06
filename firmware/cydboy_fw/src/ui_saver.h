#pragma once
// Protetor de tela (olhos que se mexem) e deep sleep.
//   - 5 min sem usar (nos menus e apps): aparecem os olhos.
//   - 10 min sem usar: a placa dorme (deep sleep) e acorda com um toque na tela.
//   - Toque no titulo "Latinha" da tela inicial: olhos na hora, ate alguem tocar (nao dorme).
// Nos jogos nao entra (o jogo pode estar rodando sem apertar nada).
#include <Arduino.h>

void saver_activity();          // chame quando houver botao ou toque (zera o relogio)
bool saver_due();               // passou o tempo de ociosidade dos olhos?
void saver_run(bool manual);    // mostra os olhos; volta quando alguem aperta algo (ou nunca, se dormir)
