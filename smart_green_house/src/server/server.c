#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"
#include "lwip/err.h"
#include "osal_debug.h"
#include "osal_task.h"
#include "cmsis_os2.h"
#include "soc_osal.h"
#include <string.h>

#include "dht11/dht11.h"
#include "jw01/jw01.h"
#include "adcsensor/adcsensor.h"
#include "motor/motor.h"
#include "light/light.h"
#include "threshold/threshold.h"

#define HTTP_PORT 80

/* 简易整数解析器 */
static int parse_int(const char *s) {
    while (*s && (*s < '0' || *s > '9')) s++;
    int v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}

void http_server_task(void *arg)
{
    (void)arg;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char recv_buf[1024];

    osal_msleep(3000);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { osal_printk("HTTP socket failed\r\n"); return; }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(HTTP_PORT);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        osal_printk("HTTP bind failed\r\n"); lwip_close(server_fd); return;
    }
    if (listen(server_fd, 5) < 0) {
        osal_printk("HTTP listen failed\r\n"); lwip_close(server_fd); return;
    }
    osal_printk("HTTP API started on port %d\r\n", HTTP_PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) continue;

        int len = recv(client_fd, recv_buf, sizeof(recv_buf) - 1, 0);
        if (len > 0) {
            recv_buf[len] = '\0';

            /* ===== GET /api/temp_humi ===== */
            if (strstr(recv_buf, "GET /api/temp_humi") != NULL) {
                int ti = dht11_get_temp_int(), td = dht11_get_temp_deci();
                int hi = dht11_get_humi_int(), hd = dht11_get_humi_deci();
                int co2 = (int)jw01_get_co2();
                int soil_mv = (int)adcsensor_get_voltage_ch2();
                int light_mv = (int)adcsensor_get_voltage_ch3();
                int soil = soil_mv * 100 / 3300;
                int light = (3300 - light_mv) * 100 / 3300;
                if (soil < 0) soil = 0;
                if (soil > 100) soil = 100;
                if (light < 0) light = 0;
                if (light > 100) light = 100;
                if (co2 > 9999) co2 = 9999;

                char json[256];
                int j = 0;
                json[j++]='{';
                json[j++]='"';json[j++]='t';json[j++]='e';json[j++]='m';json[j++]='p';json[j++]='e';json[j++]='r';json[j++]='a';json[j++]='t';json[j++]='u';json[j++]='r';json[j++]='e';json[j++]='"';json[j++]=':';
                json[j++]='0'+(ti/10);json[j++]='0'+(ti%10);json[j++]='.';json[j++]='0'+td;
                json[j++]=',';
                json[j++]='"';json[j++]='h';json[j++]='u';json[j++]='m';json[j++]='i';json[j++]='d';json[j++]='i';json[j++]='t';json[j++]='y';json[j++]='"';json[j++]=':';
                json[j++]='0'+(hi/10);json[j++]='0'+(hi%10);json[j++]='.';json[j++]='0'+hd;
                json[j++]=',';
                json[j++]='"';json[j++]='c';json[j++]='o';json[j++]='2';json[j++]='"';json[j++]=':';
                json[j++]='0'+(co2/1000);json[j++]='0'+((co2/100)%10);json[j++]='0'+((co2/10)%10);json[j++]='0'+(co2%10);
                json[j++]=',';
                json[j++]='"';json[j++]='s';json[j++]='o';json[j++]='i';json[j++]='l';json[j++]='_';json[j++]='m';json[j++]='o';json[j++]='i';json[j++]='s';json[j++]='t';json[j++]='u';json[j++]='r';json[j++]='e';json[j++]='"';json[j++]=':';
                json[j++]='0'+(soil/1000);json[j++]='0'+((soil/100)%10);json[j++]='0'+((soil/10)%10);json[j++]='0'+(soil%10);
                json[j++]=',';
                json[j++]='"';json[j++]='l';json[j++]='i';json[j++]='g';json[j++]='h';json[j++]='t';json[j++]='"';json[j++]=':';
                json[j++]='0'+(light/1000);json[j++]='0'+((light/100)%10);json[j++]='0'+((light/10)%10);json[j++]='0'+(light%10);
                json[j++]=',';
                json[j++]='"';json[j++]='t';json[j++]='h';json[j++]='_';json[j++]='t';json[j++]='e';json[j++]='m';json[j++]='p';json[j++]='"';json[j++]=':';
                json[j++]='0'+(th_temp()/10);json[j++]='0'+(th_temp()%10);
                json[j++]=',';
                json[j++]='"';json[j++]='t';json[j++]='h';json[j++]='_';json[j++]='h';json[j++]='u';json[j++]='m';json[j++]='i';json[j++]='"';json[j++]=':';
                json[j++]='0'+(th_humi()/10);json[j++]='0'+(th_humi()%10);
                json[j++]=',';
                json[j++]='"';json[j++]='t';json[j++]='h';json[j++]='_';json[j++]='c';json[j++]='o';json[j++]='2';json[j++]='"';json[j++]=':';
                { int v=th_co2(); json[j++]='0'+(v/1000);json[j++]='0'+((v/100)%10);json[j++]='0'+((v/10)%10);json[j++]='0'+(v%10); }
                json[j++]=',';
                json[j++]='"';json[j++]='t';json[j++]='h';json[j++]='_';json[j++]='s';json[j++]='o';json[j++]='i';json[j++]='l';json[j++]='"';json[j++]=':';
                json[j++]='0'+(th_soil()/10);json[j++]='0'+(th_soil()%10);
                json[j++]=',';
                json[j++]='"';json[j++]='t';json[j++]='h';json[j++]='_';json[j++]='l';json[j++]='i';json[j++]='g';json[j++]='h';json[j++]='t';json[j++]='"';json[j++]=':';
                json[j++]='0'+(th_light()/10);json[j++]='0'+(th_light()%10);
                json[j++]='}'; json[j]=0;
                int json_len = j;
                osal_printk("[JSON] %s\n", json);

                char *resp = (char *)osal_kmalloc(json_len + 100, OSAL_GFP_ATOMIC);
                if (resp) {
                    char *r = resp;
                    *r++='H';*r++='T';*r++='T';*r++='P';*r++='/';*r++='1';*r++='.';*r++='1';*r++=' ';*r++='2';*r++='0';*r++='0';*r++=' ';*r++='O';*r++='K';*r++='\r';*r++='\n';
                    *r++='C';*r++='o';*r++='n';*r++='n';*r++='e';*r++='c';*r++='t';*r++='i';*r++='o';*r++='n';*r++=':';*r++=' ';*r++='c';*r++='l';*r++='o';*r++='s';*r++='e';*r++='\r';*r++='\n';
                    *r++='C';*r++='o';*r++='n';*r++='t';*r++='e';*r++='n';*r++='t';*r++='-';*r++='T';*r++='y';*r++='p';*r++='e';*r++=':';*r++=' ';*r++='a';*r++='p';*r++='p';*r++='l';*r++='i';*r++='c';*r++='a';*r++='t';*r++='i';*r++='o';*r++='n';*r++='/';*r++='j';*r++='s';*r++='o';*r++='n';*r++='\r';*r++='\n';
                    *r++='C';*r++='o';*r++='n';*r++='t';*r++='e';*r++='n';*r++='t';*r++='-';*r++='L';*r++='e';*r++='n';*r++='g';*r++='t';*r++='h';*r++=':';*r++=' ';
                    *r++='0'+(json_len/100);*r++='0'+((json_len/10)%10);*r++='0'+(json_len%10);
                    *r++='\r';*r++='\n';*r++='\r';*r++='\n';
                    memcpy(r, json, json_len);
                    lwip_write(client_fd, resp, (r - resp) + json_len);
                    osal_kfree(resp);
                }
            }
            /* ===== POST /api/control ===== */
            else if (strstr(recv_buf, "POST /api/control") != NULL) {
                osal_printk("[CTRL] %s\n", recv_buf);
                char *body = strstr(recv_buf, "\r\n\r\n");
                if (body) {
                    body += 4;
                    int on = (strstr(body, "\"status\": 1") || strstr(body, "\"status\":1"));
                    if (strstr(body, "\"fan\""))       { if(on) fan_on();   else fan_off(); }
                    if (strstr(body, "\"pump\""))      { if(on) pump_on();  else pump_off(); }
                    if (strstr(body, "\"light_ctrl\"")){ if(on) light_on(); else light_off(); }
                }
                const char *ok = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\n{}";
                send(client_fd, ok, strlen(ok), 0);
            }
            /* ===== POST /api/threshold ===== */
            else if (strstr(recv_buf, "POST /api/threshold") != NULL) {
                char *body = strstr(recv_buf, "\r\n\r\n");
                if (body) {
                    body += 4;
                    if (strstr(body, "temp_max")) th_temp_set(parse_int(strstr(body, "temp_max\":") + 10));
                    if (strstr(body, "humi_max")) th_humi_set(parse_int(strstr(body, "humi_max\":") + 10));
                    if (strstr(body, "co2_max"))  th_co2_set(parse_int(strstr(body, "co2_max\":") + 9));
                    if (strstr(body, "soil_min")) th_soil_set(parse_int(strstr(body, "soil_min\":") + 11));
                    if (strstr(body, "light_min")) th_light_set(parse_int(strstr(body, "light_min\":") + 12));
                }
                const char *ok = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\n{}";
                send(client_fd, ok, strlen(ok), 0);
            }
            else {
                const char *nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                send(client_fd, nf, strlen(nf), 0);
            }
        }
        lwip_close(client_fd);
    }
}
