const cloud = require('wx-server-sdk');
cloud.init({ env: cloud.DYNAMIC_CURRENT_ENV });

// 华为云 IAM 认证 —— 获取 token
async function getToken() {
  const res = await cloud.callFunction({  // 或使用云函数内置 HTTP
    // 微信云函数环境不支持直接调外部 API，需要用 cloud.openapi 或 http 模块
  });
}

// 读设备影子
exports.main = async () => {
  const https = require('https');
  
  const body = JSON.stringify({
    auth: {
      identity: {
        methods: ["password"],
        password: {
          user: {
            name: "iotda_api",              // 改
            password: "你的密码",            // 改
            domain: { name: "你的账号名" }   // 改
          }
        }
      },
      scope: { project: { id: "你的项目ID" } }
    }
  });

  // 步骤 1：获取 IAM token
  const token = await new Promise((resolve, reject) => {
    const req = https.request({
      hostname: "iam.myhuaweicloud.com",
      path: "/v3/auth/tokens",
      method: "POST",
      headers: { "Content-Type": "application/json" }
    }, res => {
      resolve(res.headers["x-subject-token"]);
    });
    req.on("error", reject);
    req.write(body);
    req.end();
  });

  // 步骤 2：读设备影子
  const data = await new Promise((resolve, reject) => {
    const req = https.request({
      hostname: "049a9b92ef.iotda-device.cn-south-4.myhuaweicloud.com",
      path: "/v5/iot/你的项目ID/devices/6a12aafe6b6c4d5f8d60913b_H3863/shadow",
      method: "GET",
      headers: { "X-Auth-Token": token }
    }, res => {
      let body = "";
      res.on("data", chunk => body += chunk);
      res.on("end", () => resolve(JSON.parse(body)));
    });
    req.on("error", reject);
    req.end();
  });

  const props = data.shadow[0].reported.properties;
  return {
    temperature: props.temperature || 0,
    humidity: props.humidity || 0,
    co2: props.co2 || 0,
    light: props.light || 0,
    soil: props.soil_moisture || 0,
    connected: true,
  };
};
