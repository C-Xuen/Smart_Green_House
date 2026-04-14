#ifndef LED_H
#define LED_H

#include <stdint.h>

#define LED_R_PIN   GPIO_01   
#define LED_G_PIN   GPIO_02   // PWM 通道（与demo一致）
#define LED_B_PIN   GPIO_03

#define TEMP_HIGH_THRESHOLD     28.00   // 高温阈值（℃）
#define TEMP_LOW_THRESHOLD      26.00   // 低温阈值（℃）
#define HUMI_HIGH_THRESHOLD     75.00   // 高湿阈值（%）
#define HUMI_LOW_THRESHOLD      45.00   // 低湿阈值（%）

typedef enum {
    LED_MODE_GREEN_BREATH = 0,   // 绿色呼吸灯
    LED_MODE_RED_FLASH,      // 红色频闪
    LED_MODE_BLUE_FLASH      // 蓝色频闪
} led_mode_t;

// 外部事件标志（用于通知 LED 任务更新模式）
#define DHT11_UPDATED_EVT        (1 << 0)

led_mode_t led_calc_mode(uint8_t temp, uint8_t humi);
void led_control_task(void *argument);
void led_stop_all(void);

#endif