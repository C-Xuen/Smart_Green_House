#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

int dht11_init(void);
int dht11_read(void);
uint8_t dht11_get_temp_int(void);
uint8_t dht11_get_temp_deci(void);
uint8_t dht11_get_humi_int(void);
uint8_t dht11_get_humi_deci(void);

#endif
