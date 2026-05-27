#include "lwip/netifapi.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"
#include "wifi_hotspot.h"
#include "wifi_hotspot_config.h"
#include "td_base.h"
#include "td_type.h"
#include "stdlib.h"
#include "uart.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "soc_osal.h"
#include "osal_debug.h"

#define WIFI_IFNAME_MAX_SIZE             16
#define WIFI_MAX_SSID_LEN                33
#define WIFI_SCAN_AP_LIMIT               64
#define WIFI_MAC_LEN                     6
#define WIFI_STA_SAMPLE_LOG              "[WIFI_STA_SAMPLE]"
#define WIFI_NOT_AVALLIABLE              0
#define WIFI_AVALIABE                    1
#define WIFI_GET_IP_MAX_COUNT            300

#define WIFI_TASK_PRIO                  (osPriority_t)(13)
#define WIFI_TASK_DURATION_MS           2000
#define WIFI_TASK_STACK_SIZE            0x1000

static td_void wifi_scan_state_changed(td_s32 state, td_s32 size);
static td_void wifi_connection_changed(td_s32 state, const wifi_linked_info_stru *info, td_s32 reason_code);

wifi_event_stru wifi_event_cb = {
    .wifi_event_connection_changed      = wifi_connection_changed,
    .wifi_event_scan_state_changed      = wifi_scan_state_changed,
};

enum {
    WIFI_STA_SAMPLE_INIT = 0,
    WIFI_STA_SAMPLE_SCANING,
    WIFI_STA_SAMPLE_SCAN_DONE,
    WIFI_STA_SAMPLE_FOUND_TARGET,
    WIFI_STA_SAMPLE_CONNECTING,
    WIFI_STA_SAMPLE_CONNECT_DONE,
    WIFI_STA_SAMPLE_GET_IP,
} wifi_state_enum;

static td_u8 g_wifi_state = WIFI_STA_SAMPLE_INIT;

static td_void wifi_scan_state_changed(td_s32 state, td_s32 size)
{
    UNUSED(state);
    UNUSED(size);
    osal_printk("%s::Scan done!\r\n", WIFI_STA_SAMPLE_LOG);
    g_wifi_state = WIFI_STA_SAMPLE_SCAN_DONE;
    return;
}

static td_void wifi_connection_changed(td_s32 state, const wifi_linked_info_stru *info, td_s32 reason_code)
{
    UNUSED(info);
    UNUSED(reason_code);

    if (state == WIFI_NOT_AVALLIABLE) {
        osal_printk("%s::Connect fail! Try again!\r\n", WIFI_STA_SAMPLE_LOG);
        g_wifi_state = WIFI_STA_SAMPLE_INIT;
    } else {
        osal_printk("%s::Connect succ!\r\n", WIFI_STA_SAMPLE_LOG);
        g_wifi_state = WIFI_STA_SAMPLE_CONNECT_DONE;
    }
}

static td_s32 example_get_match_network(wifi_sta_config_stru *expected_bss, const char *ssid, const char *psk)
{
    td_s32  ret;
    td_u32  num = 64;
    td_bool find_ap = TD_FALSE;
    td_u8   bss_index;
    td_u32 scan_len = sizeof(wifi_scan_info_stru) * WIFI_SCAN_AP_LIMIT;
    wifi_scan_info_stru *result = osal_kmalloc(scan_len, OSAL_GFP_ATOMIC);
    if (result == TD_NULL) {
        return -1;
    }
    memset_s(result, scan_len, 0, scan_len);
    ret = wifi_sta_get_scan_info(result, &num);
    if (ret != 0) {
        osal_kfree(result);
        return -1;
    }
    for (bss_index = 0; bss_index < num; bss_index++) {
        if (strlen(ssid) == strlen(result[bss_index].ssid)) {
            if (memcmp(ssid, result[bss_index].ssid, strlen(ssid)) == 0) {
                find_ap = TD_TRUE;
                break;
            }
        }
    }
    if (find_ap == TD_FALSE) {
        osal_kfree(result);
        return -1;
    }
    if (memcpy_s(expected_bss->ssid, WIFI_MAX_SSID_LEN, ssid, strlen(ssid)) != 0) {
        osal_kfree(result);
        return -1;
    }
    if (memcpy_s(expected_bss->bssid, WIFI_MAC_LEN, result[bss_index].bssid, WIFI_MAC_LEN) != 0) {
        osal_kfree(result);
        return -1;
    }
    expected_bss->security_type = result[bss_index].security_type;
    if (memcpy_s(expected_bss->pre_shared_key, WIFI_MAX_SSID_LEN, psk, strlen(psk)) != 0) {
        osal_kfree(result);
        return -1;
    }
    expected_bss->ip_type = 1;
    osal_kfree(result);
    return 0;
}

