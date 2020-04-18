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

#include "rover_telematics.h"

#include "config.h"

#define WS_TIMEOUT_MS 1500

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static bool check_rover_connected(void);
static void try_connect_rover_websocket(void);
static void ws_timed_out(void* arg);
static void async_ws_connect(void);


static const char *TAG = "ROVER_TRANSPORT";

static bool rover_connected = false;
static esp_websocket_client_handle_t client = NULL;
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
    esp_websocket_client_config_t websocket_cfg = {};
    websocket_cfg.uri = ROVER_WS_URL;
    websocket_cfg.disable_auto_reconnect = true;

    ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);

    client = esp_websocket_client_init(&websocket_cfg);
    ESP_ERROR_CHECK(esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client));
}

esp_err_t rover_transport_send(uint8_t* buf, uint16_t len)
{
    esp_err_t res = ESP_FAIL;
    if (esp_websocket_client_is_connected(client) && rover_connected) {
        uint32_t len_sent = esp_websocket_client_send_bin(client, (char*)buf, len, pdMS_TO_TICKS(1000));
        //ESP_LOGW(TAG, "Length sent: %d", len_sent);
        if (len_sent > 0) {
            if (len_sent != len) {
                ESP_LOGE(TAG, "Need logic to handle partial writes");
            }
            res = ESP_OK;
        }
    }

    return res;
}

static void ws_timed_out(void* arg)
{
    ESP_LOGE(TAG, "WS timeout Rover lost");
    rover_connected = false;
    esp_websocket_client_stop(client);
    //esp_websocket_client_destroy(client);
    //client = NULL;
}

static void restart_communication_timer(void) {
    rover_connected = true;
    esp_timer_stop(ws_timeout_timer);
    ESP_ERROR_CHECK(esp_timer_start_once(ws_timeout_timer, WS_TIMEOUT_MS * 1000));
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGW(TAG, "WEBSOCKET_EVENT_CONNECTED");
        xSemaphoreGive(connect_semaphore);
        restart_communication_timer();
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        if (!rover_connected) {
            ESP_LOGW(TAG, "Not Rover");
            xSemaphoreGive(connect_semaphore);
        }

        if (rover_connected) {
            rover_connected = false;
            wifi_sta_list_t sta_list;
            if (esp_wifi_ap_get_sta_list(&sta_list) == ESP_OK) {
                if (sta_list.num > 0) {
                    async_ws_connect();
                }
            }
        };
        break;
    case WEBSOCKET_EVENT_DATA:
        restart_communication_timer();
        if (data->data_len != data->payload_len) {
            ESP_LOGE(TAG, "Need to implement segmented websocket data!");
        } else {
            rover_telematics_put(data->data_ptr, data->data_len);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGW(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    default: 
        break;
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED:
        {
            ESP_LOGW(TAG, "WIFI_EVENT_AP_STACONNECTED");
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            if (!check_rover_connected()) {
                async_ws_connect();
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

static void async_ws_connect(void)
{
    if (xSemaphoreTake(connect_semaphore, 0)) {
        ESP_LOGI(TAG, "Try connect to Rover");
        //ESP_ERROR_CHECK(esp_websocket_client_start(client));
        if (esp_websocket_client_start(client) != ESP_OK) {
            ESP_LOGE(TAG, "esp_websocket_client_start failed");
        }
        
    } else {
        ESP_LOGE(TAG, "Failed getting connect semaphore, connect already in progress");
    }
}

static bool check_rover_connected(void)
{
    return esp_websocket_client_is_connected(client) || rover_connected;
}