#include "pinctrl.h"
#include "soc_osal.h"
#include "adc.h"
#include "adc_porting.h"
#include "osal_debug.h"
#include "adcsensor.h"

static uint32_t g_voltage_ch2 = 0;
static uint32_t g_voltage_ch3 = 0;

static void adcsensor_callback(uint8_t ch, uint32_t *buffer, uint32_t length, bool *next)
{
    UNUSED(next);
    if (length > 0) {
        if (ch == ADC_CHANNEL_2) {
            g_voltage_ch2 = buffer[length - 1];
        } else if (ch == ADC_CHANNEL_3) {
            g_voltage_ch3 = buffer[length - 1];
        }
    }
}

void adcsensor_init(void)
{
    uapi_adc_init(ADC_CLOCK_500KHZ);
    uapi_adc_power_en(AFE_SCAN_MODE_MAX_NUM, true);
    osal_printk("[ADC] init done\r\n");
}

void adcsensor_sample(void)
{
    adc_scan_config_t config = {
        .type = 0,
        .freq = 1,
    };

    uapi_adc_auto_scan_ch_enable(ADC_CHANNEL_2, config, adcsensor_callback);
    uapi_adc_auto_scan_ch_disable(ADC_CHANNEL_2);

    uapi_adc_auto_scan_ch_enable(ADC_CHANNEL_3, config, adcsensor_callback);
    uapi_adc_auto_scan_ch_disable(ADC_CHANNEL_3);
}

uint32_t adcsensor_get_voltage_ch2(void)
{
    return g_voltage_ch2;
}

uint32_t adcsensor_get_voltage_ch3(void)
{
    return g_voltage_ch3;
}
