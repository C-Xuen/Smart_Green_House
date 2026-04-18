#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"
#include "lwip/err.h"
#include "debug/osal_debug.h"
#include "schedule/osal_task.h"
#include "cmsis_os2.h"
#include "soc_osal.h"
#include <string.h>

#include "dht11.h"


extern osMutexId_t data_mutex;

#define HTTP_PORT 80
#define RESP_BUF_SIZE 4096

static const char HTML_PAGE[] = 
"<!DOCTYPE html>"
"<html lang=\"zh\">"
"<head><meta charset=\"UTF-8\"><title>温湿度监控</title>"
"<style>body{font-family:system-ui;background:#FFFACD}.box{background:#2e7d32;color:#fff;padding:30px;border-radius:20px;max-width:500px;margin:50px auto;text-align:center}.val{font-size:40px}</style>"
"</head>"
"<body onload=\"load()\">"
"<div class=\"box\" id=\"box\">"
"<h2>温湿度监控</h2>"
"<div>温度: <span class=\"val\" id=\"temp\">--</span>°C</div>"
"<div>湿度: <span class=\"val\" id=\"humi\">--</span>%</div>"
"<div id=\"hint\">加载中...</div>"
"</div>"
"<script>"
"function load(){"
"var x=new XMLHttpRequest();"
"x.open('GET','/api/temp_humi',true);"
"x.onload=function(){"
"document.getElementById('hint').innerText='OK: '+x.responseText;"
"var d=JSON.parse(x.responseText);"
"document.getElementById('temp').innerText=d.temperature;"
"document.getElementById('humi').innerText=d.humidity;"
"};"
"x.onerror=function(){document.getElementById('hint').innerText='ERR';};"
"x.send();"
"setTimeout(load,3000);"
"}"
"</script>"
"</body>"
"</html>";

