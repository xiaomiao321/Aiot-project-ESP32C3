// 设备连接参数
const      projectId = 'b4563241389446e884b8947f22b5d397';
const      deviceId = '690a1b110a9ab42ae58b933d_Weather_Clock_Dev';
const      iotdahttps = '827782c6ea.st1.iotda-app.cn-east-3.myhuaweicloud.com';

Page({
  data: {
    songs: [
      { id: 0, name: "爱你", artist: "王心凌" },
      { id: 1, name: "爱情讯息", artist: "陈奕迅" },
      { id: 2, name: "啊朋友再见", artist: "无" },
      { id: 3, name: "把回忆拼好给你", artist: "王贰浪" },
      { id: 4, name: "保卫黄河", artist: "冼星海" },
      { id: 5, name: "不得不爱", artist: "潘玮柏" },
      { id: 6, name: "不再犹豫", artist: "Beyond" },
      { id: 7, name: "猜不透", artist: "丁当" },
      { id: 8, name: "卡萨布兰卡", artist: "Bertie Higgins" },
      { id: 9, name: "测谎", artist: "薛之谦" },
      { id: 10, name: "成都", artist: "赵雷" },
      { id: 11, name: "虫儿飞", artist: "黑鸭子" },
      { id: 12, name: "Counting Stars", artist: "OneRepublic" },
      { id: 13, name: "倒数", artist: "蔡依林" },
      { id: 14, name: "春娇与志明", artist: "五月天" },
      { id: 15, name: "大海", artist: "张雨生" },
      { id: 16, name: "稻香", artist: "周杰伦" },
      { id: 17, name: "东方红", artist: "李有源" },
      { id: 18, name: "东方之珠", artist: "罗大佑" },
      { id: 19, name: "东西", artist: "薛之谦" },
      { id: 20, name: "梦中的婚礼", artist: "Richard Clayderman" },
      { id: 21, name: "多远都要在一起", artist: "邓紫棋" },
      { id: 22, name: "反方向的钟", artist: "周杰伦" },
      { id: 23, name: "Fate", artist: "Ludwig van Beethoven" },
      { id: 24, name: "Five Hundred Miles", artist: "The Innocence Mission" },
      { id: 25, name: "For Elise", artist: "Ludwig van Beethoven" },
      { id: 26, name: "For Ya", artist: "来自网络" },
      { id: 27, name: "富士山下", artist: "陈奕迅" },
      { id: 28, name: "告白气球", artist: "周杰伦" },
      { id: 29, name: "刚好遇见你", artist: "李玉刚" },
      { id: 30, name: "歌唱祖国", artist: "王莘" },
      { id: 31, name: "光辉岁月", artist: "Beyond" },
      { id: 32, name: "过火", artist: "张信哲" },
      { id: 33, name: "海阔天空", artist: "Beyond" },
      { id: 34, name: "荷塘月色", artist: "凤凰传奇" },
      { id: 35, name: "红豆", artist: "王菲" },
      { id: 36, name: "红色高跟鞋", artist: "蔡健雅" },
      { id: 37, name: "后来", artist: "刘若英" },
      { id: 38, name: "花海", artist: "周杰伦" },
      { id: 39, name: "江南", artist: "林俊杰" },
      { id: 40, name: "开始懂了", artist: "孙燕姿" },
      { id: 41, name: "可不可以", artist: "张紫豪" },
      { id: 42, name: "兰亭序", artist: "周杰伦" },
      { id: 43, name: "梁祝", artist: "何占豪 陈钢" },
      { id: 44, name: "绿色", artist: "陈雪凝" },
      { id: 45, name: "明天会更好", artist: "群星" },
      { id: 46, name: "南方姑娘", artist: "赵雷" },
      { id: 47, name: "你若三冬", artist: "无" },
      { id: 48, name: "怒放的生命", artist: "汪峰" },
      { id: 49, name: "Ode to Joy", artist: "Ludwig van Beethoven" },
      { id: 50, name: "朋友", artist: "周华健" },
      { id: 51, name: "琵琶行", artist: "奇然" },
      { id: 52, name: "平凡之路", artist: "朴树" },
      { id: 53, name: "起风了", artist: "买辣椒也用券" },
      { id: 54, name: "七里香", artist: "周杰伦" },
      { id: 55, name: "青花瓷", artist: "周杰伦" },
      { id: 56, name: "晴天", artist: "周杰伦" },
      { id: 57, name: "Radetzky Marsch", artist: "Johann Strauss I" },
      { id: 58, name: "若水三千", artist: "无" },
      { id: 59, name: "山丘", artist: "李宗盛" },
      { id: 60, name: "Shape Of You", artist: "Ed Sheeran" },
      { id: 61, name: "十年", artist: "陈奕迅" },
      { id: 62, name: "送别", artist: "李叔同" },
      { id: 63, name: "素颜", artist: "许嵩" },
      { id: 64, name: "Sugar", artist: "Maroon 5" },
      { id: 65, name: "Take Me Hand", artist: "无" },
      { id: 66, name: "天黑黑", artist: "孙燕姿" },
      { id: 67, name: "童年", artist: "罗大佑" },
      { id: 68, name: "Turkish March", artist: "Wolfgang Amadeus Mozart" },
      { id: 69, name: "我怀念的", artist: "孙燕姿" },
      { id: 70, name: "我是一只小小鸟", artist: "赵传" },
      { id: 71, name: "喜欢你", artist: "Beyond" },
      { id: 72, name: "下个路口见", artist: "李宇春" },
      { id: 73, name: "小苹果", artist: "筷子兄弟" },
      { id: 74, name: "心墙", artist: "郭静" },
      { id: 75, name: "修炼爱情", artist: "林俊杰" },
      { id: 76, name: "演员", artist: "薛之谦" },
      { id: 77, name: "一生有你", artist: "水木年华" },
      { id: 78, name: "隐形的翅膀", artist: "张韶涵" },
      { id: 79, name: "有点甜", artist: "汪苏泷" },
      { id: 80, name: "遇见", artist: "孙燕姿" },
      { id: 81, name: "夜曲", artist: "周杰伦" },
      { id: 82, name: "鸳鸯戏", artist: "来自网络" },
      { id: 83, name: "这世界那么多人", artist: "莫文蔚" },
      { id: 84, name: "真的爱你", artist: "Beyond" },
      { id: 85, name: "追光者", artist: "岑宁儿" },
      { id: 86, name: "最后一页", artist: "江语晨" },
      { id: 87, name: "Windows XP", artist: "Microsoft" }
    ],
    selectedSongIndex: 0,
    uiModes: ["Full", "Lite", "No"],
    selectedUiModeIndex: 0,
    result: '请选择一首歌曲并发送命令',
  },

  onLoad: function (options) {},

  selectSong: function(e) {
    const index = e.currentTarget.dataset.index;
    this.setData({
      selectedSongIndex: index
    });
  },

  bindPickerChange: function (e) {
    console.log('picker发送选择改变，携带值为', e.detail.value);
    this.setData({
      selectedUiModeIndex: e.detail.value
    });
  },

  sendCommand: function () {
    const songIndex = this.data.selectedSongIndex;
    const uiMode = this.data.uiModes[this.data.selectedUiModeIndex];

    const commandData = {
      "service_id": "Property",
      "command_name": "play_song",
      "paras": {
        "Song_index": songIndex,
        "UI": uiMode
      }
    };

    console.log("开始下发命令。。。");//打印完整消息
    var that = this;
    var token = wx.getStorageSync('token');

    if (!token) {
      // In a real app, you might want to navigate to the device page to get a token
      // For now, just show a toast.
      wx.showToast({ title: 'Token不存在', icon: 'none' });
      that.setData({result: 'Token不存在，请先在设备页获取'});
      return;
    }

    wx.request({
        url: `https://${iotdahttps}/v5/iot/${projectId}/devices/${deviceId}/commands`,
        data: commandData,
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
  }
});