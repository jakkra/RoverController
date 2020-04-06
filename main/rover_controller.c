#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "rover_controller.h"
#include "controller_input.h"
#include "rover_transport.h"
#include "rover_utils.h"
#include "config.h"

#define NUM_ADC_INPUTS          3
#define ADC_SAMPLE_DELAY        25
#define MAX_TX_BUF_LEN          100

#define ADC_DATA_NOTIFICATION   1

static void adc_readings_done_callback(adc_sample_t* samples, uint8_t num_samples);
static void periodic_send_data(void* params);
static void build_rover_payload(void);


static const char* TAG = "ROVER_CONTROLLER";

static TaskHandle_t task_handle;
static xSemaphoreHandle sem_handle;
static adc_sample_t* adc_samples;

static uint16_t tx_buf[MAX_TX_BUF_LEN];
static uint16_t tx_buf_payload_len;

void rover_controller_init(void)
{
    adc_samples = calloc(NUM_ADC_INPUTS, sizeof(adc_sample_t));
    sem_handle = xSemaphoreCreateBinary();
    assert(sem_handle != NULL);
    xSemaphoreGive(sem_handle);
    BaseType_t status = xTaskCreate(periodic_send_data, "send_values", 4096, NULL, tskIDLE_PRIORITY, &task_handle);
    assert(status == pdPASS);

    controller_input_init((adc1_channel_t[]){ ADC1_GPIO34_CHANNEL, ADC1_GPIO35_CHANNEL, ADC1_GPIO32_CHANNEL }, NUM_ADC_INPUTS, ADC_SAMPLE_DELAY, &adc_readings_done_callback);

}

static void periodic_send_data(void* params)
{
    while (true)
    {
        uint32_t notification;
        assert(xTaskNotifyWait(ADC_DATA_NOTIFICATION, 0, &notification, portMAX_DELAY));
        assert(notification == ADC_DATA_NOTIFICATION);
        assert(xSemaphoreTake(sem_handle, portMAX_DELAY) == pdPASS);
        for (uint8_t i = 0; i < NUM_ADC_INPUTS; i++) {
            //printf("%d: Raw: %d\tVoltage: %dmV\t\t", i, adc_samples[i].raw_value, adc_samples[i].voltage);
        }
        //printf("\n");
        build_rover_payload();
        xSemaphoreGive(sem_handle);
        rover_transport_send((uint8_t*)tx_buf, tx_buf_payload_len);
    }
}

static void build_rover_payload(void)
{
    tx_buf_payload_len = 6 * sizeof(uint16_t); // Rover RC controller have 6 channels, limit to that for now for compatability

    tx_buf[0] = map(adc_samples[0].raw_value, 0, 1023, 1000, 2000);
    tx_buf[1] = 1500;//map(adc_samples[1].raw_value, 0, 1023, 1000, 2000);
    tx_buf[2] = 1500;
    tx_buf[3] = 1500;
    tx_buf[4] = 1000;
    tx_buf[5] = 1500;
    ESP_LOGW(TAG, "Send: %d, %d \t %d, %d \t %d, %d\n", tx_buf[0], tx_buf[1], tx_buf[2],  tx_buf[3], tx_buf[4], tx_buf[5]);
}

static void adc_readings_done_callback(adc_sample_t* samples, uint8_t num_samples)
{
    assert(num_samples == NUM_ADC_INPUTS);
    if (xSemaphoreTake(sem_handle, pdMS_TO_TICKS(1)) == pdPASS) {
        memcpy(adc_samples, samples, sizeof(adc_sample_t) * num_samples);
        xSemaphoreGive(sem_handle);
    } else {
        ESP_LOGE(TAG, "Failed getting adc sample copy semaphore");
    }
    
    assert(xTaskNotify(task_handle, ADC_DATA_NOTIFICATION, eSetValueWithOverwrite) == pdPASS);
}

