#include "esp_system.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "config.h"

#define WS_TIMEOUT_MS 1500

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static bool check_rover_connected(void);
static void try_connect_rover_websocket(void* params);
static void ws_timed_out(void* arg);



static const char *TAG = "ROVER_TRANSPORT";

static bool rover_connected = false;
static esp_websocket_client_handle_t client;
static TaskHandle_t connect_task_handle = NULL;
static xSemaphoreHandle connect_semaphore;
static esp_timer_handle_t ws_timeout_timer;

void rover_transport_init(void)
{
    connect_semaphore = xSemaphoreCreateBinary();
    assert(connect_semaphore != NULL);
    xSemaphoreGive(connect_semaphore);
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    const esp_timer_create_args_t oneshot_timer_args = {
        .callback = &ws_timed_out,
        .name = "ws_timeout"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &ws_timeout_timer));

}

void rover_transport_start(void)
{
    
}

esp_err_t rover_transport_send(uint8_t buf, uint16_t len)
{
    esp_err_t res = ESP_FAIL;
    if (esp_websocket_client_is_connected(client)) {
        res = (esp_err_t)esp_websocket_client_send_bin(client, (char*)buf, len, pdMS_TO_TICKS(1000));
    }

    return res;
}

static void ws_timed_out(void* arg)
{
    ESP_LOGE(TAG, "WS timeout Rover lost");
    rover_connected = false;
    esp_websocket_client_stop(client);
    esp_websocket_client_destroy(client);
    client = NULL;
}

static void restart_communication_timer(void) {
    esp_timer_stop(ws_timeout_timer);
    ESP_ERROR_CHECK(esp_timer_start_once(ws_timeout_timer, WS_TIMEOUT_MS * 1000));
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGW(TAG, "WEBSOCKET_EVENT_CONNECTED");
        restart_communication_timer();
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        client = NULL;
        break;
    case WEBSOCKET_EVENT_DATA:
        restart_communication_timer();
        //ESP_LOGW(TAG, "WEBSOCKET_EVENT_DATA");
        //ESP_LOGW(TAG, "Received opcode=%d", data->op_code);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGW(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    default: 
        break;
    }
}

static void try_connect_rover_websocket(void* params)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_websocket_client_config_t websocket_cfg = {};
    websocket_cfg.uri = ROVER_WS_URL;

    ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    esp_websocket_client_start(client);
    for (uint16_t i = 0; i < 10; i++) {
        if (esp_websocket_client_is_connected(client)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (!esp_websocket_client_is_connected(client)) {
        ESP_LOGW(TAG, "Not rover");
        esp_websocket_client_stop(client);
        esp_websocket_client_destroy(client);
        client = NULL;
    } else {
        ESP_LOGI(TAG, "Found Rover, ws connected");
        rover_connected = true;
    }
    connect_task_handle = NULL;
    xSemaphoreGive(connect_semaphore);
    vTaskDelete(NULL);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED:
        {
            ESP_LOGW(TAG, "WIFI_EVENT_AP_STACONNECTED");
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            if (!check_rover_connected()) {
                if (xSemaphoreTake(connect_semaphore, pdMS_TO_TICKS(1))) {
                    assert(connect_task_handle == NULL);
                    BaseType_t status = xTaskCreate(try_connect_rover_websocket, "try_connect_rover_websocket", 2048, NULL, tskIDLE_PRIORITY, &connect_task_handle);
                    assert(status == pdPASS);
                    if (status != pdPASS) {
                        xSemaphoreGive(connect_semaphore);
                    }
                } else {
                    ESP_LOGE(TAG, "Failed getting connect semaphore, connect already in progress");
                }
            }
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            ESP_LOGW(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
            break;
        }
    }
}

static bool check_rover_connected(void)
{
    return esp_websocket_client_is_connected(client);
}