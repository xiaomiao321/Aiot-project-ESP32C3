// 设备连接参数
const      projectId = 'b4563241389446e884b8947f22b5d397';
const      deviceId = '690a1b110a9ab42ae58b933d_Weather_Clock_Dev';
const      iotdahttps = '827782c6ea.st1.iotda-app.cn-east-3.myhuaweicloud.com';

Page({
  data: {
    leds: [
      { id: 0, state: 'off' }, { id: 1, state: 'off' },
      { id: 2, state: 'off' }, { id: 3, state: 'off' },
      { id: 4, state: 'off' }, { id: 5, state: 'off' },
      { id: 6, state: 'off' }, { id: 7, state: 'off' },
      { id: 8, state: 'off' }, { id: 9, state: 'off' }
    ],
    selectedSingleLedIndex: -1, // 记录单选模式下选中的LED

    ledColor: 'rgb(0,0,0)',
    RGBtemp: { R: 0, G: 0, B: 0 },
    result: '点击LED选择单灯，或操作下方按钮'
  },

  onLoad: function (options) {},

  // 简化后的LED点击函数，始终为单选逻辑
  toggleLed: function(e) {
    const clickedIndex = parseInt(e.currentTarget.dataset.index);
    let leds = this.data.leds;
    const isCurrentlySelected = this.data.selectedSingleLedIndex === clickedIndex;

    // 先关闭所有灯的UI状态
    for (let i = 0; i < leds.length; i++) {
      leds[i].state = 'off';
    }

    if (isCurrentlySelected) {
      // 如果点击的是已选中的灯，则取消选中
      this.setData({ leds, selectedSingleLedIndex: -1 });
    } else {
      // 如果点击的是未选中的灯，则选中它
      leds[clickedIndex].state = 'on';
      this.setData({ leds, selectedSingleLedIndex: clickedIndex });
    }
  },

  // --- RGB滑块处理函数 ---
  slider1change: function (e) { this.setData({ 'RGBtemp.R': e.detail.value }); this.updateLedColor(); },
  slider2change: function (e) { this.setData({ 'RGBtemp.B': e.detail.value }); this.updateLedColor(); },
  slider3change: function (e) { this.setData({ 'RGBtemp.G': e.detail.value }); this.updateLedColor(); },

  updateLedColor: function() {
    const { R, G, B } = this.data.RGBtemp;
    const color = `rgb(${R},${G},${B})`;
    this.setData({ ledColor: color });
  },

  // --- 新的按钮处理函数 ---
  setSingleLedColor: function() {
    if (this.data.selectedSingleLedIndex === -1) {
      wx.showToast({ title: '请先点击选择一个LED', icon: 'none' });
      return;
    }
    const { R, G, B } = this.data.RGBtemp;
    const paras = {
      mode: "single",
      index: this.data.selectedSingleLedIndex,
      Red: R,
      Green: G,
      Blue: B
    };
    this.sendRgbCommand(paras);
  },

  setAllLedsColor: function() {
    this.setData({result:'设置所有灯颜色...'});
    const { R, G, B } = this.data.RGBtemp;
    const paras = {
      mode: "all",
      Red: R,
      Green: G,
      Blue: B
    };
    this.sendRgbCommand(paras);
    // UI上点亮所有LED
    const leds = this.data.leds.map(led => ({ ...led, state: 'on' }));
    this.setData({ leds, selectedSingleLedIndex: -1 });
  },

  startRainbowMode: function() {
    this.setData({result:'启动彩虹模式...'});
    this.sendRgbCommand({ mode: "rainbow", speed: 20 });
    // UI上点亮所有LED以表示模式生效
    const leds = this.data.leds.map(led => ({ ...led, state: 'on' }));
    this.setData({ leds, selectedSingleLedIndex: -1 });
  },

  turnAllOff: function() {
    this.setData({result:'全部关闭...'});
    this.sendRgbCommand({ mode: "off" });
    // UI上关闭所有LED
    const leds = this.data.leds.map(led => ({ ...led, state: 'off' }));
    this.setData({ leds, selectedSingleLedIndex: -1 });
  },

  // --- 中央命令发送函数 ---
  sendRgbCommand: function(paras) {
    console.log("开始下发命令。。。");//打印完整消息
    var that = this;
    var token = wx.getStorageSync('token');

    if (!token) {
      that.setData({result: 'Token不存在，请先在设备页获取'});
      wx.showToast({ title: 'Token不存在', icon: 'none' });
      return;
    }

    wx.request({
        url: `https://${iotdahttps}/v5/iot/${projectId}/devices/${deviceId}/commands`,
        data: {
          "service_id": "Property",
          "command_name": "RGB",
          "paras": paras
        },
        method: 'POST', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
        header: {'content-type': 'application/json', 'X-Auth-Token': token }, //请求的header 
        success: function(res){// success
            if (res.statusCode && (res.statusCode === 200 || res.statusCode === 201)) {
              that.setData({result: '命令下发成功'});
              console.log("下发命令成功", res);//打印完整消息
            } else {
              that.setData({result: `命令下发失败, 状态码: ${res.statusCode}, 信息: ${res.data.error_msg || '无'}`});
              console.log("下发命令失败", res);
            }
        },
        fail:function(err){
            // fail
            that.setData({result: '命令下发失败: 网络请求错误'});
            console.log("命令下发失败", err);//打印完整消息
        },
        complete: function() {
            // complete
            console.log("命令下发流程结束");//打印完整消息
        } 
    });
  },
});