#include "esp_mac.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_now.h"

//Use tha same file to flash RX and TX esps.
// find the mac addr (using esptool.py --port PORT flash_id) for the pair of esps to be used and add one to RX_MAC and TX_MAC.
static const uint8_t RX_MAC[ESP_NOW_ETH_ALEN] = {0x24, 0x58, 0x7C, 0xE3, 0x71, 0x44};
static const uint8_t TX_MAC[ESP_NOW_ETH_ALEN] = {0xD8, 0x3B, 0xDA, 0x89, 0x4E, 0xF0};
static const uint8_t BROADCAST_MAC[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const char *TAG = "ESPNOW_UNIFIED";

typedef enum {
    ROLE_UNKNOWN = 0,
    ROLE_TX,
    ROLE_RX
} device_role_t;

static const uint8_t channel_list[] = {1, 6, 13};

static volatile int s_rx_alarm_count = 0;

#define TX_CHANNEL_DURATION     1000    // Each channel in channel_list (1, 6, 13) is active for TX_CHANNEL_DURATION then hops to the next channel.
#define TX_INTER_MSG_GAP        5       // Messages are sent every TX_INTER_MSG_GAP while on the channel .
#define RX_CHANNEL_DURATION     20      // Duration for each channel scan period in RX hopping 
#define RX_MONITOR_WINDOW       3000    // Cumlative results from the 3 hops are reported every RX_MONITOR_WINDOW 

// Common Wi-Fi Init
static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    // ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));

}

static bool mac_equal(const uint8_t *mac1, const uint8_t *mac2)
{
    return (memcmp(mac1, mac2, ESP_NOW_ETH_ALEN) == 0);
}

// ESPNOW Send Callback
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // Optional logging
}

static void alarm_tx_task(void *param)
{
    const TickType_t tx_channel_duration = pdMS_TO_TICKS(TX_CHANNEL_DURATION); // sec per channel
    const TickType_t tx_inter_message_delay = pdMS_TO_TICKS(TX_INTER_MSG_GAP);
    const char *alarm_msg = "ALARM!";
    size_t msg_len = strlen(alarm_msg) + 1;

    ESP_LOGI(TAG, "Starting TX task with %d-channel hopping %dms channel duration and %dms message gap", 
             sizeof(channel_list), TX_CHANNEL_DURATION, TX_INTER_MSG_GAP); 

    while (1) {
        for (int i = 0; i < sizeof(channel_list); ++i) {
            uint8_t ch = channel_list[i];
            ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
            ESP_LOGI(TAG, "TX: Set channel to %d", ch);

            TickType_t start = xTaskGetTickCount();
            int tx_count = 0;

            while ((xTaskGetTickCount() - start) < tx_channel_duration) {
                if (esp_now_send(BROADCAST_MAC, (const uint8_t *)alarm_msg, msg_len) == ESP_OK) {
                    tx_count++;
                }
                vTaskDelay(tx_inter_message_delay);
            }

            ESP_LOGI(TAG, "TX: Channel %d -> Sent %d messages", ch, tx_count);
        }
    }
}


// Receive callback (counts all messages)
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len)
{
    if (recv_info && data && data_len > 0) {
        s_rx_alarm_count++;
    }
}

// RX channel hopper (every X ms, cycle 1 → 6 → 13)
static void channel_hop_rx_task(void *param)
{
    const TickType_t channel_time = pdMS_TO_TICKS(RX_CHANNEL_DURATION);
    ESP_LOGI(TAG, "Starting RX channel hopper with %d-channel hopping %dms per channel", 
             sizeof(channel_list), RX_CHANNEL_DURATION); 

    int idx = 0;

    while (1) {
        uint8_t ch = channel_list[idx];
        ESP_ERROR_CHECK(esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE));
        idx = (idx + 1) % (sizeof(channel_list));
        vTaskDelay(channel_time);
    }
}

static void rx_monitor_task(void *param)
{
    const TickType_t window = pdMS_TO_TICKS(RX_MONITOR_WINDOW);
    while (1) {
        s_rx_alarm_count = 0;
        vTaskDelay(window);
        ESP_LOGI(TAG, "RX: Received %d alarm messages in last %d ms", s_rx_alarm_count, RX_MONITOR_WINDOW);
    }
}

static void espnow_init(device_role_t role)
{
    ESP_ERROR_CHECK(esp_now_init());

    if (role == ROLE_TX) {
        ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

        esp_now_peer_info_t peer = {0};
        memcpy(peer.peer_addr, BROADCAST_MAC, ESP_NOW_ETH_ALEN);
        peer.channel = 0;
        peer.ifidx = ESP_IF_WIFI_STA;
        peer.encrypt = false;
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
        ESP_LOGI(TAG, "Initialized as TX");

    } else if (role == ROLE_RX) {
        ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));

        esp_now_peer_info_t peer = {0};
        memcpy(peer.peer_addr, BROADCAST_MAC, ESP_NOW_ETH_ALEN);
        peer.channel = 0;
        peer.ifidx = ESP_IF_WIFI_STA;
        peer.encrypt = false;
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
        ESP_LOGI(TAG, "Initialized as RX");

    } else {
        ESP_LOGE(TAG, "Unknown role, ESPNOW not initialized.");
    }
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK(esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW));
    ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL));
#endif
}

void app_main(void)
{
    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Wi-Fi
    wifi_init();

    // Determine role from MAC
    uint8_t local_mac[ESP_NOW_ETH_ALEN];
    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, local_mac));

    device_role_t role = ROLE_UNKNOWN;
    if (mac_equal(local_mac, TX_MAC)) {
        role = ROLE_TX;
    } else if (mac_equal(local_mac, RX_MAC)) {
        role = ROLE_RX;
    } else {
        ESP_LOGW(TAG, "MAC "MACSTR" not recognized, defaulting to RX",
                 MAC2STR(local_mac));
        role = ROLE_RX;
    }

    // ESPNOW
    espnow_init(role);

    // Role-specific tasks
    if (role == ROLE_TX) {
        xTaskCreate(alarm_tx_task, "alarm_tx_task", 4096, NULL, 4, NULL);
    } else if (role == ROLE_RX) {
        xTaskCreate(channel_hop_rx_task, "channel_hop_rx_task", 4096, NULL, 5, NULL);
        xTaskCreate(rx_monitor_task, "rx_monitor_task", 4096, NULL, 5, NULL);
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
