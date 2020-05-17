#include "web_server.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <esp_system.h>
#include <esp_event.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "esp_https_server.h"

#include "esp_timer.h"

#include "rover_telematics.h"
#include "leds.h"

#include "config.h"

// Browsers don't like unsecure websockets
//#define ROVER_WS_SSL

#define MAX_WS_INCOMING_SIZE    100
#define WS_CONNECT_MESSAGE      "CONNECT"
#define MAX_WS_CONNECTIONS      2
#define INVALID_FD              -1
#define TX_BUF_SIZE             100

typedef struct ws_client {
    int     fd;
    bool    tx_in_progress;
} ws_client;

typedef struct web_server {
    httpd_handle_t      handle;
    bool                running;
    ws_client           clients[MAX_WS_CONNECTIONS];
    xSemaphoreHandle    tx_sem;
    uint8_t             tx_buf[TX_BUF_SIZE];
    uint16_t            tx_buf_len;
} web_server;


static ws_client* find_ws_client(int fd);
static void on_client_disconnect(httpd_handle_t hd, int sockfd);
static bool any_client_tx();
static bool any_client_connected();

static esp_err_t ws_handler(httpd_req_t *req);
static void on_telematics_data(uint8_t* telemetics, uint16_t length);

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
    ESP_LOGI(TAG, "webserver_init");
    server.handle = NULL;

    for (uint8_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
        server.clients[i].fd = INVALID_FD;
        server.clients[i].tx_in_progress = false;
    }

    server.tx_sem = xSemaphoreCreateBinary();
    assert(server.tx_sem != NULL);
    xSemaphoreGive(server.tx_sem);
    

    rover_telematics_register_on_data(&on_telematics_data);
}

void webserver_start(void)
{
    assert(!server.running);
    ESP_LOGI(TAG, "webserver_start");
    esp_err_t err;

#ifdef ROVER_WS_SSL
    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();

    extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
    extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
    config.cacert_pem = cacert_pem_start;
    config.cacert_len = cacert_pem_end - cacert_pem_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    config.prvtkey_pem = prvtkey_pem_start;
    config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    config.httpd.server_port = WS_SERVER_PORT;
    config.httpd.close_fn = on_client_disconnect;
    config.httpd.max_open_sockets = MAX_WS_CONNECTIONS;

    err = httpd_ssl_start(&server.handle, &config);
    assert(err == ESP_OK);

    err = httpd_register_uri_handler(server.handle, &ws);
    assert(err == ESP_OK);
    server.running = true;
    ESP_LOGI(TAG, "Web Server started on port %d, server handle %p", config.httpd.server_port, server.handle);
#else
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = WS_SERVER_PORT;
    config.close_fn = on_client_disconnect;
    config.max_open_sockets = MAX_WS_CONNECTIONS;
    err = httpd_start(&server.handle, &config);
    assert(err == ESP_OK);

    err = httpd_register_uri_handler(server.handle, &ws);
    assert(err == ESP_OK);
    server.running = true;
    ESP_LOGI(TAG, "Web Server started on port %d, server handle %p", config.server_port, server.handle);
#endif

    
}

void webserver_stop(void)
{
    assert(false);
    // Stop the httpd server
    assert(httpd_stop(server.handle) == ESP_OK);
    server.running = false;
}

static ws_client* find_ws_client(int fd)
{
    uint8_t i = 0;
    for (i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (server.clients[i].fd == fd) {
            return &server.clients[i];
        }
    }
    return NULL;
}

static void on_client_disconnect(httpd_handle_t hd, int sockfd)
{
    ESP_LOGI(TAG, "Client disconnected");
    
    ws_client* client = find_ws_client(sockfd);
    if (client == NULL) return;
    
    bool tx_ongoing = any_client_tx();

    client->tx_in_progress = false;
    client->fd = INVALID_FD;
    if (tx_ongoing && !any_client_tx()) {
        xSemaphoreGive(server.tx_sem);
    }
}

static bool any_client_tx()
{
    uint8_t i = 0;
    for (i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (server.clients[i].fd != INVALID_FD && server.clients[i].tx_in_progress) {
            return true;
        }
    }
    return false;
}

static bool any_client_connected()
{
    uint8_t i = 0;
    for (i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (server.clients[i].fd != INVALID_FD) {
            return true;
        }
    }
    return false;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    ESP_LOGW(TAG, "server: %p, req: %p", server.handle, req->handle);
    assert(server.handle == req->handle);
    uint8_t buf[MAX_WS_INCOMING_SIZE] = { 0 };
    httpd_ws_frame_t packet;
    
    memset(&packet, 0, sizeof(httpd_ws_frame_t));
    packet.payload = buf;

    esp_err_t ret = httpd_ws_recv_frame(req, &packet, MAX_WS_INCOMING_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }
    
    // Currently ESP-IDF WS Server library does not seem to have any "WS_CONNECTED" event, and therefore no way for the server to send
    // any data to the client until the client first sent something to the server. This is where we make a copy of the ws fd so we can
    // send async data to it later.
    if (packet.type == HTTPD_WS_TYPE_TEXT) {
        if (packet.len == strlen(WS_CONNECT_MESSAGE) && strncmp((char*)packet.payload, WS_CONNECT_MESSAGE, packet.len) == 0) {
            ESP_LOGI(TAG, "Got CONNECT");
            uint8_t i = 0;
            for (i = 0; i < MAX_WS_CONNECTIONS; i++) {
                if (server.clients[i].fd == INVALID_FD) {
                    server.clients[i].fd =  httpd_req_to_sockfd(req);
                    server.clients[i].tx_in_progress = false;
                    break;
                }
            }
            assert(i < MAX_WS_CONNECTIONS);
        }
    }
   
    return ESP_OK;
}

static void ws_async_send(void *arg)
{
    esp_err_t err;
    httpd_ws_frame_t packet;
    ws_client* client = (ws_client*)arg;

    memset(&packet, 0, sizeof(httpd_ws_frame_t));
    packet.payload = server.tx_buf;
    packet.len = server.tx_buf_len;
    packet.type = HTTPD_WS_TYPE_BINARY;
    packet.final = true;

    err = httpd_ws_send_frame_async(server.handle, client->fd, &packet);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "httpd_ws_send_frame_async failed: %d", err);
    }
    client->tx_in_progress = false;
    if (!any_client_tx()) {
        xSemaphoreGive(server.tx_sem);
    }
}

static void on_telematics_data(uint8_t* telemetics, uint16_t length)
{
    assert(length <= TX_BUF_SIZE);
        
    if (!any_client_connected()) {
        return;
    }
    
    if (xSemaphoreTake(server.tx_sem, 0) == pdPASS) {
        memcpy(server.tx_buf, telemetics, length);
        server.tx_buf_len = length;
        for (uint8_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
            if (server.clients[i].fd != INVALID_FD) {
                server.clients[i].tx_in_progress = true;
                esp_err_t err;
                err = httpd_queue_work(server.handle, ws_async_send, &server.clients[i]);
                if (err != ESP_OK) {
                    server.clients[i].tx_in_progress = false;
                }
            }
        }
        if (!any_client_tx()) {
            ESP_LOGE(TAG, "Failed queueing data transmission on all websockets, releasing semaphore");
            xSemaphoreGive(server.tx_sem);
        }
    }

}
