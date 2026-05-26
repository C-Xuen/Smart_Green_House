#ifndef ADCSENSOR_H
#define ADCSENSOR_H

#include <stdint.h>

void adcsensor_init(void);
uint32_t adcsensor_get_voltage_ch2(void);
uint32_t adcsensor_get_voltage_ch3(void);
void adcsensor_sample(void);

#endif
