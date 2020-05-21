#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora.h"
#include "transport_lora.h"
#include "esp_log.h"
#include "esp_err.h"
#include "assert.h"

static const char* TAG = "TRANSPORT_LORA";

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
   lora_set_frequency(868e6);
   lora_enable_crc();
   lora_set_spreading_factor(6);
   lora_implicit_header_mode(12);
   lora_set_tx_power(17);
   lora_set_bandwidth(250E3);
   lora_set_coding_rate(5);

   ESP_LOGI(TAG, "Init successful");
}

void transport_lora_send(uint8_t* data, uint16_t length)
{
   if (lora_modem_detected) {
      lora_send_packet(data, length);
   }
}