static td_bool example_check_dhcp_status(struct netif *netif_p, td_u32 *wait_count)
{
    if ((ip_addr_isany(&(netif_p->ip_addr)) == 0) && (*wait_count <= WIFI_GET_IP_MAX_COUNT)) {
        osal_printk("%s::STA DHCP success!\r\n", WIFI_STA_SAMPLE_LOG);
        char ip_str[16];
        osal_printk("%s::IP Address: %s\r\n", WIFI_STA_SAMPLE_LOG, 
            ip4addr_ntoa((const ip4_addr_t *)&netif_p->ip_addr));
        return 0;
    }

    if (*wait_count > WIFI_GET_IP_MAX_COUNT) {
        osal_printk("%s::STA DHCP timeout!\r\n", WIFI_STA_SAMPLE_LOG);
        *wait_count = 0;
        g_wifi_state = WIFI_STA_SAMPLE_INIT;
    }
    return -1;
}

static td_s32 example_sta_function(const char *ssid, const char *psk)
{
    td_char ifname[WIFI_IFNAME_MAX_SIZE + 1] = "wlan0";
    wifi_sta_config_stru expected_bss = {0};
    struct netif *netif_p = TD_NULL;
    td_u32 wait_count = 0;

    if (wifi_sta_enable() != 0) {
        return -1;
    }
    osal_printk("%s::STA enable succ\r\n", WIFI_STA_SAMPLE_LOG);

    do {
        (void)osDelay(1);
        if (g_wifi_state == WIFI_STA_SAMPLE_INIT) {
            osal_printk("%s::Scan start!\r\n", WIFI_STA_SAMPLE_LOG);
            g_wifi_state = WIFI_STA_SAMPLE_SCANING;
            if (wifi_sta_scan() != 0) {
                g_wifi_state = WIFI_STA_SAMPLE_INIT;
                continue;
            }
        } else if (g_wifi_state == WIFI_STA_SAMPLE_SCAN_DONE) {
            if (example_get_match_network(&expected_bss, ssid, psk) != 0) {
                osal_printk("%s::Target AP not found!\r\n", WIFI_STA_SAMPLE_LOG);
                g_wifi_state = WIFI_STA_SAMPLE_INIT;
                continue;
            }
            g_wifi_state = WIFI_STA_SAMPLE_FOUND_TARGET;
        } else if (g_wifi_state == WIFI_STA_SAMPLE_FOUND_TARGET) {
            osal_printk("%s::Connecting to %s...\r\n", WIFI_STA_SAMPLE_LOG, ssid);
            g_wifi_state = WIFI_STA_SAMPLE_CONNECTING;
            if (wifi_sta_connect(&expected_bss) != 0) {
                g_wifi_state = WIFI_STA_SAMPLE_INIT;
                continue;
            }
        } else if (g_wifi_state == WIFI_STA_SAMPLE_CONNECT_DONE) {
            osal_printk("%s::DHCP start\r\n", WIFI_STA_SAMPLE_LOG);
            g_wifi_state = WIFI_STA_SAMPLE_GET_IP;
            netif_p = netifapi_netif_find(ifname);
            if (netif_p == TD_NULL || netifapi_dhcp_start(netif_p) != 0) {
                osal_printk("%s::DHCP fail\r\n", WIFI_STA_SAMPLE_LOG);
                g_wifi_state = WIFI_STA_SAMPLE_INIT;
                continue;
            }
        } else if (g_wifi_state == WIFI_STA_SAMPLE_GET_IP) {
            if (example_check_dhcp_status(netif_p, &wait_count) == 0) {
                break;
            }
            wait_count++;
        }
    } while (1);

    return 0;
}

int wifi_connect(const char *ssid, const char *psk)
{
    if (ssid == NULL || psk == NULL) {
        osal_printk("%s::Invalid parameters\r\n", WIFI_STA_SAMPLE_LOG);
        return -1;
    }

    if (wifi_register_event_cb(&wifi_event_cb) != 0) {
        osal_printk("%s::Event callback register fail\r\n", WIFI_STA_SAMPLE_LOG);
        return -1;
    }
    osal_printk("%s::Event callback register succ\r\n", WIFI_STA_SAMPLE_LOG);

    while (wifi_is_wifi_inited() == 0) {
        (void)osDelay(10);
    }
    osal_printk("%s::WiFi init succ\r\n", WIFI_STA_SAMPLE_LOG);

    if (example_sta_function(ssid, psk) != 0) {
        osal_printk("%s::Connect fail\r\n", WIFI_STA_SAMPLE_LOG);
        return -1;
    }

    osal_printk("%s::WiFi connected! SSID: %s\r\n", WIFI_STA_SAMPLE_LOG, ssid);
    return 0;
}
