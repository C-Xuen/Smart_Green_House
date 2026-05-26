#include "threshold.h"

static uint8_t  g_th_temp = 38;
static uint8_t  g_th_humi = 80;
static uint16_t g_th_co2  = 1000;
static uint8_t  g_th_soil = 60;

void th_init(void) {}

uint8_t  th_temp(void)             { return g_th_temp; }
void     th_temp_set(uint8_t v)    { g_th_temp = v; }
uint8_t  th_humi(void)             { return g_th_humi; }
void     th_humi_set(uint8_t v)    { g_th_humi = v; }
uint16_t th_co2(void)              { return g_th_co2; }
void     th_co2_set(uint16_t v)    { g_th_co2 = v; }
uint8_t  th_soil(void)             { return g_th_soil; }
void     th_soil_set(uint8_t v)    { g_th_soil = v; }
