#include "pinctrl.h"
#include "i2c.h"
#include "osal_debug.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "oled.h"
#include "cn_font.h"
#include <stdarg.h>
#include <stdio.h>
#include "common_def.h"        
#include "i2c.h" 

#define I2C_BUS_ID      1
#define I2C_PIN_SCL     15
#define I2C_PIN_SDA     16
#define I2C_BAUDRATE    400000
#define I2C_PIN_MODE    2

void oled_init(void)
{
    uint32_t baudrate = I2C_BAUDRATE;
    uint8_t hscode = 0x0;
    uapi_pin_set_mode(I2C_PIN_SCL, I2C_PIN_MODE);
    uapi_pin_set_mode(I2C_PIN_SDA, I2C_PIN_MODE);
    uapi_pin_set_pull(I2C_PIN_SCL, PIN_PULL_TYPE_UP);
    uapi_pin_set_pull(I2C_PIN_SDA, PIN_PULL_TYPE_UP);

    uapi_i2c_master_init(I2C_BUS_ID, baudrate, hscode);

    ssd1306_Init();
    ssd1306_Fill(Black);
    osal_printk("[OLED] init done\r\n");
}

void oled_clear(void)
{
    ssd1306_Fill(Black);
}

void oled_show_string(uint8_t x, uint8_t y, const char *str)
{
    ssd1306_SetCursor(x, y);
    ssd1306_DrawString((char *)str, Font_7x10, White);
}

void oled_printf(uint8_t x, uint8_t y,const char *fmt, ...)
{
    ssd1306_SetCursor(x, y);
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    ssd1306_DrawString(buf, Font_7x10, White);
}

void oled_update(void)
{
    ssd1306_UpdateScreen();
}

void oled_show_cn(int idx)
{
    ssd1306_DrawCN((uint16_t)idx, &Font_CN12, White);
}
