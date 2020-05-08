#pragma once

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"

typedef struct controller_sample_t {
    uint32_t        raw_value;
    uint32_t        voltage;
} controller_sample_t;

typedef enum inputs {
    INPUT_LEFT_JOYSTICK_X,
    INPUT_LEFT_JOYSTICK_Y,
    INPUT_LEFT_JOYSTICK_ROTATE,
    INPUT_RIGHT_JOYSTICK_X,
    INPUT_RIGHT_JOYSTICK_Y,
    INPUT_RIGHT_JOYSTICK_ROTATE,
    INPUT_ANALOG_END,
    INPUT_POT_LEFT = INPUT_ANALOG_END,
    INPUT_POT_RIGHT,
    INPUT_ANALOG_I2C_END,
    INPUT_SWITCH_1_UP = INPUT_ANALOG_I2C_END,
    INPUT_SWITCH_1_DOWN,
    INPUT_SWITCH_2_UP,
    INPUT_SWITCH_2_DOWN,
    INPUT_SWITCH_3_UP,
    INPUT_SWITCH_3_DOWN,
    INPUT_SWITCH_4_UP,
    INPUT_SWITCH_4_DOWN,
    INPUT_SWITCH_5_UP,
    INPUT_SWITCH_5_DOWN,
    INPUTS_END
} inputs;

typedef void samples_callback(controller_sample_t* samples, uint8_t num_samples);

void controller_input_init(uint16_t time_between_samples_ms, samples_callback* callback);
uint32_t controller_input_get_map(uint8_t id, uint32_t min, uint32_t max);

