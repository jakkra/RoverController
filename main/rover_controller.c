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

#define ADC_SAMPLE_DELAY        50
#define MAX_TX_BUF_LEN          100

#define ADC_DATA_NOTIFICATION   1

static void sample_readings_done_callback(controller_sample_t* samples, uint8_t num_samples);
static void periodic_send_data(void* params);
static bool build_rover_payload(void);
static uint16_t convert_3_way_switch(uint32_t raw1, uint32_t raw2);


static const char* TAG = "ROVER_CONTROLLER";

static TaskHandle_t task_handle;
static xSemaphoreHandle sem_handle;
static controller_sample_t controller_samples[INPUTS_END];

static uint16_t tx_buf[MAX_TX_BUF_LEN];
static uint16_t tx_buf_payload_len;

void rover_controller_init(void)
{
    sem_handle = xSemaphoreCreateBinary();
    assert(sem_handle != NULL);
    xSemaphoreGive(sem_handle);
    BaseType_t status = xTaskCreate(periodic_send_data, "send_values", 4096, NULL, tskIDLE_PRIORITY, &task_handle);
    assert(status == pdPASS);

    controller_input_init(ADC_SAMPLE_DELAY, &sample_readings_done_callback);
}

static void periodic_send_data(void* params)
{
    while (true)
    {
        uint32_t notification;
        assert(xTaskNotifyWait(ADC_DATA_NOTIFICATION, 0, &notification, portMAX_DELAY));
        assert(notification == ADC_DATA_NOTIFICATION);
        assert(xSemaphoreTake(sem_handle, portMAX_DELAY) == pdPASS);
        for (uint8_t i = 0; i < INPUTS_END; i++) {
            //printf("%d: Raw: %d, %dmV  ", i, controller_samples[i].raw_value, controller_samples[i].voltage);
        }
        //printf("\n");
        bool payload_changed = build_rover_payload();
        xSemaphoreGive(sem_handle);
        if (payload_changed) {
            rover_transport_send((uint8_t*)tx_buf, tx_buf_payload_len);
        }
    }
}

static bool build_rover_payload(void)
{
    bool payload_changed = true;
    uint16_t temp_tx_buf[MAX_TX_BUF_LEN];

    tx_buf_payload_len = 6 * sizeof(uint16_t); // Rover RC controller have 6 channels, limit to that for now for compatability

    temp_tx_buf[0] = map(controller_samples[INPUT_RIGHT_JOYSTICK_X].voltage, 0, 3300, 1000, 2000);
    temp_tx_buf[1] = map(controller_samples[INPUT_RIGHT_JOYSTICK_Y].voltage, 0, 3300, 1000, 2000);
    temp_tx_buf[2] = map(controller_samples[INPUT_LEFT_JOYSTICK_X].voltage, 0, 3300, 1000, 2000);
    temp_tx_buf[3] = map(controller_samples[INPUT_LEFT_JOYSTICK_Y].voltage, 0, 3300, 1000, 2000);
    temp_tx_buf[4] = convert_3_way_switch(controller_samples[INPUT_SWITCH_1_UP].raw_value, controller_samples[INPUT_SWITCH_1_DOWN].raw_value);
    temp_tx_buf[5] = 1500;

    //TODO("Could do more here, like only send if something changed more than x")
    if (memcmp(temp_tx_buf, tx_buf, tx_buf_payload_len) == 0) {
        payload_changed = false;
    } else {
        memcpy(tx_buf, temp_tx_buf, tx_buf_payload_len);
    }
    //ESP_LOGW(TAG, "Send: %d, %d \t %d, %d \t %d, %d\n", tx_buf[0], tx_buf[1], tx_buf[2],  tx_buf[3], tx_buf[4], tx_buf[5]);
    return payload_changed;
}

static void sample_readings_done_callback(controller_sample_t* samples, uint8_t num_samples)
{
    assert(num_samples == INPUTS_END);
    if (xSemaphoreTake(sem_handle, pdMS_TO_TICKS(5)) == pdPASS) {
        memcpy(controller_samples, samples, sizeof(controller_sample_t) * num_samples);
        xSemaphoreGive(sem_handle);
    } else {
        ESP_LOGE(TAG, "Failed getting adc sample copy semaphore");
    }
    
    assert(xTaskNotify(task_handle, ADC_DATA_NOTIFICATION, eSetValueWithOverwrite) == pdPASS);
}

static uint16_t convert_3_way_switch(uint32_t up, uint32_t down)
{
    uint16_t value;

    if (up == 0 && down == 0) {
        value = 1500;
    } else if (up == 1) {
        value = 1000;
    } else {
        value = 2000;
    }

    return value;
} 
