// 设备连接参数 (从led_control.js复制)
const      projectId = 'b4563241389446e884b8947f22b5d397';
const      deviceId = '690a1b110a9ab42ae58b933d_Weather_Clock_Dev';
const      iotdahttps = '827782c6ea.st1.iotda-app.cn-east-3.myhuaweicloud.com';

Page({
  data: {
    time: '00:00',
    weekArray: ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday'],
    weekIndex: 0,
    isOn: true,
    result: '请设置闹钟并发送命令'
  },

  onLoad: function (options) {
    // 初始化时间为当前时间
    const now = new Date();
    const hour = String(now.getHours()).padStart(2, '0');
    const minute = String(now.getMinutes()).padStart(2, '0');
    this.setData({
      time: `${hour}:${minute}`
    });
  },

  bindTimeChange: function (e) {
    this.setData({
      time: e.detail.value
    });
  },

  bindWeekChange: function (e) {
    this.setData({
      weekIndex: e.detail.value
    });
  },

  bindSwitchChange: function (e) {
    this.setData({
      isOn: e.detail.value
    });
  },

  sendCommand: function () {
    console.log("开始下发闹钟命令。。。");
    var that = this;
    var token = wx.getStorageSync('token');

    if (!token) {
      that.setData({result: 'Token不存在，请先在设备页获取'});
      wx.showToast({ title: 'Token不存在', icon: 'none' });
      return;
    }

    const [hourStr, minuteStr] = this.data.time.split(':');
    const Hour = parseInt(hourStr, 10);
    const Minute = parseInt(minuteStr, 10);
    const Week = this.data.weekArray[this.data.weekIndex];
    const On = this.data.isOn;

    const paras = {
      Hour: Hour,
      Minute: Minute,
      Week: Week,
      On: On
    };

    wx.request({
        url: `https://${iotdahttps}/v5/iot/${projectId}/devices/${deviceId}/commands`,
        data: {
          "service_id": "Property",
          "command_name": "Alarm",
          "paras": paras
        },
        method: 'POST',
        header: {'content-type': 'application/json', 'X-Auth-Token': token },
        success: function(res){
            if (res.statusCode && (res.statusCode === 200 || res.statusCode === 201)) {
              that.setData({result: '闹钟命令下发成功'});
              console.log("下发闹钟命令成功", res);
            } else {
              that.setData({result: `闹钟命令下发失败, 状态码: ${res.statusCode}, 信息: ${res.data.error_msg || '无'}`});
              console.log("下发闹钟命令失败", res);
            }
        },
        fail:function(err){
            that.setData({result: '闹钟命令下发失败: 网络请求错误'});
            console.log("闹钟命令下发失败", err);
        },
        complete: function() {
            console.log("闹钟命令下发流程结束");
        }
    });
  }
});