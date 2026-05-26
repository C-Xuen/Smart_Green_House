#include "pinctrl.h"
#include "soc_osal.h"
#include "uart.h"
#include "osal_debug.h"
#include "jw01.h"

#define JW01_UART_BUS    UART_BUS_2
#define JW01_TX_PIN      7
#define JW01_RX_PIN      8
#define JW01_PIN_MODE    1
#define JW01_BAUDRATE    9600
#define JW01_DATABITS    3
#define JW01_STOPBITS    1
#define JW01_PARITY      0
#define JW01_RX_BUF_SIZE 16

static uint8_t g_rx_buf[JW01_RX_BUF_SIZE];
static uint16_t g_co2_ppm = 0;

void jw01_init(void)
{
    uapi_pin_set_mode(JW01_TX_PIN, JW01_PIN_MODE);
    uapi_pin_set_mode(JW01_RX_PIN, JW01_PIN_MODE);

    uart_attr_t attr = {
        .baud_rate = JW01_BAUDRATE,
        .data_bits = JW01_DATABITS,
        .stop_bits = JW01_STOPBITS,
        .parity = JW01_PARITY
    };

    uart_pin_config_t pins = {
        .tx_pin = JW01_TX_PIN,
        .rx_pin = JW01_RX_PIN,
        .cts_pin = PIN_NONE,
        .rts_pin = PIN_NONE
    };

    uart_buffer_config_t buffer_config = {
        .rx_buffer = g_rx_buf,
        .rx_buffer_size = JW01_RX_BUF_SIZE
    };

    int ret = uapi_uart_init(JW01_UART_BUS, &pins, &attr, NULL, &buffer_config);
    osal_printk("[JW01] init, ret=%d\r\n", ret);
}

int jw01_read(void)
{
    uint8_t frame[6];

    for (int i = 0; i < 3; i++) {
        if (uapi_uart_read(JW01_UART_BUS, &frame[0], 1, 0) == 1 && frame[0] == 0x2C) {
            goto got_header;
        }
    }
    return -1;

got_header:
    for (int i = 1; i < 6; i++) {
        for (int t = 0; t < 30; t++) {
            if (uapi_uart_read(JW01_UART_BUS, &frame[i], 1, 0) == 1) break;
        }
    }

    uint8_t sum = frame[0] + frame[1] + frame[2] + frame[3] + frame[4];
    if (sum != frame[5]) return -2;

    g_co2_ppm = frame[1] * 256 + frame[2];
    return 0;
}

uint16_t jw01_get_co2(void)
{
    return g_co2_ppm;
}
