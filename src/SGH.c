#include <stdint.h>
#include <string.h>
#include "cmsis_os2.h"
#include "debug/osal_debug.h"
#include "schedule/osal_task.h"
#include "app_init.h"
#include "gpio.h"
#include "pinctrl.h"
#include "common_def.h"

#include "dht11.h"
#include "LED.h"
#include "wifi_connect.h"

#define WIFI_SSID     "ACH"      // 替换为你的WiFi名称
#define WIFI_PASSWORD "12345678"   // 替换为你的WiFi密码

static void *dht11_task_func(const char *arg)
{
    dht11_read(arg);
    return NULL;
}

static void *wifi_task_func(const char *arg)
{
    (void)arg;
    osal_printk("[WIFI] Connecting to SSID: %s\r\n", WIFI_SSID);
    if (wifi_connect(WIFI_SSID, WIFI_PASSWORD) == 0) {
        osal_printk("[WIFI] Connected successfully!\r\n");
    } else {
        osal_printk("[WIFI] Connection failed!\r\n");
    }
    return NULL;
}

extern void http_server_task(void *arg);

/**
  * 系统入口函数
  */
static void smart_green_house_entry(void)
{
    uapi_gpio_init();

    // 1. 创建互斥量保护全局数据
    data_mutex = osMutexNew(NULL);
    // 2. 创建事件标志组
    led_event_id = osEventFlagsNew(NULL);

    // 3. 创建 WiFi 连接任务
    osThreadAttr_t wifi_attr = {0};
    wifi_attr.name = "WiFi_Connect";
    wifi_attr.stack_size = 0x2000;
    wifi_attr.priority = osPriorityHigh;
    if (osThreadNew((osThreadFunc_t)wifi_task_func, NULL, &wifi_attr) == NULL) {
        osal_printk("[ERR] Failed to create WiFi task\r\n");
    }

    // 4. 创建 DHT11 任务
    osThreadAttr_t attr = {0};
    attr.name = "DHT11";
    attr.stack_size = 0x1000;
    attr.priority = (osPriority_t)(17);
    if (osThreadNew((osThreadFunc_t)dht11_task_func, NULL, &attr) == NULL) {
        osal_printk("[ERR] Failed to create dht11 task\r\n");
    }

    // 5. 创建 DHT11 任务
    osThreadAttr_t led_attr = {0};
    led_attr.name = "LED_Control";
    led_attr.stack_size = 0x1000;
    led_attr.priority = osPriorityNormal;
    if (osThreadNew(led_control_task, NULL, &led_attr) == NULL) {
        osal_printk("[ERR] Failed to create LED task\r\n");
    }

    // 6. 创建 HTTP 服务器任务
    osThreadAttr_t http_attr = {0};
    http_attr.name = "HTTP_Server";
    http_attr.stack_size = 0x2000;
    http_attr.priority = (osPriority_t)(18);
    if (osThreadNew((osThreadFunc_t)http_server_task, NULL, &http_attr) == NULL) {
        osal_printk("[ERR] Failed to create HTTP server task\r\n");
    }
}

/* 将入口函数注册到系统启动 */
app_run(smart_green_house_entry);
