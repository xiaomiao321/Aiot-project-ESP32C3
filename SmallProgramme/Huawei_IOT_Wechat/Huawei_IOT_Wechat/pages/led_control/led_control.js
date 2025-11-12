// 设备连接参数
const      projectId = 'b4563241389446e884b8947f22b5d397';
const      deviceId = '690a1b110a9ab42ae58b933d_Weather_Clock_Dev';
const      iotdahttps = '827782c6ea.st1.iotda-app.cn-east-3.myhuaweicloud.com';

// HSL转RGB函数
function hslToRgb(h, s, l) {
  let r, g, b;
  if (s == 0) {
    r = g = b = l; 
  } else {
    const hue2rgb = (p, q, t) => {
      if (t < 0) t += 1;
      if (t > 1) t -= 1;
      if (t < 1 / 6) return p + (q - p) * 6 * t;
      if (t < 1 / 2) return q;
      if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
      return p;
    };
    const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    const p = 2 * l - q;
    r = hue2rgb(p, q, h + 1 / 3);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1 / 3);
  }
  return `rgb(${Math.round(r * 255)}, ${Math.round(g * 255)}, ${Math.round(b * 255)})`;
}


Page({
  data: {
    leds: [
      { id: 0, state: 'off', style: '' }, { id: 1, state: 'off', style: '' },
      { id: 2, state: 'off', style: '' }, { id: 3, state: 'off', style: '' },
      { id: 4, state: 'off', style: '' }, { id: 5, state: 'off', style: '' },
      { id: 6, state: 'off', style: '' }, { id: 7, state: 'off', style: '' },
      { id: 8, state: 'off', style: '' }, { id: 9, state: 'off', style: '' }
    ],
    selectedSingleLedIndex: -1, // 记录单选模式下选中的LED

    ledColor: 'rgb(0,0,0)',
    RGBtemp: { R: 0, G: 0, B: 0 },
    result: '点击LED选择单灯，或操作下方按钮'
  },
  rainbowTimer: null, // 彩虹效果定时器
  rainbowCycle: 0, // 彩虹效果周期计数

  onLoad: function (options) {},
  onUnload: function() {
    // 页面卸载时清除定时器
    if (this.rainbowTimer) {
      clearInterval(this.rainbowTimer);
      this.rainbowTimer = null;
    }
  },

  // 清除彩虹动画
  clearRainbowAnimation() {
    if (this.rainbowTimer) {
      clearInterval(this.rainbowTimer);
      this.rainbowTimer = null;
    }
  },

  // 简化后的LED点击函数，始终为单选逻辑
  toggleLed: function(e) {
    this.clearRainbowAnimation();
    const clickedIndex = parseInt(e.currentTarget.dataset.index);
    let leds = this.data.leds;
    const isCurrentlySelected = this.data.selectedSingleLedIndex === clickedIndex;

    // 先关闭所有灯的UI状态并清除样式
    for (let i = 0; i < leds.length; i++) {
      leds[i].state = 'off';
      leds[i].style = '';
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
    this.clearRainbowAnimation();
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
    // 更新单个灯的颜色
    let leds = this.data.leds;
    leds[this.data.selectedSingleLedIndex].style = `background-color: rgb(${R},${G},${B});`;
    this.setData({leds});
  },

  setAllLedsColor: function() {
    this.clearRainbowAnimation();
    this.setData({result:'设置所有灯颜色...'});
    const { R, G, B } = this.data.RGBtemp;
    const paras = {
      mode: "all",
      Red: R,
      Green: G,
      Blue: B
    };
    this.sendRgbCommand(paras);
    // UI上点亮所有LED并设置颜色
    const leds = this.data.leds.map(led => ({ 
      ...led, 
      state: 'on',
      style: `background-color: rgb(${R},${G},${B});`
    }));
    this.setData({ leds, selectedSingleLedIndex: -1 });
  },

  startRainbowMode: function() {
    this.setData({result:'启动彩虹模式...'});
    this.sendRgbCommand({ mode: "rainbow", speed: 20 });
    
    this.clearRainbowAnimation(); // 先清除旧的定时器
    this.rainbowCycle = 0; // 重置计数器

    this.rainbowTimer = setInterval(() => {
      const leds = this.data.leds.map((led, i) => {
        const hue = ((i * (360 / this.data.leds.length)) + this.rainbowCycle) % 360;
        const color = hslToRgb(hue / 360, 1, 0.5);
        return {
          ...led,
          state: 'on',
          style: `background-color: ${color};`
        };
      });
      this.setData({ leds });
      this.rainbowCycle = (this.rainbowCycle + 5) % 360; // 调整此值可改变速度
    }, 50); // 调整此值可改变刷新率
    
    this.setData({ selectedSingleLedIndex: -1 });
  },

  turnAllOff: function() {
    this.clearRainbowAnimation();
    this.setData({result:'全部关闭...'});
    this.sendRgbCommand({ mode: "off" });
    // UI上关闭所有LED并清除样式
    const leds = this.data.leds.map(led => ({ ...led, state: 'off', style: '' }));
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
        method: 'POST',
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