#pragma once
// Tela de inicio: quatro blocos (Retro, Tempo, Onibus, Agenda). Toque no bloco, ou D-pad e A.
#define HOME_RETRO   0
#define HOME_TEMPO   1
#define HOME_ONIBUS  2
#define HOME_AGENDA  3

int home_show();   // devolve o app escolhido (HOME_*)
