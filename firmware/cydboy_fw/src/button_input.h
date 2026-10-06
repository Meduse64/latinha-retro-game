#pragma once
#include <stdint.h>

void button_init();
void button_update();
uint16_t button_get_buttons();
bool button_pcf_detected();   // true se ha botoes fisicos no PCF8574
