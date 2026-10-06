#pragma once
#include <stddef.h>
// App Agenda: proximos eventos do Google Calendar, pelo "endereco secreto no formato iCal"
// (o mesmo do projeto Meteo). Entende eventos repetidos (RRULE).
void app_agenda_run();
// Dados do bloco da tela de inicio: big = data do proximo evento ("10 Nov"), small = "14:30 Fisio"; false sem dados
bool agenda_tile(char* big, size_t nb, char* small, size_t ns);
