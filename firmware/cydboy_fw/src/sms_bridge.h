#pragma once
// Ponte para o SMS Plus (Game Gear e Master System). So existe se a pasta src/smsplus existir
// (HAS_SMS em hw_config.h). A ROM do cartao SD e copiada para a particao de dados da flash
// e mapeada na memoria, porque o SMS Plus le a ROM como um bloco de memoria.
#include <stdint.h>
#include <stdbool.h>
#include "hw_config.h"

#define SYS_GB  0
#define SYS_GG  1
#define SYS_SMS 2

#if HAS_SMS
bool smsb_open(const char* path, uint8_t sys);   // copia a ROM para a flash (se preciso) e mapeia
bool smsb_start();                               // inicia o SMS Plus
void smsb_close();
void smsb_run_frame(uint8_t jpad, bool draw);    // jpad: bits do CYDboy (1 = apertado)
void smsb_reset();
uint8_t* smsb_cart_ram(uint32_t* size);          // RAM de bateria (so existe se o jogo ligar)
bool smsb_save_state(const char* rom_path);
bool smsb_load_state(const char* rom_path);
const char* smsb_error();
#endif
