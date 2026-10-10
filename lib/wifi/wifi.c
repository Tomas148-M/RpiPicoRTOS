#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/atomic.h>
#include <wifi_connection.h>

LOG_MODULE_REGISTER(wifi, CONFIG_LOG_DEFAULT_LEVEL);
static struct net_if *wifi_iface;
static struct net_mgmt_event_callback wifi_cb, ipv4_cb;
static atomic_t connected;
K_SEM_DEFINE(connection_changed, 0, 1);

static void wifi_event(struct net_mgmt_event_callback *cb, uint64_t event,
                       struct net_if *iface)
{
    if (iface != wifi_iface) {
        return;
    }
    if (event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *status = cb->info;
        if (status && status->status == 0) {
            atomic_set(&connected, 1);
            LOG_INF("Wi-Fi connected; waiting for DHCP");
        } else {
            atomic_clear(&connected);
            LOG_ERR("Wi-Fi connection failed (%d)", status ? status->status : -1);
        }
    } else if (event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        atomic_clear(&connected);
        LOG_WRN("Wi-Fi disconnected");
    } else {
        return;
    }
    k_sem_give(&connection_changed);
}

static void ipv4_event(struct net_mgmt_event_callback *cb, uint64_t event,
                       struct net_if *iface)
{
    ARG_UNUSED(cb);
    if (iface == wifi_iface && event == NET_EVENT_IPV4_DHCP_BOUND) {
        const struct in_addr *addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
        char text[NET_IPV4_ADDR_LEN];
        if (addr) {
            LOG_INF("DHCP ready: IPv4 %s", net_addr_ntop(AF_INET, addr, text, sizeof(text)));
        }
    }
}

int wifi_run(const char *ssid, const char *password)
{
    if (!ssid || !password) { return -EINVAL; }
    struct wifi_connect_req_params params = {
        .ssid = (const uint8_t *)ssid,
        .ssid_length = strlen(ssid),
        .psk = (const uint8_t *)password,
        .psk_length = strlen(password),
        .security = WIFI_SECURITY_TYPE_PSK,
        .band = WIFI_FREQ_BAND_2_4_GHZ,
        .channel = WIFI_CHANNEL_ANY,
        .mfp = WIFI_MFP_OPTIONAL,
        .timeout = 30,
    };

    LOG_INF("Starting Wi-Fi on %s", CONFIG_BOARD);
    if (strlen(ssid) == 0) {
        LOG_WRN("Set SSID and password in src/wifi_env.h, then rebuild");
        return 0;
    }
    if (strlen(ssid) > 32 || strlen(password) < 8 ||
        strlen(password) > 63) {
        LOG_ERR("WPA2 requires a 1-32 byte SSID and an 8-63 byte passphrase");
        return 0;
    }
    wifi_iface = net_if_get_wifi_sta();
    if (!wifi_iface) {
        LOG_ERR("No Wi-Fi interface; select the Pico 2 W board");
        return 0;
    }
    net_mgmt_init_event_callback(&wifi_cb, wifi_event,
        NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);
    net_mgmt_init_event_callback(&ipv4_cb, ipv4_event, NET_EVENT_IPV4_DHCP_BOUND);
    net_mgmt_add_event_callback(&ipv4_cb);
    net_dhcpv4_start(wifi_iface);

    while (true) {
        k_sem_reset(&connection_changed);
        LOG_INF("Connecting to %s", ssid);
        int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_iface, &params, sizeof(params));
        if (ret) {
            LOG_ERR("Connection request failed (%d)", ret);
        } else {
            ret = k_sem_take(&connection_changed, K_SECONDS(40));
            if (ret) {
                LOG_WRN("Connection timed out");
                (void)net_mgmt(NET_REQUEST_WIFI_DISCONNECT, wifi_iface, NULL, 0);
            }
            while (atomic_get(&connected)) {
                k_sem_take(&connection_changed, K_FOREVER);
            }
        }
        LOG_INF("Retrying in 10 seconds");
        k_sleep(K_SECONDS(10));
    }
    return 0;
}

