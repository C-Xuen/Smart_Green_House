#include "pinctrl.h"
#include "gpio.h"
#include "motor.h"

#define FAN_PIN  GPIO_11
#define PUMP_PIN GPIO_12

static int g_fan_manual  = 0;
static int g_pump_manual = 0;

void motor_init(void)
{
    uapi_pin_set_mode(FAN_PIN, 0);
    uapi_gpio_set_val(FAN_PIN, GPIO_LEVEL_HIGH);
    uapi_gpio_set_dir(FAN_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_pin_set_mode(PUMP_PIN, 0);
    uapi_gpio_set_val(PUMP_PIN, GPIO_LEVEL_HIGH);
    uapi_gpio_set_dir(PUMP_PIN, GPIO_DIRECTION_OUTPUT);
}

void fan_on(void)  { uapi_gpio_set_val(FAN_PIN,  GPIO_LEVEL_LOW);  g_fan_manual = 1; }
void fan_off(void) { uapi_gpio_set_val(FAN_PIN,  GPIO_LEVEL_HIGH); g_fan_manual = 0; }
void pump_on(void) { uapi_gpio_set_val(PUMP_PIN, GPIO_LEVEL_LOW);  g_pump_manual = 1; }
void pump_off(void){ uapi_gpio_set_val(PUMP_PIN, GPIO_LEVEL_HIGH); g_pump_manual = 0; }

int  fan_is_manual(void)  { return g_fan_manual; }
int  pump_is_manual(void) { return g_pump_manual; }
void fan_set_manual(int v)  { g_fan_manual = v; }
void pump_set_manual(int v) { g_pump_manual = v; }
