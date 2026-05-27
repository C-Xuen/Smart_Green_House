#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"

void oled_init(void);
void oled_clear(void);
void oled_show_string(uint8_t x, uint8_t y, const char *str);
void oled_printf(uint8_t x, uint8_t y,const char *fmt, ...);
void oled_update(void);
void oled_show_cn(int idx);

#endif
