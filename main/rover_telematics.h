#pragma once

#include <stdint.h>
#include <stddef.h>

typedef void on_telematics(uint8_t* telematics, uint16_t length);

void rover_telematics_register_on_data(on_telematics* callback);

void rover_telematics_put(uint8_t* telematics, uint16_t length);