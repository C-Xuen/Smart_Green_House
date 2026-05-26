#include "pinctrl.h"
#include "gpio.h"
#include "soc_osal.h"
#include "dht11.h"

#define DHT11_PIN    GPIO_00

static uint8_t g_temp_int = 0;
static uint8_t g_temp_deci = 0;
static uint8_t g_humi_int = 0;
static uint8_t g_humi_deci = 0;

static int read_bit(void)
{
    int t = 0;
    while (uapi_gpio_get_val(DHT11_PIN) == GPIO_LEVEL_LOW && t < 200) t++;
    if (t >= 200) return -1;
    
    t = 0;
    while (uapi_gpio_get_val(DHT11_PIN) == GPIO_LEVEL_HIGH && t < 200) t++;
    if (t >= 200) return -1;
    
    return t;
}

int dht11_init(void)
{
    return 0;
}

int dht11_read(void)
{
    int t;
    
    uapi_pin_set_mode(DHT11_PIN, 0);
    uapi_pin_set_pull(DHT11_PIN, PIN_PULL_TYPE_UP);
    uapi_gpio_set_dir(DHT11_PIN, GPIO_DIRECTION_OUTPUT);
    uapi_gpio_set_val(DHT11_PIN, GPIO_LEVEL_LOW);
    osal_msleep(20);
    
    uapi_pin_set_mode(DHT11_PIN, 0);
    uapi_gpio_set_dir(DHT11_PIN, GPIO_DIRECTION_INPUT);
    uapi_pin_set_pull(DHT11_PIN, PIN_PULL_TYPE_DISABLE);
    
    t = 0;
    while (uapi_gpio_get_val(DHT11_PIN) == GPIO_LEVEL_HIGH && t < 300) { t++; osal_udelay(1); }
    if (t >= 300) return -1;
    
    t = 0;
    while (uapi_gpio_get_val(DHT11_PIN) == GPIO_LEVEL_LOW && t < 300) { t++; osal_udelay(1); }
    
    t = 0;
    while (uapi_gpio_get_val(DHT11_PIN) == GPIO_LEVEL_HIGH && t < 300) { t++; osal_udelay(1); }
    
    int bits[40];
    for (int i = 0; i < 40; i++) {
        bits[i] = read_bit();
        if (bits[i] < 0) return -2;
    }
    
    unsigned char data[5] = {0};
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            data[i] = (data[i] << 1) | (bits[i*8+j] > 50 ? 1 : 0);
        }
    }
    
    if (data[0] + data[1] + data[2] + data[3] != data[4]) {
        return -3;
    }
    
    g_humi_int = data[0];
    g_humi_deci = data[1];
    g_temp_int = data[2];
    g_temp_deci = data[3];
    
    return 0;
}

uint8_t dht11_get_temp_int(void) { return g_temp_int; }
uint8_t dht11_get_temp_deci(void) { return g_temp_deci; }
uint8_t dht11_get_humi_int(void) { return g_humi_int; }
uint8_t dht11_get_humi_deci(void) { return g_humi_deci; }
