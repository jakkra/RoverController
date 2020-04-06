
#include "controller_input.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "soc/adc_channel.h"
#include "rover_utils.h"

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64


static void print_char_val_type(esp_adc_cal_value_t val_type);
static void adc_sampleTask(void* params);
static adc_atten_t attenuation = ADC_ATTEN_DB_11;
static adc_bits_width_t width = ADC_WIDTH_BIT_10;
static adc_sample_t* adc_samples;
static esp_adc_cal_characteristics_t *adc_chars;
static uint8_t num_channels;
static uint16_t sleep_time;
static adc_callback* on_sample_done_callback;

void controller_input_init(adc1_channel_t* channels, uint8_t num, uint16_t time_between_samples_ms, adc_callback* callback)
{
    assert(callback != NULL);
    on_sample_done_callback = callback;
    num_channels = num;
    sleep_time = time_between_samples_ms;
    adc_samples = calloc(num_channels, sizeof(adc_sample_t));
    adc1_config_width(width);
    for (uint8_t i = 0; i < num_channels; i++) {
        adc1_config_channel_atten(channels[i] , attenuation);
        adc_samples[i].channel = channels[i];
    }

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, attenuation, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    TaskHandle_t handle;
    BaseType_t status = xTaskCreate(adc_sampleTask, "try_connectadc_sampleTask_rover_websocket", 2048, NULL, tskIDLE_PRIORITY, &handle);
    assert(status == pdPASS);
}

uint32_t controller_input_get_map(uint8_t id, uint32_t min, uint32_t max)
{
    assert(id < num_channels);
    uint32_t max_reading;
    switch (width) {
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

    return map(adc_samples[id].raw_value, 0, max_reading, min, max);
}

static void adc_sampleTask(void* params)
{
    while (true)
    {
        for (uint8_t i = 0; i < num_channels; i++) {
            uint32_t adc_reading = 0;
            for (int sample = 0; sample < NO_OF_SAMPLES; sample++) {
                adc_reading += (uint32_t)adc1_get_raw(adc_samples[i].channel);
            }
            adc_reading /= NO_OF_SAMPLES;
            adc_samples[i].raw_value = adc_reading;
            adc_samples[i].voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        }

        on_sample_done_callback(adc_samples, num_channels);
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