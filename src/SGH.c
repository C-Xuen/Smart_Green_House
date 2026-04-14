#include <stdint.h>
#include <string.h>
#include "cmsis_os2.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "app_init.h"
#include "gpio.h"
#include "pinctrl.h"
#include "dht11.h"
#include "common_def.h"
#include "LED.h"

static void *dht11_task_func(const char *arg)
{
    read(arg);
    return NULL;
}

 /**
  * 系统入口函数
  */
static void smart_green_house_entry(void)
{
    uapi_gpio_init();

    // // 3. 初始化按键（包括创建事件标志组）

    // 4. 创建互斥量保护全局数据
    data_mutex = osMutexNew(NULL);
    // 创建事件标志组
    led_event_id = osEventFlagsNew(NULL);

    // 5. 创建 DHT11 任务
    osThreadAttr_t attr = {0};
    attr.name = "DHT11";
    attr.stack_size = 0x1000;
    attr.priority = (osPriority_t)(17);
    if (osThreadNew((osThreadFunc_t)dht11_task_func, NULL, &attr) == NULL) {
        osal_printk("[ERR] Failed to create dht11 task\r\n");
    }

    osThreadAttr_t led_attr = {0};
    led_attr.name = "LED_Control";
    led_attr.stack_size = 0x1000;
    led_attr.priority = osPriorityNormal;
    if (osThreadNew(led_control_task, NULL, &led_attr) == NULL) {
        osal_printk("[ERR] Failed to create LED task\r\n");
    }
    // // 6. 创建按键任务
    // osThreadId_t button_task_id = osThreadNew(button_task_func, NULL, NULL);
    // if (button_task_id == NULL) {
    //     osal_printk("Failed to create button task\r\n");
    // }
}

/* 将入口函数注册到系统启动 */
app_run(smart_green_house_entry);
