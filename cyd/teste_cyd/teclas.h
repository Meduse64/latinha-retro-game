#pragma once
// Tipos das teclas. Ficam num arquivo a parte porque a Arduino IDE insere os prototipos das
// funcoes logo depois dos #include, antes de qualquer definicao do .ino: se estes tipos
// estivessem no .ino, funcoes como readRaw() e pollKey() nao os enxergariam.
#include <Arduino.h>

enum Key : uint8_t { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK, KEY_BACK, KEY_A, KEY_B, KEY_HOME, KEY_COUNT };
enum BtnEvt : uint8_t { EV_NONE, EV_DOWN, EV_CLICK, EV_LONG, EV_REPEAT };

struct KeyState { bool down, longFired; uint32_t downAt, lastRep, lastChange; };
