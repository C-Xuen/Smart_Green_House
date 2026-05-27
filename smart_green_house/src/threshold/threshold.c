#include "threshold.h"
#include "soc_osal.h"
#include "osal_debug.h"
#include "nv.h"

/* NV key for threshold storage */
#define NV_KEY_THRESHOLD 0x8001

typedef struct {
    uint8_t  temp;
    uint8_t  humi;
    uint16_t co2;
    uint8_t  soil;
    uint8_t  light;
} th_data_t;

static uint8_t  g_th_temp  = 35;
static uint8_t  g_th_humi  = 85;
static uint16_t g_th_co2   = 1000;
static uint8_t  g_th_soil  = 60;
static uint8_t  g_th_light = 5;

static void th_save(void)
{
    th_data_t data = { g_th_temp, g_th_humi, g_th_co2, g_th_soil, g_th_light };
    uapi_nv_write(NV_KEY_THRESHOLD, (uint8_t *)&data, sizeof(data));
}

static void th_load(void)
{
    th_data_t data;
    uint16_t len = sizeof(data);
    if (uapi_nv_read(NV_KEY_THRESHOLD, sizeof(data), &len, (uint8_t *)&data) == 0 && len == sizeof(data)) {
        g_th_temp  = data.temp;
        g_th_humi  = data.humi;
        g_th_co2   = data.co2;
        g_th_soil  = data.soil;
        g_th_light = data.light;
    }
}

void th_init(void)
{
    th_load();
}

uint8_t  th_temp(void)             { return g_th_temp; }
void     th_temp_set(uint8_t v)    { g_th_temp = v; th_save(); }
uint8_t  th_humi(void)             { return g_th_humi; }
void     th_humi_set(uint8_t v)    { g_th_humi = v; th_save(); }
uint16_t th_co2(void)              { return g_th_co2; }
void     th_co2_set(uint16_t v)    { g_th_co2 = v; th_save(); }
uint8_t  th_soil(void)             { return g_th_soil; }
void     th_soil_set(uint8_t v)    { g_th_soil = v; th_save(); }
uint8_t  th_light(void)            { return g_th_light; }
void     th_light_set(uint8_t v)   { g_th_light = v; th_save(); }
