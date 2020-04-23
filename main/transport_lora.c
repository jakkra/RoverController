#include <stdio.h>
#include "lora.h"
#include "transport_lora.h"
#include "esp_log.h"

static const char* TAG = "TRANSPORT_WS";

void transport_lora_init(void)
{
   lora_init();
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
   lora_send_packet(data, length);
}