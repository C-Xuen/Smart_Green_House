#include "pinctrl.h"
#include "gpio.h"
#include "light.h"

#define LIGHT_PIN GPIO_13

static int g_light_manual = 0;

void light_init(void)
{
    uapi_pin_set_mode(LIGHT_PIN, 0);
    uapi_gpio_set_val(LIGHT_PIN, GPIO_LEVEL_LOW);
    uapi_gpio_set_dir(LIGHT_PIN, GPIO_DIRECTION_OUTPUT);
}

void light_on(void)  { uapi_gpio_set_val(LIGHT_PIN, GPIO_LEVEL_HIGH); g_light_manual = 1; }
void light_off(void) { uapi_gpio_set_val(LIGHT_PIN, GPIO_LEVEL_LOW);  g_light_manual = 0; }

int light_is_manual(void) { return g_light_manual; }
