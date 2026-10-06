// Substitui o bt_controller.cpp do CYDboy original, que depende do Bluepad32.
// Esta versao nao usa Bluetooth: libera cerca de 120 KB de RAM para o cache da ROM.
#include "bt_controller.h"

void bt_controller_init() {}
void bt_controller_update() {}
uint16_t bt_controller_get_buttons() { return 0; }
bool bt_controller_is_connected() { return false; }
const char* bt_controller_get_name() { return ""; }
void bt_controller_ui_show() {}
