#include "pinctrl.h"
#include "gpio.h"
#include "motor.h"

#define FAN_PIN  GPIO_11
#define PUMP_PIN GPIO_12

void motor_init(void)
{
    uapi_pin_set_mode(FAN_PIN, 0);
    uapi_gpio_set_dir(FAN_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(FAN_PIN, GPIO_LEVEL_HIGH);
    uapi_gpio_set_dir(PUMP_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(PUMP_PIN, GPIO_LEVEL_HIGH);
}

void fan_on(void)  { uapi_gpio_set_val(FAN_PIN,  GPIO_LEVEL_LOW);  }
void fan_off(void) { uapi_gpio_set_val(FAN_PIN,  GPIO_LEVEL_HIGH); }
void pump_on(void) { uapi_gpio_set_val(PUMP_PIN, GPIO_LEVEL_LOW);  }
void pump_off(void){ uapi_gpio_set_val(PUMP_PIN, GPIO_LEVEL_HIGH); }
