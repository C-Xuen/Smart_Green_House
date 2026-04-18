# 智能温室系统 (Smart Green House)

基于 Hi3861 开发板的温湿度监控系统，支持 DHT11 传感器数据采集、LED 状态指示和 Web 端实时显示。

## 项目概述

本项目实现了一个完整的智能温室监控系统：
- **DHT11 温湿度传感器**：每 3 秒采集一次温湿度数据
- **三色 LED 指示**：绿色呼吸灯（正常）、红色频闪（过高）、蓝色频闪（过低）
- **WiFi 连接**：连接热点并获取 IP 地址
- **HTTP 服务器**：在网页上实时显示温湿度数据

## 目录结构

```
smart_green_house/
├── src/
│   ├── SGH.c           # 主程序入口，创建所有任务
│   ├── dht11.c         # DHT11 传感器驱动
│   ├── LED.c           # LED 控制（PWM 呼吸灯 + 频闪）
│   ├── server.c        # HTTP 服务器
│   ├── wifi_connect.c # WiFi 连接
│   └── button.c       # 按钮驱动（预留）
├── include/
│   ├── dht11.h       # DHT11 头文件及全局变量声明
│   ├── LED.h          # LED 头文件及模式定义
│   ├── wifi_connect.h# WiFi 连接头文件
│   └── ...           # 其他系统头文件
├── CmakeLists.txt     # 构建配置
└── README.md        # 本文档
```

## 硬件连接

| 功能 | 引脚 | 说明 |
|------|------|------|
| DHT11 数据 | GPIO_00 | 单总线通信 |
| 绿色 LED (PWM) | GPIO_02 | PWM 呼吸灯输出 |
| 红色 LED | GPIO_03 | 频闪警报 |
| 蓝色 LED | GPIO_04 | 频闪警报 |

## 代码说明

### 1. SGH.c - 主程序入口

**作用**：系统入口，创建所有 RTOS 任务和同步对象。

**核心代码**：
```c
static void smart_green_house_entry(void)
{
    uapi_gpio_init();
    
    // 创建互斥量保护全局数据
    data_mutex = osMutexNew(NULL);
    // 创建事件标志组
    led_event_id = osEventFlagsNew(NULL);
    
    // 创建 4 个任务
    // 1. WiFi 连接任务
    // 2. DHT11 采集任务
    // 3. LED 控制任务
    // 4. HTTP 服务器任务
}
```

**WiFi 配置**（第 14-15 行）：
```c
#define WIFI_SSID     "ACH"      // 替换为你的 WiFi 名称
#define WIFI_PASSWORD "12345678" // 替换为你的 WiFi 密码
```

### 2. dht11.c - DHT11 传感器驱动

**作用**：读取 DHT11 传感器的温湿度数据，并通过互斥量和事件标志组通知其他任务。

**核心逻辑**：
1. 发送启动信号（拉低 20ms）
2. 等待 ACK 响应
3. 读取 40 位数据
4. 校验和验证
5. 更新全局变量 `g_latest_temp`、`g_latest_humi`
6. 发送事件标志 `DHT11_UPDATED_EVT`

**全局变量**（线程安全）：
```c
float g_latest_temp = 0.0f;  // 温度
float g_latest_humi = 0.0f;  // 湿度
osMutexId_t data_mutex;       // 互斥量
osEventFlagsId_t led_event_id;// 事件标志组
```

**阈值判断**（在 LED.c 中定义）：
- 温度过高：> 28°C
- 温度过低：< 26°C
- 湿度过高：> 75%
- 湿度过低：< 45%

### 3. LED.c - LED 控制

**作用**：根据温湿度数据控制三色 LED 指示灯。

**LED 模式**：
| 模式 | 条件 | 效果 |
|------|------|------|
| GREEN_BREATH | 正常 | 绿色 PWM 呼吸灯 |
| RED_FLASH | 温度>28 或 湿度>75 | 红色 LED 频闪 |
| BLUE_FLASH | 温度<26 或 湿度<45 | 蓝色 LED 频闪 |

**PWM 配置**：
- PWM 通道 2
- 分组 ID 0
- 周期 1ms (1000Hz)

**核心代码**：
```c
void led_control_task(void *argument)
{
    // 等待事件标志
    uint32_t flags = osEventFlagsWait(led_event_id, DHT11_UPDATED_EVT,
                                      osFlagsWaitAny, 10);
    
    if (flags & DHT11_UPDATED_EVT) {
        // 重新计算模式
        led_mode_t new_mode = led_calc_mode(temp_int, humi_int);
    }
    
    // 执行 LED 效果
    switch (current_mode) {
        case LED_MODE_GREEN_BREATH:
            // 呼吸灯效果
            break;
        case LED_MODE_RED_FLASH:
            // 红色频闪
            break;
        case LED_MODE_BLUE_FLASH:
            // 蓝色频闪
            break;
    }
}
```

### 4. server.c - HTTP 服务器

**作用**：提供 Web 页面和 API 接口。

**端点**：
| 路径 | 说明 |
|------|------|
| `/` | 返回 HTML 页面 |
| `/api/temp_humi` | 返回 JSON 格式的温湿度数据 |

**JSON 响应格式**：
```json
{"temperature": "27.2", "humidity": "53.3"}
```

