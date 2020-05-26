#pragma once

#include "esp_system.h"

void transport_ws_init(void);
void transport_ws_start(void);
esp_err_t transport_ws_send(uint8_t* buf, uint16_t len);