#include "pinctrl.h"
#include "soc_osal.h"
#include "osal_debug.h"
#include "cmsis_os2.h"
#include "app_init.h"
#include "common_def.h"
#include <stdio.h>
#include <stdarg.h>

#include "adcsensor/adcsensor.h"
#include "oled/oled.h"
#include "dht11/dht11.h"
#include "bee/bee.h"
#include "flame/flame.h"
#include "jw01/jw01.h"
#include "button/button.h"
#include "threshold/threshold.h"
#include "motor/motor.h"
#include "wifi_connect.h"

extern void http_server_task(void *arg);
extern void mqtt_task(void *arg);
extern void led_control_task(void *arg);

osEventFlagsId_t led_event_id = NULL;
osMutexId_t data_mutex = NULL;

#define WIFI_SSID     "ACH"
#define WIFI_PASSWORD "12345678"

static void *wifi_task_func(const char *arg)
{
    (void)arg;
    osal_printk("[WIFI] connecting to %s...\r\n", WIFI_SSID);
    wifi_connect(WIFI_SSID, WIFI_PASSWORD);
    osal_printk("[WIFI] done\r\n");
    return NULL;
}

#define SGH_TASK_STACK_SIZE    0x1000
#define SGH_TASK_PRIO          (osPriority_t)(17)

enum { PAGE_MAIN, PAGE_TEMP, PAGE_HUMI, PAGE_CO2, PAGE_SOIL, PAGE_MAX };

#define CN_WEN   0
#define CN_DU    1
#define CN_SHEN  2
#define CN_GUANG 3
#define CN_ZHAO  4
#define CN_TU    5
#define CN_RANG  6
#define CN_ER    7
#define CN_YANG  8
#define CN_HUA   9
#define CN_TAN  10
#define CN_BAO  11
#define CN_JING 12
#define CN_AN   13
#define CN_QUAN 14
#define CN_YU   15
#define CN_ZHI  16
#define CN_DANG 17
#define CN_QIAN 18
#define CN_SHE  19
#define CN_ZHI2 20
#define CN_NONG 21
#define CN_YE   22
#define CN_DA   23
#define CN_PENG 24
#define CN_JIAN 25
#define CN_CE   26
#define CN_XI   27
#define CN_TONG 28

static void cn(int x, int y, int idx)          { ssd1306_SetCursor(x,y); oled_show_cn(idx); }
static void cn2(int x, int y, int a, int b)     { cn(x,y,a); oled_show_cn(b); }
static void cn_title(int y, int n, int arr[])    { ssd1306_SetCursor(0,y); for(int i=0;i<n;i++) oled_show_cn(arr[i]); }
static void cn_str(int x, int y, int a, int b, const char *fmt, ...) {
    cn2(x,y,a,b);
    char buf[24]; va_list args; va_start(args,fmt);
    vsnprintf(buf,sizeof(buf),fmt,args); va_end(args);
    oled_printf(x+32,y,"%s",buf);
}

static uint8_t soil_percent(uint32_t mv) { int p=(int)mv*100/3300; if(p<0)p=0; if(p>100)p=100; return p; }
static uint8_t light_percent(uint32_t mv) { int p=(3300-(int)mv)*100/3300; if(p<0)p=0; if(p>100)p=100; return p; }
static int g_jw_ok = 0;

static int check_alarm(int dht_ret, int jw_ret)
{
    if(dht_ret==0 && dht11_get_temp_int()>=th_temp()) return 1;
    if(dht_ret==0 && dht11_get_humi_int()>=th_humi()) return 1;
    if(jw_ret==0  && jw01_get_co2()>=th_co2())       return 1;
    if(soil_percent(adcsensor_get_voltage_ch2())>=th_soil()) return 1;
    return 0;
}

static int check_co2_alarm(int jw_ret)
{
    return (jw_ret==0 && jw01_get_co2()>=th_co2());
}

static int check_dht_alarm(int dht_ret)
{
    return (dht_ret==0 && (dht11_get_temp_int()>=th_temp() || dht11_get_humi_int()>=th_humi()));
}

static void draw_main_page(int dht_ret, int jw_ret, int alarm)
{
    int t0[]={CN_NONG,CN_YE,CN_DA,CN_PENG,CN_JIAN,CN_CE,CN_XI,CN_TONG};
    cn_title(0, 8, t0);

    cn2(0, 16, CN_GUANG, CN_ZHAO);
    oled_printf(24, 16, ":%d%%", light_percent(adcsensor_get_voltage_ch3()));

    cn2(75, 16, CN_TU, CN_RANG);
    oled_printf(99, 16, ":%d%%", soil_percent(adcsensor_get_voltage_ch2()));

    ssd1306_SetCursor(0, 32);
    oled_show_cn(CN_WEN); oled_show_cn(CN_SHEN); oled_show_cn(CN_DU);
    if(dht_ret==0)
        oled_printf(36, 32, ":%d.%dC %d.%d%%", dht11_get_temp_int(),dht11_get_temp_deci(),
            dht11_get_humi_int(),dht11_get_humi_deci());
    else
        oled_show_string(36, 32, ":DHT11 Error");

    if(g_jw_ok)
        oled_printf(0, 48, "CO2:%d ppm", jw01_get_co2());
    else
        oled_show_string(0, 48, "CO2: ---");
}

