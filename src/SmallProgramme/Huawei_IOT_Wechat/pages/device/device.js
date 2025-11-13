// pages/Huawei_IOT.js

// 设置用户登录参数，此处需要全部修改替换为真实参数
const      domainname = 'GT-xiaomiao123';
const      username = 'Wechat';
const      password = 'xiaomiao123!';
const      projectId = 'b4563241389446e884b8947f22b5d397';
const      deviceId = '690a1b110a9ab42ae58b933d_Weather_Clock_Dev';
const      iamhttps = 'iam.cn-east-3.myhuaweicloud.com';//在华为云连接参数中获取
const      iotdahttps = '827782c6ea.st1.iotda-app.cn-east-3.myhuaweicloud.com';//在华为云连接参数中获取

Page({
    shadowInterval: 2000, // 默认2秒刷新间隔
    /**
     * 页面的初始数据
     */
    data: {
        result:'等待订阅，请点击 订阅按钮',
        temperature: "0",
        lux: "0",
        gpuLoad: "0",
        cpuLoad: "0",
        ramLoad: "0",
        esp32Temp: "0",

        tempProgress: 0,
        luxProgress: 0,
        gpuLoadProgress: 0,
        cpuLoadProgress: 0,
        ramLoadProgress: 0,
        esp32TempProgress: 0,
    },
    
    /**
     * 订阅按钮按下：
     */
    touchBtn_subTopic:function() {
      console.log("订阅按钮按下");
      this.setData({result:'订阅按钮按下'});
      this.gettoken();
    },
    /**
     * 获取设备影子按钮按下：
     */
    touchBtn_getshadow:function()
    {
        console.log("获取设备影子按钮按下");
        this.setData({result:'获取设备影子按钮按下'});
        this.getshadow();
    },
  
    slider4change: function (e) {  //Fre
      const intervalInSeconds = e.detail.value;
      this.shadowInterval = intervalInSeconds * 1000;
      
      // 清除旧定时器并设置新定时器
      clearInterval(this.timer);
      this.timer = setInterval(() => {
        this.getshadow();
      }, this.shadowInterval);

      const formattedInterval = intervalInSeconds.toFixed(1);
      this.setData({result: `刷新频率已设置为 ${formattedInterval} 秒`});
      console.log(`刷新频率已设置为 ${formattedInterval} 秒`);
    },

    /**
     * 获取token
     */
    gettoken:function(){
      console.log("开始获取。。。");//打印完整消息
      var that=this;
      wx.request({
          url: `https://${iamhttps}/v3/auth/tokens`,
          data:`{"auth": { "identity": {"methods": ["password"],"password": {"user": {"name": "${username}","password": "${password}","domain": {"name": "${domainname}"}}}},"scope": {"project": {"name": "cn-north-4"}}}}`,
          method: 'POST',
          header: {'content-type': 'application/json' },
          success: function(res){
            if (res.statusCode && res.statusCode === 201) {
              console.log("获取token成功");
              that.setData({result: 'Token获取成功'});
              var token = res.header['X-Subject-Token'] || res.header['x-subject-token'];
              wx.setStorageSync('token', token);
            } else {
              that.setData({result: `Token获取失败, 状态码: ${res.statusCode}`});
            }
          },
          fail:function(err){
              that.setData({result: 'Token获取失败: 网络请求错误'});
              console.log("获取token失败", err);
          },
          complete: function() {
              console.log("获取token完成");
          } 
      });
    },
    /**
     * 获取设备影子
     */
    getshadow:function(){
        console.log("开始获取影子");
        var that=this;
        var token=wx.getStorageSync('token');
        wx.request({
            url: `https://${iotdahttps}/v5/iot/${projectId}/devices/${deviceId}/shadow`,
            data:'',
            method: 'GET',
            header: {'content-type': 'application/json','X-Auth-Token':token },
            success: function(res){
              if (res.statusCode && res.statusCode === 200 && res.data.shadow) {
                const props = res.data.shadow[0].reported.properties;
                
                const temperature = props.Temperature || 0;
                const lux = props.Lux || 0;
                const gpuLoad = props.GPULoad || 0;
                const cpuLoad = props.CPULoad || 0;
                const ramLoad = props.RAMLoad || 0;
                const esp32Temp = props.ESP32Temp || 0;

                that.setData({
                  temperature: parseFloat(temperature).toFixed(1),
                  lux: parseInt(lux),
                  gpuLoad: parseInt(gpuLoad),
                  cpuLoad: parseInt(cpuLoad),
                  ramLoad: parseFloat(ramLoad).toFixed(1),
                  esp32Temp: parseFloat(esp32Temp).toFixed(1),

                  tempProgress: Math.min(100, (temperature / 50) * 100),
                  luxProgress: Math.max(0, (1 - (lux / 1024)) * 100),
                  gpuLoadProgress: gpuLoad,
                  cpuLoadProgress: cpuLoad,
                  ramLoadProgress: ramLoad,
                  esp32TempProgress: Math.min(100, (esp32Temp / 80) * 100),
                });
              } else {
                that.setData({result: `影子获取失败, 状态码: ${res.statusCode}`});
              }
            },
            fail:function(err){
                that.setData({result: '影子获取失败: 网络请求错误'});
                console.log("获取影子失败", err);
            },
            complete: function() {
                console.log("获取影子完成");
            } 
        });
    },
      
    /**
     * 生命周期函数--监听页面加载
     */
    onLoad(options) {

    },

    /**
     * 生命周期函数--监听页面初次渲染完成
     */
    onReady() {

    },

    /**
     * 生命周期函数--监听页面显示
     */
    onShow() {
      clearInterval(this.timer);
      this.getshadow(); //先获取一次
      this.timer=setInterval(() => {
        this.getshadow()
      }, this.shadowInterval)
    },

    /**
     * 生命周期函数--监听页面隐藏
     */
    onHide() {
      clearInterval(this.timer)
    },

    /**
     * 生命周期函数--监听页面卸载
     */
    onUnload() {
      clearInterval(this.timer)
    },
})