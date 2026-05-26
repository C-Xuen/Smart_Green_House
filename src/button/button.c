#include "pinctrl.h"
#include "gpio.h"
#include "soc_osal.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "common_def.h"
#include "button.h"

#define BTN1_PIN GPIO_03
#define BTN2_PIN GPIO_14
#define BTN3_PIN GPIO_06

static volatile int g_btn1_pending = 0;
static volatile int g_btn2_pending = 0;
static volatile int g_btn3_pending = 0;

static void *button_task(const char *arg)
{
    unused(arg);
    int last1 = 0, last2 = 0, last3 = 0;

    while (1) {
        int b1 = (uapi_gpio_get_val(BTN1_PIN) == GPIO_LEVEL_LOW);
        int b2 = (uapi_gpio_get_val(BTN2_PIN) == GPIO_LEVEL_LOW);
        int b3 = (uapi_gpio_get_val(BTN3_PIN) == GPIO_LEVEL_LOW);

        if (b1 && !last1) g_btn1_pending = 1;
        if (b2 && !last2) g_btn2_pending = 1;
        if (b3 && !last3) g_btn3_pending = 1;

        last1 = b1; last2 = b2; last3 = b3;
        osal_msleep(10);
    }
    return NULL;
}

void button_init(void)
{
    uapi_pin_set_mode(BTN1_PIN, 0);
    uapi_gpio_set_dir(BTN1_PIN, GPIO_DIRECTION_INPUT);
    uapi_pin_set_pull(BTN1_PIN, PIN_PULL_TYPE_UP);

    uapi_pin_set_mode(BTN2_PIN, 0);
    uapi_gpio_set_dir(BTN2_PIN, GPIO_DIRECTION_INPUT);
    uapi_pin_set_pull(BTN2_PIN, PIN_PULL_TYPE_UP);

    uapi_pin_set_mode(BTN3_PIN, 0);
    uapi_gpio_set_dir(BTN3_PIN, GPIO_DIRECTION_INPUT);
    uapi_pin_set_pull(BTN3_PIN, PIN_PULL_TYPE_UP);

    osThreadAttr_t attr = {0};
    attr.name = "ButtonTask";
    attr.stack_size = 0x400;
    attr.priority = (osPriority_t)(18);
    osThreadNew((osThreadFunc_t)button_task, NULL, &attr);
}

int btn1_pressed(void)
{
    if (g_btn1_pending) { g_btn1_pending = 0; return 1; }
    return 0;
}

int btn2_pressed(void)
{
    if (g_btn2_pending) { g_btn2_pending = 0; return 1; }
    return 0;
}

int btn3_pressed(void)
{
    if (g_btn3_pending) { g_btn3_pending = 0; return 1; }
    return 0;
}
