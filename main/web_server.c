#include "web_server.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <esp_system.h>
#include <esp_event.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "esp_timer.h"


#include "config.h"

#define MAX_WS_INCOMING_SIZE 100
#define WS_CONNECT_MESSAGE "CONNECT"

static esp_err_t ws_handler(httpd_req_t *req);
static void send_to_ws_client(void* arg);

typedef struct web_server {
    httpd_handle_t  handle;
    bool            running;
    httpd_handle_t  hd;
    int             fd;
} web_server;

static const httpd_uri_t ws = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_handler,
    .user_ctx   = NULL,
    .is_websocket = true
};
static const char *TAG = "web_server";

static web_server server;

void webserver_init(void)
{
    memset(&server, 0, sizeof(web_server));
    server.running = false;
}

void webserver_start(void)
{
    assert(!server.running);
    esp_err_t err;
    server.handle = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = WS_SERVER_PORT;

    err = httpd_start(&server.handle, &config);
    assert(err == ESP_OK);
    err = httpd_register_uri_handler(server.handle, &ws);
    assert(err == ESP_OK);
    server.running = true;
    ESP_LOGI(TAG, "Web Server started on port %d", config.server_port);

    const esp_timer_create_args_t send_to_ws_client_args = {
        .callback = &send_to_ws_client,
        /* name is optional, but may help identify the timer when debugging */
        .name = "periodic"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&send_to_ws_client_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 5000000));
}

void webserver_stop(void)
{
    assert(false);
    // Stop the httpd server
    assert(httpd_stop(server.handle) == ESP_OK);
    server.running = false;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    uint8_t buf[MAX_WS_INCOMING_SIZE] = { 0 };
    httpd_ws_frame_t packet;
    memset(&packet, 0, sizeof(httpd_ws_frame_t));

    ESP_LOGI(TAG, "Method: %d", req->method);
    
    packet.payload = buf;
    packet.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &packet, MAX_WS_INCOMING_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "Packet type: %d", packet.type);
    
    if (packet.type == HTTPD_WS_TYPE_TEXT) {
        ESP_LOGI(TAG, "Got packet with message: %s", packet.payload);
        if (strncmp((char*)packet.payload, WS_CONNECT_MESSAGE, packet.len) == 0) {
            ESP_LOGI(TAG, "Got CONNECT");
            server.hd = req->handle;
            server.fd = httpd_req_to_sockfd(req);
            packet.payload = (uint8_t*)"CONNECTED";
        } else {
            packet.payload = (uint8_t*)"Ignored";
        }
    }
    packet.type = HTTPD_WS_TYPE_TEXT;
    ret = httpd_ws_send_frame(req, &packet);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }
   
    return ESP_OK;
}

static void ws_async_send(void *arg)
{
    static const char * data = "Async data";
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    ESP_LOGW(TAG, "Async Send code: %d", httpd_ws_send_frame_async(server.hd, server.fd, &ws_pkt));
}

static void send_to_ws_client(void* arg) {
    ESP_LOGI(TAG, "Send timer %d", httpd_queue_work(server.hd, ws_async_send, "NULL"));
}
