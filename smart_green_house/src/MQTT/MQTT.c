#include "MQTTClient.h"   
#include "cJSON.h"  
#include "osal_debug.h"
#include "osal_task.h"
#include "cmsis_os2.h"
#include "dht11/dht11.h"
#include "jw01/jw01.h"
#include "adcsensor/adcsensor.h"
#include "flame/flame.h"
#include "threshold/threshold.h"
#include "LED/LED.h"
#include <string.h>

extern int MQTTClient_init(void);

/* ---------- 从参数生成工具获得的华为云 MQTT 凭证 ---------- */
#define DEVICE_CLIENT_ID "6a12aafe6b6c4d5f8d60913b_H3863_0_0_2026052413"
#define DEVICE_USERNAME  "6a12aafe6b6c4d5f8d60913b_H3863"
#define DEVICE_PASSWORD  "a7eb74c9a5fce43cf56aba277c5190bd4c81bf913b93cf49e733ece07fdd69aa"

#define MQTT_ADDR "tcp://049a9b92ef.iotda-device.cn-south-4.myhuaweicloud.com:1883"
#define DEVICE_ID "6a12aafe6b6c4d5f8d60913b_H3863"

/* ---------- 华为云 IoT 平台预定义 Topic ---------- */
#define TOPIC_PROP_REPORT  "$oc/devices/" DEVICE_ID "/sys/properties/report"
#define TOPIC_CMD_RESPONSE "$oc/devices/" DEVICE_ID "/sys/commands/response/request_id="

static MQTTClient g_mqtt_client = NULL;
static volatile int g_mqtt_connected = 0;

/* 用于暂存 JSON 负载，避免反复分配 */
static char g_payload_buf[512];

/* -------------------------------------------------------------------------- */
/*  设备激活：创建并连接 MQTT 客户端                                            */
/*  使用 华为云 IoT 平台提供的三元组进行认证（ClientId / Username / Password）   */
/* -------------------------------------------------------------------------- */
static int mqtt_device_activate(void)
{
    int rc;

    /* 1. 创建 MQTT 客户端句柄 */
    if (g_mqtt_client == NULL) {
        osal_printk("[MQTT] addr=%s\r\n", MQTT_ADDR);
        osal_printk("[MQTT] cid=%s\r\n", DEVICE_CLIENT_ID);
        rc = MQTTClient_create(&g_mqtt_client, MQTT_ADDR, DEVICE_CLIENT_ID,
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
        if (rc != MQTTCLIENT_SUCCESS) {
            osal_printk("[MQTT] create failed, rc=%d\r\n", rc);
            return -1;
        }
        osal_printk("[MQTT] client created\r\n");
    }

    /* 2. 配置连接参数，填入用户名和密码进行设备认证 */
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 120;          /* 心跳间隔 120 秒 */
    conn_opts.cleansession = 1;                 /* 清除旧会话 */
    conn_opts.username = DEVICE_USERNAME;       /* 设备认证用户名 */
    conn_opts.password = DEVICE_PASSWORD;       /* 设备认证密码 */
    conn_opts.connectTimeout = 10;              /* 连接超时 10 秒 */

    /* 3. 连接华为云 MQTT Broker */
    osal_printk("[MQTT] connecting to broker...\r\n");
    rc = MQTTClient_connect(g_mqtt_client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        osal_printk("[MQTT] connect failed, rc=%d\r\n", rc);
        MQTTClient_destroy(&g_mqtt_client);
        g_mqtt_client = NULL;
        return -1;
    }

    g_mqtt_connected = 1;
    osal_printk("[MQTT] connected! device activated.\r\n");
    return 0;
}

/* -------------------------------------------------------------------------- */
/*  上报设备属性到华为云 IoT 平台                                               */
/*  payload 格式（华为云要求）：                                                 */
/*  {                                                                          */
/*    "services": [{                                                           */
/*      "service_id": "smart_green_house",                                     */
/*      "properties": { "temperature": 26.5, "humidity": 49.2 }               */
/*    }]                                                                       */
/*  }                                                                          */
/* -------------------------------------------------------------------------- */
static int mqtt_report_properties(float temp, float humi, int co2, int soil, int light, int flame)
{
    if (!g_mqtt_connected || g_mqtt_client == NULL) {
        /* 自动重连 */
        if (mqtt_device_activate() != 0) return -1;
    }

    /* 使用 cJSON 构建载荷 */
    cJSON *root = cJSON_CreateObject();
    cJSON *services_arr = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "services", services_arr);

    cJSON *svc = cJSON_CreateObject();
    cJSON_AddItemToArray(services_arr, svc);
    cJSON_AddStringToObject(svc, "service_id", "smart_green_house");

    cJSON *props = cJSON_CreateObject();
    cJSON_AddItemToObject(svc, "properties", props);
    cJSON_AddNumberToObject(props, "temperature", temp);
    cJSON_AddNumberToObject(props, "humidity", humi);
    cJSON_AddNumberToObject(props, "co2", co2);
    cJSON_AddNumberToObject(props, "soil_moisture", soil);
    cJSON_AddNumberToObject(props, "light", light);
    cJSON_AddNumberToObject(props, "flame", flame);
    cJSON_AddNumberToObject(props, "th_temp", th_temp());
    cJSON_AddNumberToObject(props, "th_humi", th_humi());
    cJSON_AddNumberToObject(props, "th_co2",  th_co2());
    cJSON_AddNumberToObject(props, "th_soil", th_soil());
    cJSON_AddNumberToObject(props, "th_light",th_light());

    /* 序列化为 JSON 字符串 */
    char *str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (str == NULL) return -1;

    int str_len = (int)strlen(str);
    if (str_len >= (int)sizeof(g_payload_buf)) {
        /* 如果动态分配的内存太大，截断（正常不会超过512） */
        str_len = sizeof(g_payload_buf) - 1;
    }
    memcpy(g_payload_buf, str, str_len);
    g_payload_buf[str_len] = '\0';
    cJSON_free(str);

    /* 发布到属性上报 Topic */
    int rc = MQTTClient_publish(g_mqtt_client, TOPIC_PROP_REPORT,
                                str_len, g_payload_buf, 0, 0, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        osal_printk("[MQTT] publish failed, rc=%d\r\n", rc);
        return -1;
    }

    osal_printk("[MQTT] reported: %s\r\n", g_payload_buf);
    return 0;
}