static void *sgh_task(const char *arg)
{
    unused(arg);
    oled_init(); bee_init(); flame_init(); dht11_init();
    adcsensor_init(); jw01_init(); button_init(); th_init(); motor_init();
    osal_printk("[SGH] init OK\r\n");

    int page=PAGE_MAIN, tick=0, dht_ret=-1, jw_ret=-1;

    while(1){
        tick++;
        adcsensor_sample();
        if(tick%10==0)dht_ret=dht11_read();
        if(tick%8==0) { jw_ret=jw01_read(); if(jw_ret==0) g_jw_ok=1; }

        if(btn1_pressed()) page=(page+1)%PAGE_MAX;
        if(btn2_pressed()){
            if(page==PAGE_TEMP)th_temp_set(th_temp()+1);
            if(page==PAGE_HUMI)th_humi_set(th_humi()+1);
            if(page==PAGE_CO2) th_co2_set(th_co2()+50);
            if(page==PAGE_SOIL)th_soil_set(th_soil()+1);
        }
        if(btn3_pressed()){
            if(page==PAGE_TEMP&&th_temp()>0)  th_temp_set(th_temp()-1);
            if(page==PAGE_HUMI&&th_humi()>0)  th_humi_set(th_humi()-1);
            if(page==PAGE_CO2 &&th_co2()>=50) th_co2_set(th_co2()-50);
            if(page==PAGE_SOIL&&th_soil()>0)  th_soil_set(th_soil()-1);
        }

        int alarm=check_alarm(dht_ret,jw_ret);
        int co2_a=check_co2_alarm(jw_ret);
        int dht_a=check_dht_alarm(dht_ret);

        if(flame_detected()){
            bee_on(); fan_on(); pump_on();
        }else if(co2_a || dht_a){
            bee_on(); fan_on(); pump_off();
        }else if(alarm){
            bee_on(); pump_off();
        }else{
            bee_off(); fan_off(); pump_off();
        }

        oled_clear();
        switch(page){
        case PAGE_MAIN: draw_main_page(dht_ret,jw_ret,alarm); break;
        case PAGE_TEMP: {
            int t0[]={CN_WEN,CN_DU,CN_YU,CN_ZHI}; cn_title(0,4,t0);
            cn_str(0,16,CN_DANG,CN_QIAN,":%d.%d C",dht_ret==0?dht11_get_temp_int():0,dht_ret==0?dht11_get_temp_deci():0);
            cn_str(0,32,CN_SHE,CN_ZHI2,":%d C",th_temp());
            oled_show_string(0,48,"Btn2:+ Btn3:-"); break;
        }
        case PAGE_HUMI: {
            int t0[]={CN_SHEN,CN_DU,CN_YU,CN_ZHI}; cn_title(0,4,t0);
            cn_str(0,16,CN_DANG,CN_QIAN,":%d.%d %%",dht_ret==0?dht11_get_humi_int():0,dht_ret==0?dht11_get_humi_deci():0);
            cn_str(0,32,CN_SHE,CN_ZHI2,":%d %%",th_humi());
            oled_show_string(0,48,"Btn2:+ Btn3:-"); break;
        }
        case PAGE_CO2: {
            int t0[]={CN_ER,CN_YANG,CN_HUA,CN_TAN,CN_YU,CN_ZHI}; cn_title(0,6,t0);
            cn_str(0,16,CN_DANG,CN_QIAN,":%d ppm",jw_ret==0?jw01_get_co2():0);
            cn_str(0,32,CN_SHE,CN_ZHI2,":%d ppm",th_co2());
            oled_show_string(0,48,"Btn2:+ Btn3:-"); break;
        }
        case PAGE_SOIL: {
            int t0[]={CN_TU,CN_RANG,CN_SHEN,CN_DU,CN_YU,CN_ZHI}; cn_title(0,6,t0);
            cn_str(0,16,CN_DANG,CN_QIAN,":%d %%",soil_percent(adcsensor_get_voltage_ch2()));
            cn_str(0,32,CN_SHE,CN_ZHI2,":%d %%",th_soil());
            oled_show_string(0,48,"Btn2:+ Btn3:-"); break;
        }
        }
        oled_update(); osal_msleep(100);
    }
    return NULL;
}

static void sgh_entry(void)
{
    osal_printk("[SGH] entry start\r\n");

    /* 初始化互斥量和事件标志 */
    data_mutex = osMutexNew(NULL);
    led_event_id = osEventFlagsNew(NULL);

    /* WiFi 连接任务 */
    osThreadAttr_t wifi_attr={0};
    wifi_attr.name="WiFi"; wifi_attr.stack_size=0x2000; wifi_attr.priority=osPriorityHigh;
    osThreadNew((osThreadFunc_t)wifi_task_func, NULL, &wifi_attr);

    /* HTTP 服务器 */
    osThreadAttr_t http_attr={0};
    http_attr.name="HTTP"; http_attr.stack_size=0x2000; http_attr.priority=(osPriority_t)(18);
    osThreadNew((osThreadFunc_t)http_server_task, NULL, &http_attr);

    /* MQTT 上报 */
    osThreadAttr_t mqtt_attr={0};
    mqtt_attr.name="MQTT"; mqtt_attr.stack_size=0x3000; mqtt_attr.priority=osPriorityNormal;
    osThreadNew((osThreadFunc_t)mqtt_task, NULL, &mqtt_attr);

    /* LED 控制 */
    osThreadAttr_t led_attr={0};
    led_attr.name="LED"; led_attr.stack_size=0x1000; led_attr.priority=osPriorityNormal;
    osThreadNew(led_control_task, NULL, &led_attr);

    /* SGH 主任务（OLED + 传感器 + 蜂鸣器 + 火焰） */
    osThreadAttr_t attr={0};
    attr.name="SGH_Task"; attr.stack_size=SGH_TASK_STACK_SIZE; attr.priority=SGH_TASK_PRIO;
    osThreadNew((osThreadFunc_t)sgh_task,NULL,&attr);
}
app_run(sgh_entry);
