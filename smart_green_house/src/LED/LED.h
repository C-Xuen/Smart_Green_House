#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

#define LED_R_PIN   GPIO_01
#define LED_G_PIN   GPIO_02
#define LED_B_PIN   GPIO_03

#define TEMP_HIGH_THRESHOLD     28
#define TEMP_LOW_THRESHOLD      26
#define HUMI_HIGH_THRESHOLD     75
#define HUMI_LOW_THRESHOLD      45

#define DHT11_UPDATED_EVT        (1 << 0)

typedef enum {
    LED_MODE_GREEN_BREATH = 0,
    LED_MODE_RED_FLASH,
    LED_MODE_BLUE_FLASH
} led_mode_t;

led_mode_t led_calc_mode(uint8_t temp, uint8_t humi);
void led_control_task(void *argument);
void led_stop_all(void);

#endif
