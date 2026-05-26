Page({
  data: {
    connected: false, timestamp: "",
    Temp: 0, Hum: 0, Light: 0, CO2: 0, Soil: 0,
    fan: false, heat_fan: false, light_ctrl: false, pump: false,
    temp_min: 10, temp_max: 35, humi_min: 30, humi_max: 85,
    co2_max: 1200, light_min: 5000, soil_min: 20,
    alarms: [],
    useCloud: false,  // true=云函数, false=直连Flask
  },

  API_BASE: "http://192.168.52.186:5000",   // 改成你 PC 的局域网 IP

  onLoad() {
    this.loadThreshold();
    this.fetchAll();
    setInterval(() => { this.fetchAll(); }, 3000);
  },

  /* 获取数据 */
  fetchAll() {
    if (this.data.useCloud) {
      this.fetchViaCloud();
    } else {
      this.fetchViaHTTP();
    }
  },

  /* 方式 A：直连 Flask（内网调试用） */
  fetchViaHTTP() {
    var that = this;
    wx.request({
      url: that.API_BASE + "/api/data", method: "GET", timeout: 5000,
      success: function (res) {
        if (res.statusCode == 200 && res.data) {
          var d = res.data;
          that.setData({
            connected: true, timestamp: d.timestamp,
            Temp: d.temperature, Hum: d.humidity,
            CO2: d.co2, Light: d.light, Soil: d.soil,
            fan: d.fan||false, heat_fan: d.heat_fan||false,
            light_ctrl: d.light_ctrl||false, pump: d.pump||false,
          });
        }
      }
    });
  },

  /* 方式 B：云函数（公网可用）*/
  fetchViaCloud() {
    var that = this;
    wx.cloud.callFunction({
      name: "getDeviceData",
      success: function (res) {
        if (res.result) {
          var d = res.result;
          that.setData({
            connected: true, timestamp: "",
            Temp: d.temperature, Hum: d.humidity,
            CO2: d.co2 || 0, Light: d.light || 0, Soil: d.soil || 0,
          });
        }
      },
    });
  },

  /* ===== 阈值 ===== */
  loadThreshold() {
    var that = this;
    wx.request({
      url: that.API_BASE + "/api/threshold", method: "GET", timeout: 3000,
      success: function (res) {
        if (res.statusCode == 200 && res.data) that.setData(res.data);
      }
    });
  },

  onMinInput: function (e) {
    var key = e.currentTarget.dataset.key;
    var obj = {}; obj[key] = e.detail.value; this.setData(obj);
  },
  onMaxInput: function (e) {
    var key = e.currentTarget.dataset.key;
    var obj = {}; obj[key] = e.detail.value; this.setData(obj);
  },
  onSingleInput: function (e) {
    var key = e.currentTarget.dataset.key;
    var obj = {}; obj[key] = e.detail.value; this.setData(obj);
  },

  saveThreshold() {
    var that = this;
    var data = {
      temp_min: parseFloat(that.data.temp_min),
      temp_max: parseFloat(that.data.temp_max),
      humi_min: parseFloat(that.data.humi_min),
      humi_max: parseFloat(that.data.humi_max),
      co2_max: parseInt(that.data.co2_max),
      light_min: parseInt(that.data.light_min),
      soil_min: parseInt(that.data.soil_min),
    };
    wx.request({
      url: that.API_BASE + "/api/threshold", method: "POST",
      header: { "Content-Type": "application/json" },
      data: data,
      success: function () { wx.showToast({ title: "已保存", icon: "success" }); },
    });
  },

  /* ===== 设备控制 ===== */
  onCtrl: function (e) {
    var dev = e.currentTarget.dataset.dev;
    var val = e.detail.value;
    wx.request({
      url: this.API_BASE + "/api/control", method: "POST",
      header: { "Content-Type": "application/json" },
      data: { device: dev, status: val ? 1 : 0 },
    });
  },
});
