#pragma once

#include "esp_system.h"

void rover_transport_init(void);
void rover_transport_start(void);
esp_err_t rover_transport_send(uint8_t* buf, uint8_t len);