/* -------------------------------------------------------------------------- */
/*  MQTT 主任务                                                                */
/*  循环上报温湿度数据到华为云 IoT 平台                                         */
/* -------------------------------------------------------------------------- */
void mqtt_task(void *arg)
{
    (void)arg;

    /* 等待 WiFi 连接完成 */
    osal_msleep(8000);

    /* 初始化 MQTT 库（创建互斥锁等，必须在 create 之前调用） */
    MQTTClient_init();

    /* 设备激活：连接华为云 IoT 平台 */
    while (mqtt_device_activate() != 0) {
        osal_printk("[MQTT] retry in 5s...\r\n");
        osal_msleep(5000);
    }

    /* 循环上报数据 */
    while (1) {
        osal_msleep(5000);

        /* 读取温湿度（新 API） */
        float temp = dht11_get_temp_int() + dht11_get_temp_deci() * 0.1f;
        float humi = dht11_get_humi_int() + dht11_get_humi_deci() * 0.1f;

        /* 保留两位小数 */
        temp  = (float)((int)(temp * 100 + 0.5f)) / 100.0f;
        humi  = (float)((int)(humi * 100 + 0.5f)) / 100.0f;

        /* 读取 CO2 浓度 */
        jw01_read();
        int co2 = (int)jw01_get_co2();

        /* 读取土壤湿度（CH2）和光照（CH3），换算为百分比 */
        adcsensor_sample();
        uint32_t soil_mv  = adcsensor_get_voltage_ch2();
        uint32_t light_mv = adcsensor_get_voltage_ch3();
        int soil  = (int)(soil_mv * 100 / 3300);
        int light = (int)((3300 - light_mv) * 100 / 3300);
        if (soil  < 0) soil = 0;
        if (soil  > 100) soil = 100;
        if (light < 0) light = 0;
        if (light > 100) light = 100;

        /* 读取火焰传感器 */
        int flame = flame_detected() ? 1 : 0;

        /* 上报到华为云 */
        mqtt_report_properties(temp, humi, co2, soil, light, flame);
    }
}
