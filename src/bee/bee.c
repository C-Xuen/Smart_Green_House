#include "pinctrl.h"
#include "soc_osal.h"
#include "gpio.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "bee.h"


#define CONFIG_BEE_PIN    2


void bee_init(void)
{
    uapi_pin_set_mode(CONFIG_BEE_PIN, PIN_MODE_0);
    uapi_gpio_set_dir(CONFIG_BEE_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(CONFIG_BEE_PIN, GPIO_LEVEL_LOW);
}

void bee_on(void)
{
    uapi_gpio_set_val(CONFIG_BEE_PIN, GPIO_LEVEL_HIGH);
}

void bee_off(void)
{
    uapi_gpio_set_val(CONFIG_BEE_PIN, GPIO_LEVEL_LOW);
}
    
void toggle_bee(void)
{
    uapi_gpio_toggle(CONFIG_BEE_PIN);
}

