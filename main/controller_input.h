#pragma once

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"

typedef struct adc_sample_t {
    adc1_channel_t  channel;
    uint32_t        raw_value;
    uint32_t        voltage;
} adc_sample_t;

typedef enum inputs {
    INPUT_LEFT_JOYSTICK,
    INPUT_RIGHT_JOYSTICK,
    INPUT_LEFT_JOYSTICK_BUTTON,
    INPUT_RIGHT_JOYSTICK_BUTTON,
    INPOUT_SWITCH_ROVER_MODE,
    INPOUT_SWITCH_ARM_MODE,
} inputs;

typedef void adc_callback(adc_sample_t* samples, uint8_t num_samples);

void controller_input_init(adc1_channel_t* channels, uint8_t num, uint16_t time_between_samples_ms, adc_callback* callback);
uint32_t controller_input_get_map(uint8_t id, uint32_t min, uint32_t max);

