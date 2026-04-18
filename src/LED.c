#include "gpio.h"
#include "pinctrl.h"
#include "debug/osal_debug.h"
#include "schedule/osal_task.h"
#include "cmsis_os2.h"
#include "pwm.h"

#include "LED.h"

#define PWM_CHANNEL     2
#define PWM_GROUP_ID    0

// 全局变量（用于任务间传递事件）
extern osEventFlagsId_t led_event_id;   // 在 main.c 中定义

// 当前工作模式
static led_mode_t current_mode = LED_MODE_GREEN_BREATH;

// 呼吸灯步进变量
static int breath_duty = 0;
static int breath_dir = 1;
static bool pwm_initialized = false;
static uint32_t breath_tick = 0;

// 频闪状态（用于交替翻转）
static uint8_t  flash_state = 0;

void led_pwm_init(void)
{
    // 配置引脚复用为 PWM 模式
    osal_printk("PWM init: GPIO=%d, mode=1\r\n", LED_G_PIN);
    uapi_pin_set_mode(LED_G_PIN, 1);
    
    // PWM 配置：周期 1ms（1000Hz），初始占空比 50%
    pwm_config_t cfg = {
        .low_time = 500,   // 低电平时间 500us
        .high_time = 500,  // 高电平时间 500us (50% 占空比)
        .offset_time = 0,
        .cycles = 0,
        .repeat = true
    };
    
    uapi_pwm_deinit();
    uapi_pwm_init();
    osal_printk("PWM init: open channel %d\r\n", PWM_CHANNEL);
    uapi_pwm_open(PWM_CHANNEL, &cfg);
    
    // 使用分组方式启动（与demo一致）
    uint8_t channel_id = PWM_CHANNEL;
    uapi_pwm_set_group(PWM_GROUP_ID, &channel_id, 1);
    uapi_pwm_start_group(PWM_GROUP_ID);
    
    osal_printk("PWM init done\r\n");
    pwm_initialized = true;
}

/* ------------------- 硬件初始化 ------------------- */
void led_alarm_init(void)
{
    // 初始化 GPIO 模块
    uapi_gpio_init();

    // 配置两个 LED 引脚为 GPIO 推挽输出（LED_G 已用于 PWM，不再设置）
    uapi_pin_set_mode(LED_R_PIN, HAL_PIO_FUNC_GPIO);
    uapi_pin_set_mode(LED_B_PIN, HAL_PIO_FUNC_GPIO);

    uapi_gpio_set_dir(LED_R_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_dir(LED_B_PIN, GPIO_DIRECTION_OUTPUT);

    // 初始全部关闭
    uapi_gpio_set_val(LED_R_PIN, GPIO_LEVEL_LOW);
    uapi_gpio_set_val(LED_B_PIN, GPIO_LEVEL_LOW);

    osal_printk("LED alarm init done\r\n");
}

/* ------------------- 模式计算 ------------------- */
led_mode_t led_calc_mode(uint8_t temp, uint8_t humi)
{
    // 优先级：过高 > 过低 > 正常
    if (temp > TEMP_HIGH_THRESHOLD || humi > HUMI_HIGH_THRESHOLD)
        return LED_MODE_RED_FLASH;
    else if (temp < TEMP_LOW_THRESHOLD || humi < HUMI_LOW_THRESHOLD)
        return LED_MODE_BLUE_FLASH;
    else
        return LED_MODE_GREEN_BREATH;
}

/* ------------------- 关闭所有 LED ------------------- */
void led_stop_all(void)
{
    uapi_gpio_set_val(LED_R_PIN, GPIO_LEVEL_LOW);
    uapi_gpio_set_val(LED_B_PIN, GPIO_LEVEL_LOW);
    
    if (pwm_initialized) {
        uapi_pwm_stop_group(PWM_GROUP_ID);
        
        pwm_config_t cfg = {
            .low_time = 1000,
            .high_time = 0,
            .offset_time = 0,
            .cycles = 0,
            .repeat = true
        };
        uapi_pwm_open(PWM_CHANNEL, &cfg);
        uapi_pwm_start_group(PWM_GROUP_ID);
    }
}

void led_set_duty(uint8_t percent)
{
    if (!pwm_initialized) return;
    
    // 限制 0~100
    if (percent > 100) percent = 100;
    
    // 周期固定 1000us，高电平时间 = percent * 10us
    uint32_t high_us = percent * 10;   // 例如 50% -> 500us
    uint32_t low_us  = 1000 - high_us;
    
    // 使用分组方式更新（与初始化一致）
    uapi_pwm_stop_group(PWM_GROUP_ID);
    
    pwm_config_t cfg = {
        .low_time = low_us,
        .high_time = high_us,
        .offset_time = 0,
        .cycles = 0,
        .repeat = true
    };
    // osal_printk("PWM set duty: open channel\r\n");
    uapi_pwm_open(PWM_CHANNEL, &cfg);
    uapi_pwm_start_group(PWM_GROUP_ID);
}

/* ------------------- LED 控制任务（需在 RTOS 中创建） ------------------- */
void led_control_task(void *argument)
{
    (void)argument;
    led_alarm_init();
    led_pwm_init();   // 初始化硬件 PWM

    // 等待事件标志组
    if (led_event_id == NULL) {
        osal_printk("LED event flag not created!\r\n");
        return;
    }

    uint32_t flash_interval_ms = 200;
    const uint32_t step_ms = 10;   // 呼吸灯每 10ms 步进一次（更慢更平滑）
    // 如果想更快或更慢，可以调整 step_ms（步进间隔）或 breath_duty += breath_dir 中的增量

    while (1) {
        // 等待温湿度更新事件（超时等待，避免阻塞）
        uint32_t flags = osEventFlagsWait(led_event_id, DHT11_UPDATED_EVT,
                                          osFlagsWaitAny, 10);  // 超时 10ms

        if (flags & DHT11_UPDATED_EVT) {
            // 事件触发，重新计算模式
            uint8_t temp_int = (uint8_t)g_latest_temp;
            uint8_t humi_int = (uint8_t)g_latest_humi;
            led_mode_t new_mode = led_calc_mode(temp_int, humi_int);

            if (new_mode != current_mode) {
                current_mode = new_mode;
                led_stop_all();
                breath_duty = 1;  // 从最小值开始，确保LED会亮
                breath_dir = 1;
                flash_state = 0;
                osal_printk("LED mode changed to %d\r\n", current_mode);
            }
        }
        
        // 根据当前模式执行对应的 LED 效果
        switch (current_mode) {
            case LED_MODE_GREEN_BREATH:
                // 呼吸灯效果：慢速渐变，每 25ms 步进一次
                breath_duty += breath_dir;
                if (breath_duty >= 50) {
                    breath_duty = 50;
                    breath_dir = -1;
                } else if (breath_duty <= 5) {  // 最小值 5%，避免完全熄灭
                    breath_duty = 5;
                    breath_dir = 1;
                }
                led_set_duty((uint8_t)breath_duty);
                break;
                
            case LED_MODE_RED_FLASH:
                // 红色 LED 频闪（每 step_ms 翻转一次）
                flash_state = !flash_state;
                uapi_gpio_set_val(LED_R_PIN, flash_state ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
                break;
                
            case LED_MODE_BLUE_FLASH:
                // 蓝色 LED 频闪
                flash_state = !flash_state;
                uapi_gpio_set_val(LED_B_PIN, flash_state ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
                break;
                
            default:
                break;
        }
        
        osDelay(step_ms);
    }
}