void http_server_task(void *arg)
{
    (void)arg;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char recv_buf[256];

    osal_msleep(3000);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        osal_printk("HTTP socket create failed\r\n");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(HTTP_PORT);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        osal_printk("HTTP bind failed\r\n");
        lwip_close(server_fd);
        return;
    }

    if (listen(server_fd, 5) < 0) {
        osal_printk("HTTP listen failed\r\n");
        lwip_close(server_fd);
        return;
    }
    osal_printk("HTTP server started on port %d\r\n", HTTP_PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) continue;

        int len = recv(client_fd, recv_buf, sizeof(recv_buf)-1, 0);
        if (len > 0) {
            recv_buf[len] = '\0';
            osal_printk("[HTTP] %s\n", recv_buf);
            
            if (strstr(recv_buf, "GET /api/temp_humi") != NULL) {
                osal_printk("[API] Handling /api/temp_humi request\n");
                float temp, humi;
                
                osal_printk("[API] acquiring mutex...\n");
                osMutexAcquire(data_mutex, osWaitForever);
                temp = g_latest_temp;
                humi = g_latest_humi;
                osMutexRelease(data_mutex);
                osal_printk("[API] released mutex, raw: temp=%d, humi=%d\n", (int)temp, (int)humi);

                int ti = (int)temp;
                int td = ((int)(temp * 10)) % 10;
                int hi = (int)humi;
                int hd = ((int)(humi * 10)) % 10;
                
                osal_printk("[API] temp=%d.%d, humi=%d.%d\n", ti, td, hi, hd);
                
                char json[48];
                int j = 0;
                json[j++] = '{';
                json[j++] = '\"';json[j++]='t';json[j++]='e';json[j++]='m';json[j++]='p';json[j++]='e';json[j++]='r';json[j++]='a';json[j++]='t';json[j++]='u';json[j++]='r';json[j++]='e';json[j++]='\"';json[j++]=':';
                json[j++] = '0' + (ti / 10); json[j++] = '0' + (ti % 10); json[j++] = '.'; json[j++] = '0' + td;
                json[j++] = ',';
                json[j++] = '\"';json[j++]='h';json[j++]='u';json[j++]='m';json[j++]='i';json[j++]='d';json[j++]='i';json[j++]='t';json[j++]='y';json[j++]='\"';json[j++] = ':';
                json[j++] = '0' + (hi / 10); json[j++] = '0' + (hi % 10); json[j++] = '.'; json[j++] = '0' + hd;
                json[j++] = '}';
                json[j] = 0;
                int json_len = j;

                osal_printk("[API] json=%s, len=%d\n", json, json_len);

                char *response = (char *)osal_kmalloc(json_len + 100, OSAL_GFP_ATOMIC);
                if (response) {
                    char *resp = response;
                    *resp++ = 'H'; *resp++='T'; *resp++='T'; *resp++='P'; *resp++='/';
                    *resp++='1'; *resp++='.'; *resp++='1'; *resp++=' '; *resp++='2';
                    *resp++='0'; *resp++='0'; *resp++=' '; *resp++='O'; *resp++='K';
                    *resp++='\r'; *resp++='\n';
                    *resp++='C'; *resp++='o'; *resp++='n'; *resp++='n'; *resp++='e';
                    *resp++='c'; *resp++='t'; *resp++='i'; *resp++='o'; *resp++='n';
                    *resp++=':'; *resp++=' '; *resp++='c'; *resp++='l'; *resp++='o';
                    *resp++='s'; *resp++='e'; *resp++='\r'; *resp++='\n';
                    *resp++='C'; *resp++='o'; *resp++='n'; *resp++='t'; *resp++='e';
                    *resp++='n'; *resp++='t'; *resp++='-'; *resp++='T'; *resp++='y';
                    *resp++='p'; *resp++='e'; *resp++=':'; *resp++=' ';
                    *resp++='a'; *resp++='p'; *resp++='p'; *resp++='l'; *resp++='i';
                    *resp++='c'; *resp++='a'; *resp++='t'; *resp++='i'; *resp++='o';
                    *resp++='n'; *resp++='/'; *resp++='j'; *resp++='s'; *resp++='o';
                    *resp++='n'; *resp++='\r'; *resp++='\n';
                    *resp++='C'; *resp++='o'; *resp++='n'; *resp++='t'; *resp++='e';
                    *resp++='n'; *resp++='t'; *resp++='-'; *resp++='L'; *resp++='e';
                    *resp++='n'; *resp++='g'; *resp++='t'; *resp++='h'; *resp++=':';
                    *resp++=' '; 
                    *resp++ = '0' + (json_len / 10);
                    *resp++ = '0' + (json_len % 10);
                    *resp++='\r'; *resp++='\n'; *resp++='\r'; *resp++='\n';
                    int hlen = resp - response;
                    memcpy(resp, json, json_len);
                    int total = hlen + json_len;
                    int sent = lwip_write(client_fd, response, total);
                    osal_printk("[API] hlen=%d, json_len=%d, sent=%d\n", hlen, json_len, sent);
                    osal_kfree(response);
                }
            } else if (strstr(recv_buf, "GET /") != NULL) {
                int html_len = strlen(HTML_PAGE);
                char *response = (char *)osal_kmalloc(html_len + 200, OSAL_GFP_ATOMIC);
                if (response) {
                    char *r = response;
                    const char *start = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: ";
                    while (*start) *r++ = *start++;
                    *r++ = '0' + (html_len / 1000);
                    *r++ = '0' + ((html_len / 100) % 10);
                    *r++ = '0' + ((html_len / 10) % 10);
                    *r++ = '0' + (html_len % 10);
                    *r++ = '\r'; *r++ = '\n';
                    *r++ = '\r'; *r++ = '\n';
                    int hlen = r - response;
                    memcpy(r, HTML_PAGE, html_len);
                    lwip_write(client_fd, response, html_len + hlen);
                    osal_kfree(response);
                }
            } else {
                const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                send(client_fd, not_found, strlen(not_found), 0);
            }
        }
        lwip_close(client_fd);
    }
}