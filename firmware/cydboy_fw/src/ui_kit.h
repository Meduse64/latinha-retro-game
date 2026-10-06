#pragma once
// Pecas de tela comuns aos apps (inicio, Tempo, Onibus, Agenda): cores, cabecalho, rodape,
// leitura dos botoes e do toque. Tudo para a tela deitada, 320x240.
#include <Arduino.h>
#include "display.h"
#include "hw_config.h"
#include "touch_input.h"

#define K_BG      0x0000
#define K_HEADER  0x08A4
#define K_CYAN    0x07FF
#define K_TEAL    0x0698
#define K_BLUE    0x24BD
#define K_LBLUE   0x3DFE
#define K_CARD    0x10E5
#define K_CARDSEL 0x1969
#define K_BORDER  0x21AA
#define K_WHITE   0xFFFF
#define K_ICE     0xDEFB
#define K_RED     0xF9A6
#define K_GREEN   0x262B
#define K_AMBER   0xFDA0
#define K_ORANGE  0xFC60
#define K_YELLOW  0xFFE0
#define K_GRAY    0x7BEF
#define K_DGRAY   0x2945

#define UI_HEADER_H 30
#define UI_FOOTER_H 22

struct UiIn {
    uint8_t b;          // botoes apertados agora (bits GB_BTN_*)
    uint8_t pressed;    // botoes que acabaram de ser apertados
    bool tap;           // toque novo nesta chamada
    bool held;          // dedo na tela
    int16_t tx, ty;     // posicao do toque (vale quando tap ou held)
};

void ui_poll(UiIn& in);                 // le botoes e toque; chame uma vez por volta do laco
void ui_wait_release();                 // espera soltar botoes e tela
void ui_header(const char* title, const char* right = nullptr, bool back = true);
void ui_footer(const char* hint);
void ui_text(int x, int y, const char* s, uint16_t fg, uint16_t bg, uint8_t font = 2, uint8_t datum = TL_DATUM);
void ui_msg(const char* l1, const char* l2 = nullptr, uint16_t color = K_AMBER);   // caixa de aviso no meio
bool ui_hit(const UiIn& in, int x, int y, int w, int h);                            // tap dentro do retangulo
void ui_strip_accents(char* s);         // UTF-8 -> ASCII (a fonte da tela nao tem acentos)
