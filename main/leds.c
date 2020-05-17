#include "leds.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "string.h"
#include "driver/gpio.h"
#include "config.h"
#include "esp_log.h"

static const char* TAG = "LEDS";

#define LED_UNACTIVE_LIMIT   500

typedef struct LedState {
    gpio_num_t gpio;
    bool on;
    uint32_t toggle_ms;
} LedState;

static void blink_task(void* args);

static LedState leds[LED_END];
static SemaphoreHandle_t mutex = NULL;

void leds_init(void)
{
    assert(mutex == NULL);
    memset(&leds, 0, sizeof(leds));
    leds[LED_RIGHT].gpio = (gpio_num_t)LED_RIGHT_GPIO;
    leds[LED_LEFT].gpio = (gpio_num_t)LED_LEFT_GPIO;

    gpio_pad_select_gpio(LED_RIGHT_GPIO);
    gpio_pad_select_gpio(LED_LEFT_GPIO);
    gpio_set_direction(LED_RIGHT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_LEFT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_RIGHT_GPIO, 1);
    gpio_set_level(LED_LEFT_GPIO, 1);

    mutex = xSemaphoreCreateMutex();
    assert(mutex != NULL);
    xSemaphoreGive(mutex);

    TaskHandle_t handle;
    BaseType_t status = xTaskCreate(blink_task, "blink_task", 2048, NULL, tskIDLE_PRIORITY, &handle);
    assert(status == pdPASS);
}

void leds_toggle(Led num)
{
    assert(num < LED_END);
    assert(mutex != NULL);
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10))) {
        leds[num].toggle_ms = pdTICKS_TO_MS(xTaskGetTickCount());
        xSemaphoreGive(mutex);
    } else {
        assert(false);
    }
}

static void blink_task(void* args)
{
    while (true) {
        uint32_t ms_now = pdTICKS_TO_MS(xTaskGetTickCount());
        xSemaphoreTake(mutex, portMAX_DELAY);
        for (uint8_t i = 0; i < LED_END; i++) {
            if (ms_now - leds[i].toggle_ms <= LED_UNACTIVE_LIMIT) {
                leds[i].on = !leds[i].on;
                gpio_set_level(leds[i].gpio, leds[i].on);
            } else {
                leds[i].on = false;
                gpio_set_level(leds[i].gpio, 0);
            }
        }
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}