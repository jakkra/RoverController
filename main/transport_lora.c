#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lora.h"
#include "transport_lora.h"
#include "rover_telematics.h"
#include "esp_log.h"
#include "esp_err.h"
#include "assert.h"
#include "driver/gpio.h"

#define DIO0_PIN GPIO_NUM_26

#define LORA_DIO0_HIGH 2

#define MAX_LORA_PAYLOAD 100

static const char* TAG = "TRANSPORT_LORA";

static void IRAM_ATTR gpio_dio0_isr_handler(void* arg);
static void lora_receive_task(void* arg);


static TaskHandle_t lora_receive_task_handle;
static xSemaphoreHandle lora_sem;
static bool lora_modem_detected = false;

void transport_lora_init(void)
{
   if (lora_init() == ESP_OK) {
      lora_modem_detected = true;
   } else {
      lora_modem_detected = false;
      ESP_LOGE(TAG, "No LoRa modem detected");
      return;
   }

   lora_sem = xSemaphoreCreateBinary();
   assert(lora_sem != NULL);
   
   lora_set_frequency(868e6);
   lora_enable_crc();
   lora_set_spreading_factor(7);
   lora_set_tx_power(17);
   lora_set_bandwidth(500E3);
   lora_set_coding_rate(5);

   gpio_config_t io_conf;
   io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
   io_conf.pin_bit_mask = 1ULL << DIO0_PIN;
   io_conf.mode = GPIO_MODE_INPUT;
   io_conf.pull_down_en = 0;
   io_conf.pull_up_en = 0;
   gpio_config(&io_conf);

   gpio_install_isr_service(0);

   BaseType_t status = xTaskCreate(lora_receive_task, "lora_receive_task", 2048, NULL, 2, &lora_receive_task_handle);
   assert(status == pdPASS);

   gpio_isr_handler_add(DIO0_PIN, gpio_dio0_isr_handler, (void*) DIO0_PIN);
   lora_receive();
   xSemaphoreGive(lora_sem);

   ESP_LOGI(TAG, "Init successful");
}

void transport_lora_send(uint8_t* data, uint16_t length)
{
   if (lora_modem_detected) {
      assert(xSemaphoreTake(lora_sem, pdMS_TO_TICKS(50)) == pdTRUE);
      gpio_isr_handler_remove(DIO0_PIN);
      lora_send_packet(data, length);
      gpio_isr_handler_add(DIO0_PIN, gpio_dio0_isr_handler, (void*) DIO0_PIN);
      lora_receive();
      xSemaphoreGive(lora_sem);
   }
}

static void lora_receive_task(void* arg)
{
    while (true) {
         uint8_t buf[MAX_LORA_PAYLOAD];
         uint32_t notification;

         assert(xTaskNotifyWait(LORA_DIO0_HIGH, 0, &notification, portMAX_DELAY));
         assert(notification == LORA_DIO0_HIGH);
         assert(xSemaphoreTake(lora_sem, pdMS_TO_TICKS(50)) == pdTRUE);
         while (lora_received()) {
            uint32_t x = lora_receive_packet(buf, sizeof(buf));
            if (x == -1) {
               printf("crc err\n");
            } else {
                printf("Received: %d\n", x);
                rover_telematics_put(buf, x);
            }
         }
         gpio_isr_handler_add(DIO0_PIN, gpio_dio0_isr_handler, (void*) DIO0_PIN);
         xSemaphoreGive(lora_sem);
    }
}

static void IRAM_ATTR gpio_dio0_isr_handler(void* arg)
{
   gpio_isr_handler_remove(DIO0_PIN);
   assert(xTaskNotifyFromISR(lora_receive_task_handle, LORA_DIO0_HIGH, eSetValueWithOverwrite, NULL) == pdPASS);
}
