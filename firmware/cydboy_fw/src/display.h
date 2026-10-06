#pragma once
#include <TFT_eSPI.h>
extern TFT_eSPI tft;

void display_init();
void display_set_backlight(uint8_t level);
void display_clear(uint16_t color = TFT_BLACK);
void display_push_gb_line(uint8_t line_y, const uint8_t* px, const uint16_t* pal, uint8_t mask);
void display_push_gg_line(int line_y, const uint8_t* rgb332);   // Game Gear: 160 pixels em 3-3-2
void display_push_sms_line(int line_y, const uint8_t* rgb332);  // Master System: 256 pixels em 3-3-2
void display_draw_controls();
void display_clear_controls();

// Tamanho da imagem do jogo: 0 Normal 1:1, 1 Ajustado x1,5, 2 Cheia x1,67, 3 Esticada (so deitado)
#define DISPLAY_SIZE_MODES 4
void display_set_size_mode(uint8_t m);
uint8_t display_get_size_mode();
const char* display_size_mode_name(uint8_t m);
