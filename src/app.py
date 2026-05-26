from flask import Flask, render_template, jsonify, request
import requests, time, json, os

app = Flask(__name__)

# ============ 华为云配置 ============
PROJECT_ID = "019e58e2a10d72658ed091c6b113e751"
DEVICE_ID  = "6a12aafe6b6c4d5f8d60913b_H3863"
IAM_USER   = "iotda_api"
IAM_PASS   = "A6lf$qu8Gc0EOhG4xgoZb5nSkdx6"
DOMAIN     = "hid_1ou0n_f2rvep7jg"

IAM_URL  = "https://iam.myhuaweicloud.com/v3/auth/tokens"

# 三个可能的 HTTPS 端点（逐一尝试）
IOTDA_EPS = [
    "049a9b92ef.iotda-app.cn-south-4.myhuaweicloud.com",
]

_g = {"token": "", "expires": 0}

# ============ IAM 鉴权 ============
def get_token():
    if time.time() < _g["expires"]:
        return _g["token"]
    body = {
        "auth": {
            "identity": {
                "methods": ["password"],
                "password": {"user": {"name": IAM_USER, "password": IAM_PASS, "domain": {"name": DOMAIN}}}
            },
            "scope": {"project": {"id": PROJECT_ID}}
        }
    }
    try:
        r = requests.post(IAM_URL, json=body, timeout=10)
        if r.status_code == 201:
            _g["token"] = r.headers.get("X-Subject-Token", "")
            _g["expires"] = time.time() + 3500
            print("IAM OK")
            return _g["token"]
        print(f"IAM fail: {r.status_code} {r.text[:150]}")
    except Exception as e:
        print(f"IAM err: {e}")
    return ""

# ============ 读设备影子 ============
def query_shadow(ep, token):
    """尝试用一个端点读取设备影子"""
    url = f"https://{ep}/v5/iot/{PROJECT_ID}/devices/{DEVICE_ID}/shadow"
    try:
        r = requests.get(url, headers={"X-Auth-Token": token}, timeout=5)
        if r.status_code == 200:
            props = r.json().get("shadow", [{}])[0]
            props = props.get("reported", {}).get("properties", {})
            print(f"OK via {ep}")
            return props
        print(f"  {ep} -> {r.status_code} {r.text[:100]}")
    except Exception as e:
        print(f"  {ep} -> ERR: {e}")
    return None

def get_sensor_data():
    token = get_token()
    if not token:
        return _default()

    for ep in IOTDA_EPS:
        props = query_shadow(ep, token)
        if props:
            return {
                "temperature": props.get("temperature", 0),
                "humidity":    props.get("humidity", 0),
                "co2":         props.get("co2", 0),
                "light":       props.get("light", 0),
                "soil":        props.get("soil_moisture", 0),
                "fire":        props.get("flame", 0),
                "fan": 0, "heat_fan": 0,
                "light_ctrl": 0, "pump": 0, "alarm": 0,
                "connected": 1,
                "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
            }
    return _default()

def _default():
    return {
        "temperature": 0, "humidity": 0, "co2": 0, "light": 0,
        "soil": 0, "fire": 0, "fan": 0, "heat_fan": 0,
        "light_ctrl": 0, "pump": 0, "alarm": 0, "connected": 0,
        "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
    }

# ============ 阈值+历史+告警 ============
threshold_config = {"temp_min":10,"temp_max":35,"humi_min":30,"humi_max":85,"co2_max":1200,"light_min":5000,"soil_min":20}
HISTORY_FILE = "history.json"
ALARMS_FILE  = "alarms.json"

def sj(f, d):
    with open(f, 'w', encoding='utf-8') as fh: json.dump(d, fh, ensure_ascii=False, indent=2)
def lj(f):
    if not os.path.exists(f): sj(f, [])
    with open(f, encoding='utf-8') as fh: return json.load(fh)

def check_alarm(data):
    alarms = lj(ALARMS_FILE)
    new = []
    if data["temperature"] > threshold_config["temp_max"] or data["temperature"] < threshold_config["temp_min"]:
        new.append(f"温度异常：{data['temperature']}℃")
    if data["humidity"] > threshold_config["humi_max"] or data["humidity"] < threshold_config["humi_min"]:
        new.append(f"湿度异常：{data['humidity']}%")
    for msg in new: alarms.insert(0, {"time": data["timestamp"], "msg": msg})
    sj(ALARMS_FILE, alarms[:50])

def save_history(data):
    h = lj(HISTORY_FILE)
    h.append({"time": data["timestamp"], "temp": data["temperature"], "humi": data["humidity"]})
    sj(HISTORY_FILE, h[-30:])

# ============ Flask 路由 ============
@app.route('/')
def index():
    return render_template('index.html', threshold=threshold_config)

@app.route('/api/data')
def api_data():
    d = get_sensor_data(); check_alarm(d); save_history(d); return jsonify(d)

@app.route('/api/history')
def api_history(): return jsonify(lj(HISTORY_FILE))

@app.route('/api/alarms')
def api_alarms(): return jsonify(lj(ALARMS_FILE))

@app.route('/api/threshold')
def api_threshold(): return jsonify(threshold_config)

@app.route('/api/threshold', methods=['POST'])
def set_threshold():
    global threshold_config
    d = request.get_json()
    for k in threshold_config:
        if k in d: threshold_config[k] = float(d[k])
    return jsonify({"code": 0, "msg": "ok"})

@app.route('/api/control', methods=['POST'])
def control_device(): return jsonify({"code": 0, "msg": "ok"})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
