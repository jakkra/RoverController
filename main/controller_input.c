#include <string.h>
#include "controller_input.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"
#include "rover_utils.h"
#include "esp_log.h"
#include "ads1115.h"

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   32

#define ATTENUATION     ADC_ATTEN_DB_11
#define ADC_WIDTH       ADC_WIDTH_BIT_10

#define I2C_NUM         I2C_NUM_0

#define LAST_SWITCH     INPUT_SWITCH_3_UP


static uint16_t rover_channel_map[INPUTS_END] = {
    INPUT_LEFT_JOYSTICK_X,
    INPUT_LEFT_JOYSTICK_Y,
    INPUT_LEFT_JOYSTICK_ROTATE,
    INPUT_RIGHT_JOYSTICK_X,
    INPUT_RIGHT_JOYSTICK_Y,
    INPUT_RIGHT_JOYSTICK_ROTATE,
    INPUT_POT_LEFT,
    INPUT_POT_RIGHT,
    INPUT_SWITCH_1_UP,
    INPUT_SWITCH_1_DOWN,
    INPUT_SWITCH_2_UP,
    INPUT_SWITCH_2_DOWN,
    INPUT_SWITCH_3_UP,
    INPUT_SWITCH_3_DOWN,
    INPUT_SWITCH_4_UP,
    INPUT_SWITCH_4_DOWN,
    INPUT_SWITCH_5_UP,
    INPUT_SWITCH_5_DOWN,
};

// Mapping of all inputs to GPIO/ADC num
static uint16_t rover_pin_map[INPUTS_END] = {
    [INPUT_LEFT_JOYSTICK_X] = ADC1_GPIO39_CHANNEL,
    [INPUT_LEFT_JOYSTICK_Y] = ADC1_GPIO36_CHANNEL,
    [INPUT_LEFT_JOYSTICK_ROTATE] = ADC1_GPIO33_CHANNEL,
    [INPUT_RIGHT_JOYSTICK_X] = ADC1_GPIO34_CHANNEL,
    [INPUT_RIGHT_JOYSTICK_Y] = ADC1_GPIO35_CHANNEL,
    [INPUT_RIGHT_JOYSTICK_ROTATE] = ADC1_GPIO32_CHANNEL,
    [INPUT_POT_LEFT] = ADS1115_MUX_0_GND,
    [INPUT_POT_RIGHT] = ADS1115_MUX_1_GND,
    [INPUT_SWITCH_1_UP] = GPIO_NUM_25,
    [INPUT_SWITCH_1_DOWN] = GPIO_NUM_13,
    [INPUT_SWITCH_2_UP] = GPIO_NUM_17,
    [INPUT_SWITCH_2_DOWN] = GPIO_NUM_21,
    [INPUT_SWITCH_3_UP] = GPIO_NUM_22,
    [INPUT_SWITCH_3_DOWN] = GPIO_NUM_23,
    [INPUT_SWITCH_4_UP] = GPIO_NUM_15,
    [INPUT_SWITCH_4_DOWN] = GPIO_NUM_2,
    [INPUT_SWITCH_5_UP] = GPIO_NUM_4,
    [INPUT_SWITCH_5_DOWN] = GPIO_NUM_12, // Note: ESP32 won't start if pulled high at boot
};


static void print_char_val_type(esp_adc_cal_value_t val_type);
static void sample_task(void* params);

static controller_sample_t samples[INPUTS_END];
static esp_adc_cal_characteristics_t* adc_chars;
static uint16_t sleep_time;
static samples_callback* on_sample_done_callback;
static ads1115_t ads;

void controller_input_init(uint16_t time_between_samples_ms, samples_callback* callback)
{
    assert(callback != NULL);
    memset(samples, 0, sizeof(samples));

    on_sample_done_callback = callback;
    sleep_time = time_between_samples_ms;

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ATTENUATION, ADC_WIDTH, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    adc1_config_width(ADC_WIDTH);
    for (uint8_t i = 0; i < INPUT_ANALOG_END; i++) {
        adc1_config_channel_atten((adc1_channel_t)rover_pin_map[i], ATTENUATION);
    }

    for (uint8_t i = INPUT_ANALOG_I2C_END; i < LAST_SWITCH; i++) {
        gpio_pad_select_gpio((gpio_num_t)rover_pin_map[i]);
        gpio_set_direction((gpio_num_t)rover_pin_map[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)rover_pin_map[i], GPIO_PULLDOWN_ONLY);
    }

    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0));
    ESP_ERROR_CHECK(i2c_set_pin(I2C_NUM, ROVER_CONTROLLER_SDA, ROVER_CONTROLLER_SCL, true, true, I2C_MODE_MASTER));
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = GPIO_NUM_23,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &conf));

    ads = ads1115_config(I2C_NUM, 0x48);
    ads1115_set_sps(&ads, ADS1115_SPS_860);

    TaskHandle_t handle;
    BaseType_t status = xTaskCreate(sample_task, "gpio_sample_task", 4096, NULL, tskIDLE_PRIORITY, &handle);
    assert(status == pdPASS);
}

uint32_t controller_input_get_map(uint8_t id, uint32_t min, uint32_t max)
{
    assert(id < INPUT_ANALOG_END);
    uint32_t max_reading;
    switch (ADC_WIDTH) {
        case ADC_WIDTH_BIT_9:
            max_reading = 512;
            break;
        case ADC_WIDTH_BIT_10: 
            max_reading = 1024;
            break;
        case ADC_WIDTH_BIT_11:
            max_reading = 2048;
            break;
        case ADC_WIDTH_BIT_12:
            max_reading = 4096;
            break;
        default:
            assert(false);

    }

    return map(samples[id].raw_value, 0, max_reading, min, max);
}

static void sample_task(void* params)
{
    while (true) {
        for (uint8_t i = 0; i < INPUT_ANALOG_END; i++) {
            uint32_t adc_reading = 0;
            for (int sample = 0; sample < NO_OF_SAMPLES; sample++) {
                adc_reading += (uint32_t)adc1_get_raw((adc1_channel_t)rover_pin_map[i]);
            }
            adc_reading /= NO_OF_SAMPLES;
            samples[i].raw_value = adc_reading;
            samples[i].voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        }

        for (uint8_t i = INPUT_ANALOG_END; i < INPUT_ANALOG_I2C_END; i++) {
            // Not used right now so skip
            //ads1115_set_mux(&ads, rover_pin_map[i]);
            //samples[i].raw_value = ads1115_get_raw(&ads);
            //samples[i].voltage = ads1115_get_voltage_from_raw(&ads, samples[i].raw_value) * 1000;
        }

        for (uint8_t i = INPUT_ANALOG_I2C_END; i < LAST_SWITCH; i++) {
            samples[i].raw_value = gpio_get_level((gpio_num_t)rover_pin_map[i]);
            samples[i].voltage = samples[i].raw_value == 0 ? 0 : 3300;
        }

        on_sample_done_callback(samples, INPUTS_END);
        vTaskDelay(pdMS_TO_TICKS(sleep_time));
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}