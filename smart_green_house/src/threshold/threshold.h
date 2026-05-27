#ifndef TH_H
#define TH_H

#include <stdint.h>

void th_init(void);

uint8_t  th_temp(void);
void     th_temp_set(uint8_t v);
uint8_t  th_humi(void);
void     th_humi_set(uint8_t v);
uint16_t th_co2(void);
void     th_co2_set(uint16_t v);
uint8_t  th_soil(void);
void     th_soil_set(uint8_t v);
uint8_t  th_light(void);
void     th_light_set(uint8_t v);

#endif
