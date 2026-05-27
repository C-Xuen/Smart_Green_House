#include "gpio.h"
#include "pinctrl.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "cmsis_os2.h"
#include "pwm.h"
#include "dht11/dht11.h"
#include "LED.h"

#define PWM_CHANNEL     2
#define PWM_GROUP_ID    0

extern osEventFlagsId_t led_event_id;

static led_mode_t current_mode = LED_MODE_GREEN_BREATH;
static int breath_duty = 0;
static int breath_dir = 1;
static bool pwm_initialized = false;
static uint8_t flash_state = 0;

void led_pwm_init(void)
{
    uapi_pin_set_mode(LED_G_PIN, 1);
    
    pwm_config_t cfg = {
        .low_time = 500,
        .high_time = 500,
        .offset_time = 0,
        .cycles = 0,
        .repeat = true
    };
    
    uapi_pwm_deinit();
    uapi_pwm_init();
    uapi_pwm_open(PWM_CHANNEL, &cfg);
    
    uint8_t channel_id = PWM_CHANNEL;
    uapi_pwm_set_group(PWM_GROUP_ID, &channel_id, 1);
    uapi_pwm_start_group(PWM_GROUP_ID);
    
    pwm_initialized = true;
}

void led_alarm_init(void)
{
    uapi_gpio_init();
    uapi_pin_set_mode(LED_R_PIN, HAL_PIO_FUNC_GPIO);
    uapi_pin_set_mode(LED_B_PIN, HAL_PIO_FUNC_GPIO);
    uapi_gpio_set_dir(LED_R_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_dir(LED_B_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(LED_R_PIN, GPIO_LEVEL_LOW);
    uapi_gpio_set_val(LED_B_PIN, GPIO_LEVEL_LOW);
}

led_mode_t led_calc_mode(uint8_t temp, uint8_t humi)
{
    if (temp > TEMP_HIGH_THRESHOLD || humi > HUMI_HIGH_THRESHOLD)
        return LED_MODE_RED_FLASH;
    else if (temp < TEMP_LOW_THRESHOLD || humi < HUMI_LOW_THRESHOLD)
        return LED_MODE_BLUE_FLASH;
    else
        return LED_MODE_GREEN_BREATH;
}

void led_stop_all(void)
{
    uapi_gpio_set_val(LED_R_PIN, GPIO_LEVEL_LOW);
    uapi_gpio_set_val(LED_B_PIN, GPIO_LEVEL_LOW);
    
    if (pwm_initialized) {
        uapi_pwm_stop_group(PWM_GROUP_ID);
        pwm_config_t cfg = { .low_time = 1000, .high_time = 0, .offset_time = 0, .cycles = 0, .repeat = true };
        uapi_pwm_open(PWM_CHANNEL, &cfg);
        uapi_pwm_start_group(PWM_GROUP_ID);
    }
}

void led_set_duty(uint8_t percent)
{
    if (!pwm_initialized) return;
    if (percent > 100) percent = 100;
    
    uint32_t high_us = percent * 10;
    uint32_t low_us  = 1000 - high_us;
    
    uapi_pwm_stop_group(PWM_GROUP_ID);
    pwm_config_t cfg = { .low_time = low_us, .high_time = high_us, .offset_time = 0, .cycles = 0, .repeat = true };
    uapi_pwm_open(PWM_CHANNEL, &cfg);
    uapi_pwm_start_group(PWM_GROUP_ID);
}

void led_control_task(void *argument)
{
    (void)argument;
    led_alarm_init();
    led_pwm_init();

    if (led_event_id == NULL) return;

    const uint32_t step_ms = 10;

    while (1) {
        uint32_t flags = osEventFlagsWait(led_event_id, DHT11_UPDATED_EVT, osFlagsWaitAny, 10);

        if (flags & DHT11_UPDATED_EVT) {
            uint8_t temp_int = dht11_get_temp_int();
            uint8_t humi_int = dht11_get_humi_int();
            led_mode_t new_mode = led_calc_mode(temp_int, humi_int);

            if (new_mode != current_mode) {
                current_mode = new_mode;
                led_stop_all();
                breath_duty = 1;
                breath_dir = 1;
                flash_state = 0;
            }
        }
        
        switch (current_mode) {
            case LED_MODE_GREEN_BREATH:
                breath_duty += breath_dir;
                if (breath_duty >= 50) { breath_duty = 50; breath_dir = -1; }
                else if (breath_duty <= 5) { breath_duty = 5; breath_dir = 1; }
                led_set_duty((uint8_t)breath_duty);
                break;
            case LED_MODE_RED_FLASH:
                flash_state = !flash_state;
                uapi_gpio_set_val(LED_R_PIN, flash_state ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
                break;
            case LED_MODE_BLUE_FLASH:
                flash_state = !flash_state;
                uapi_gpio_set_val(LED_B_PIN, flash_state ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
                break;
            default: break;
        }
        
        osDelay(step_ms);
    }
}
