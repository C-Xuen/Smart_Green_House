#ifndef __DHT11_H
#define __DHT11_H

#include <stdint.h>
#include "cmsis_os2.h"

#define DHT11_PIN    GPIO_00
#define PIN_MODE_0   0

extern uint8_t g_latest_temp;
extern uint8_t g_latest_humi;
extern osMutexId_t data_mutex;
extern osEventFlagsId_t led_event_id;

#define DHT11_UPDATED_EVT   (1 << 0)   // 事件标志位

#define dht11_high uapi_gpio_set_val(ONE_WIRE_DQ, GPIO_LEVEL_HIGH)
#define dht11_low uapi_gpio_set_val(ONE_WIRE_DQ, GPIO_LEVEL_LOW)
#define Read_Data uapi_gpio_get_val(ONE_WIRE_DQ)

void DHT11_GPIO_Init_OUT(void);
void DHT11_GPIO_Init_IN(void);
void DHT11_Start(void);
unsigned char DHT11_REC_Byte(void);
unsigned char DHT11_REC_Data(void);
static int read_bit(void);
void read(const char *arg);

#endif