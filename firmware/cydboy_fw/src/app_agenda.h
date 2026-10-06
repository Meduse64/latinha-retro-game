#pragma once
#include <stddef.h>
// App Agenda: proximos eventos do Google Calendar, pelo "endereco secreto no formato iCal"
// (o mesmo do projeto Meteo). Entende eventos repetidos (RRULE).
void app_agenda_run();
bool agenda_summary(char* out, size_t n);   // titulo do proximo evento para a tela de inicio; false sem dados