**HTTP 响应头**（手动构建示例）：
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 36
Access-Control-Allow-Origin: *

{"temperature": "27.2", "humidity": "53.3"}
```

**注意**：由于开发板 C 库不支持 `%f` 格式化，JSON 字符串需手动构建。

### 5. wifi_connect.c - WiFi 连接

**作用**：连接 WiFi 热点并通过 DHCP 获取 IP 地址。

**连接流程**：
1. 初始化 WiFi 模块
2. 扫描可用网络
3. 连接指定 SSID
4. 启动 DHCP 获取 IP

### 6. button.c - 按钮驱动（预留）

**作用**：预留的按钮功能，目前未使用。

## 遇到的问题及解决方案

### 1. snprintf 不支持浮点数

**问题**：开发板的 C 库不支持 `%f` 格式化，导致 "Output illegal string" 错误。

**解决**：手动构建 JSON 字符串，逐位添加数字字符：
```c
int ti = (int)temp;       // 整数部分
int td = ((int)(temp * 10)) % 10;  // 小数部分
json[j++] = '0' + (ti / 10);
json[j++] = '0' + (ti % 10);
json[j++] = '.';
json[j++] = '0' + td;
```

### 2. Content-Length 位置问题

**问题**：HTTP 响应头中 Content-Length 字段位置写错（使用 memcpy 后位置偏移），导致浏览器无法正确接收数据。

**解决**：使用字符指针逐字节构建响应头，避免位置偏移错误：
```c
char *r = response;
const char *start = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: ";
while (*start) *r++ = *start++;
*r++ = '0' + (json_len / 10);
*r++ = '0' + (json_len % 10);
*r++ = '\r'; *r++ = '\n';
*r++ = '\r'; *r++ = '\n';
```

### 3. 内存分配函数错误

**问题**：尝试使用 `osal_malloc` 但实际函数名为 `osal_kmalloc`。

**解决**：使用正确的函数名：
```c
char *response = (char *)osal_kmalloc(total_len, OSAL_GFP_ATOMIC);
osal_kfree(response);
```

### 4. HTML 中缺少元素 ID

**问题**：JavaScript 中使用 `getElementById('box')` 但 HTML 中未定义该 ID。

**解决**：为 div 元素添加 id 属性：
```html
<div class="box" id="box">
```

### 5. JavaScript 同步请求问题

**问题**：使用同步 `XMLHttpRequest`（`x.open('GET', '/api/temp_humi', false)`）导致请求可能无法正确返回数据。

**解决**：改用异步请求并使用 `onload` 回调：
```javascript
x.open('GET', '/api/temp_humi', true);
x.onload = function() {
    var d = JSON.parse(x.responseText);
    // 更新页面元素
};
x.send();
```

### 6. HTTP keep-alive 连接不稳定

**问题**：使用 `Connection: keep-alive` 时，多个请求复用同一连接可能导致数据错乱或 ERR。

**解决**：在 HTTP 响应头中添加 `Connection: close`，强制短连��：
```c
const char *start = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: ";
```

### 7. lwip_write 发送不完整

**问题**：直接使用 `memcpy` 构建固定格式字符串后调用 `lwip_write`，可能因缓冲区问题导致响应不完整。

**解决**：先构建完整响应，再一次性发送：
```c
char *r = response;
// ... 构建完整响应头 ...
memcpy(r, html_page, html_len);
int sent = lwip_write(client_fd, response, total_len);
```

## 构建与烧录

### 使用 HiBurn 烧录

1. 打开 HiBurn 工具
2. 选择串口（如 COM9）
3. 加载 .bin 文件
4. 设置波特率 115200
5. 点击 "Burn" 开始烧录

### 串口日志

烧录后，通过串口查看日志：
- `[WIFI] Connecting to SSID: xxx` - WiFi 连接中
- `[WIFI] Connected successfully!` - WiFi 连接成功
- `[OK] T=27.2C H=53.3%` - DHT11 读取成功
- `[HTTP] GET /` - 收到 HTTP 请求
- `[HTTP] GET /api/temp_humi` - 收到 API 请求

## 网页访问

1. 在浏览器中访问 `http://<设备IP>/`
2. 页面会每 3 秒自动刷新温湿度数据
3. 根据温湿度值改变背景色和提示文字

## 任务优先级

| 任务 | 优先级 | 栈大小 |
|------|--------|--------|
| WiFi 连接 | 高 (13) | 8KB |
| DHT11 采集 | 17 | 4KB |
| LED 控制 | 正常 | 4KB |
| HTTP 服务器 | 18 | 8KB |

## 扩展功能

- 添加更多传感器（如土壤湿度、光照强度）
- 支持 MQTT 上传数据到云平台
- 添加 OTA 远程升级功能
- 增加手机 App 支持

## 稳定性注意事项

1. **WiFi 信号**：确保设备与路由器距离不要太远，信号强度影响通信稳定性
2. **短连接**：建议在响应头中使用 `Connection: close`，避免 keep-alive 导致的连接复用问题
3. **强制刷新**：网页显示异常时，使用 `Ctrl+F5` 强制刷新清除缓存
4. **错误处理**：在 JavaScript 中添加 `onerror` 回调处理网络错误