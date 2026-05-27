#include "pinctrl.h"
#include "gpio.h"
#include "flame.h"

#define FLAME_PIN GPIO_01

void flame_init(void)
{
    uapi_pin_set_mode(FLAME_PIN, 0);
    uapi_gpio_set_dir(FLAME_PIN, GPIO_DIRECTION_INPUT);
}

int flame_detected(void)
{
    return uapi_gpio_get_val(FLAME_PIN) == GPIO_LEVEL_LOW;
}
