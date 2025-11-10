// pages/Huawei_IOT.js

//设置温度值和湿度值 
var Temp = null;
var Phot = null;
var RED = '';
var GREEN = '';
var BLUE = '';
var RGBtemp = [0, 0, 0, 0, 0];
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
    //设置温度值和湿度值 
        temperature: "",
        photores: "",
        ledColor: 'rgb(0,0,0)',
        tempProgress: 0,
        photProgress: 0,
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
    /**
     * 设备命令下发按钮按下：
     */
    touchBtn_setCommand:function()
    {
        console.log("设备命令下发按钮按下");
        this.setData({result:'设备命令下发按钮按下，正在下发。。。'});
        this.setCommand(1);
    },  
    
    /**
     * 配置四个滑动条的功能：
     */
    slider1change: function (e) {  //RED
      RGBtemp[1] = e.detail.value;
      this.updateLedColor();
      console.log(`需要发送的RGB数据分别为：`, 'R:', RGBtemp[1], 'G:', RGBtemp[3], 'B:', RGBtemp[2], 'fre:', RGBtemp[4]);
      console.log(`slider1发生change事件，携带值为`, e.detail.value);
    },
  
    slider2change: function (e) {  //BLUE
      RGBtemp[2] = e.detail.value;
      this.updateLedColor();
      console.log(`需要发送的RGB数据分别为：`, 'R:', RGBtemp[1], 'G:', RGBtemp[3], 'B:', RGBtemp[2], 'fre:', RGBtemp[4]);
      console.log(`slider2发生change事件，携带值为`, e.detail.value);
    },
    slider3change: function (e) { //GREEN
      RGBtemp[3] = e.detail.value;
      this.updateLedColor();
      console.log(`需要发送的RGB数据分别为：`, 'R:', RGBtemp[1], 'G:', RGBtemp[3], 'B:', RGBtemp[2], 'fre:', RGBtemp[4]);
      console.log(`slider3发生change事件，携带值为`, e.detail.value);
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

    updateLedColor: function() {
      const r = RGBtemp[1];
      const g = RGBtemp[3];
      const b = RGBtemp[2];
      const color = `rgb(${r},${g},${b})`;
      this.setData({ ledColor: color });
    },

    /**
     * 开关灯按钮：
     */
    onClickOpen() {
      console.log("开灯按钮按下");
      this.setData({result:'开灯按钮按下，正在下发。。。'});
      this.setCommand(1);
    },
    onClickOff() {
      console.log("关灯按钮按下");
      this.setData({result:'关灯按钮按下，正在下发。。。'});
      this.setCommand(0);
    },

    /**
     * 获取token
     */
    gettoken:function(){
        console.log("开始获取。。。");//打印完整消息
        var that=this;  //这个很重要，在下面的回调函数中由于异步问题不能有效修改变量，需要用that获取
        wx.request({
            url: `https://${iamhttps}/v3/auth/tokens`,
            data:`{"auth": { "identity": {"methods": ["password"],"password": {"user": {"name": "${username}","password": "${password}","domain": {"name": "${domainname}"}}}},"scope": {"project": {"name": "cn-north-4"}}}}`,
            method: 'POST', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
            header: {'content-type': 'application/json' }, // 请求的 header 
            success: function(res){// success
              if (res.statusCode && res.statusCode === 201) {
                console.log("获取token成功");//打印完整消息
                that.setData({result: 'Token获取成功'});
                var token='';
                token=JSON.stringify(res.header['X-Subject-Token']);//解析消息头的token
                token=token.replaceAll("\"", "");
                wx.setStorageSync('token',token);//把token写到缓存中,以便可以随时随地调用
              } else {
                that.setData({result: `Token获取失败, 状态码: ${res.statusCode}`});
              }
            },
            fail:function(err){
                // fail
                that.setData({result: 'Token获取失败: 网络请求错误'});
                console.log("获取token失败", err);//打印完整消息
            },
            complete: function() {
                // complete
                console.log("获取token完成");//打印完整消息
            } 
        });
    },
    /**
     * 获取设备影子
     */
    getshadow:function(){
        console.log("开始获取影子");//打印完整消息
        var that=this;  //这个很重要，在下面的回调函数中由于异步问题不能有效修改变量，需要用that获取
        var token=wx.getStorageSync('token');//读缓存中保存的token
        wx.request({
            url: `https://${iotdahttps}/v5/iot/${projectId}/devices/${deviceId}/shadow`,
            data:'',
            method: 'GET', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
            header: {'content-type': 'application/json','X-Auth-Token':token }, //请求的header 
            success: function(res){// success
              if (res.statusCode && res.statusCode === 200) {
                var shadow=JSON.stringify(res.data.shadow[0].reported.properties);
                console.log('设备影子数据：'+shadow);
                Temp=JSON.stringify(res.data.shadow[0].reported.properties.Temperature);
                Phot=JSON.stringify(res.data.shadow[0].reported.properties.Lux);
                // 更新温度和光照的进度条
                const tempProgress = Math.min(100, (parseFloat(Temp) / 50) * 100); // 假设温度范围0-50度
                const photProgress = Math.max(0, (1 - (parseFloat(Phot) / 1024)) * 100); // 反向计算：值越大（越暗），进度越低。并确保不低于0。 

                that.setData({temperature:Temp, photores:Phot, tempProgress: tempProgress, photProgress: photProgress});
              } else {
                that.setData({result: `影子获取失败, 状态码: ${res.statusCode}`});
              }
            },
            fail:function(err){
                // fail
                that.setData({result: '影子获取失败: 网络请求错误'});
                console.log("获取影子失败", err);//打印完整消息
            },
            complete: function() {
                // complete
                console.log("获取影子完成");//打印完整消息
            } 
        });
    },
    /**
     * 设备命令下发
     */
    setCommand:function(key){
        console.log("开始下发命令。。。");//打印完整消息
        var that=this;  //这个很重要，在下面的回调函数中由于异步问题不能有效修改变量，需要用that获取
        var token=wx.getStorageSync('token');//读缓存中保存的token
        wx.request({
            url: `https://${iotdahttps}/v5/iot/${projectId}/devices/${deviceId}/commands`,
            // data:'{"service_id": "Arduino","command_name": "RGB","paras": { "RED":100,"GREEN":100, "BLUE":100,"Switch":1}}',
            data:`{"service_id": "Arduino","command_name": "RGB","paras": { "RED": ${RGBtemp[1]},"GREEN":${RGBtemp[2]}, "BLUE":${RGBtemp[3]},"Switch":${key}}}`,
            method: 'POST', // OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
            header: {'content-type': 'application/json','X-Auth-Token':token }, //请求的header 
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
      // 切换页面时，确保清除旧的定时器，防止创建多个定时器实例
      clearInterval(this.timer);

      this.getshadow(); //先获取一次
      //设置定时器
      this.timer=setInterval(() => {
        this.getshadow()
      }, this.shadowInterval)
    },

    /**
     * 生命周期函数--监听页面隐藏
     */
    onHide() {
      //当页面隐藏时，清除定时器
      clearInterval(this.timer)
    },

    /**
     * 生命周期函数--监听页面卸载
     */
    onUnload() {
      //当页面关闭时，清除定时器
      clearInterval(this.timer)
    },

    /**
     * 页面相关事件处理函数--监听用户下拉动作
     */
    onPullDownRefresh() {

    },

    /**
     * 页面上拉触底事件的处理函数
     */
    onReachBottom() {

    },

    /**
     * 用户点击右上角分享
     */
    onShareAppMessage() {

    }
})