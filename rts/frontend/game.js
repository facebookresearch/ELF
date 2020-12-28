/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

// 创建画布，仅仅是容器，需要调用脚本来执行绘图
var canvas = document.createElement("canvas");
// 获得绘图环境，在该函数下绘图
var ctx = canvas.getContext("2d");
canvas.width = 10700;           // 1400
canvas.height = 10500;       //1000
var left_frame_width = 1000;
var cell_size = 30;         // 50
var rect_size = 30;       // 50
var unit_size = 32;     // 32
var cell_colors = ['#404040', 'blue', 'black'];
var player_colors = ['blue', 'red', 'yellow'];

// 不同游戏的单位名称
// var unit_names_minirts = ["RESOURCE", "WORKER", "MELEE_ATTACKER", "RANGE_ATTACKER", "BARRACKS", "BASE"];
var unit_names_minirts = [["RESOURCE", "WORKER", "MELEE_ATTACKER1", "RANGE_ATTACKER1", "BARRACKS", "BASE"],["RESOURCE", "WORKER", "MELEE_ATTACKER2", "RANGE_ATTACKER2", "BARRACKS", "BASE"]];
var unit_names_flag = ["FLAG_BASE", "FLAG_ATHLETE", "FLAG"];
var unit_names_td = ["BASE", "WORKER", "RANGE_ATTACKER"];
var x_down = null;
var y_down = null;
var x_curr;
var y_curr;
var dragging = false;
var tick = 0;
var dealer;
var button_left = left_frame_width + 30;
var speed = 0;
var min_speed = -10;
var max_speed = 5;

// FPS 进度条
var range2 = document.createElement("INPUT");
range2.type = "range";
range2.min = min_speed;
range2.max = max_speed;
range2.value = 0;
range2.step = 1;
range2.style.position = "absolute";
range2.style.top = 600;
range2.style.left = left_frame_width + 50;
range2.style.zindex = 2;
range2.style.width = "300px";
range2.style.height = "30px";
range2.oninput = function(){
    var update = this.value - speed;
    speed = this.value;
    if (update > 0) {
        for (var i = 0; i < update;i++){
            send_cmd(tick + ' F');
        }
    }
    if (update < 0) {
        for (var i = 0; i < -update;i++){
            send_cmd(tick + ' W');
        }
    }
}

// document.body.appendChild(range2);

// 按钮
var addButton = function(text, cmd) {
    var button = document.createElement("button");
    button.innerHTML = text;
    button.style.position = "absolute";
    button.style.top = 500;
    button.style.left = button_left;
    button.style.zindex = 2;
    button.style.width = "50px";
    button.style.height = "30px";
    button_left += 100;
    document.body.appendChild(button);
    button.addEventListener ("click", function() {
        if (cmd == "F") {
            console.log(speed);
            if (speed >= max_speed) return;
            else {
                speed = speed + 1;
                range2.value = speed;
            }
        }
        if (cmd == "W") {
            console.log(speed);
            if (speed <= min_speed) return;
            else {
                speed = speed - 1;
                range2.value = speed;
            }
        }
        send_cmd(tick + ' ' + cmd);
    });
};

// 添加几个按键
// addButton("Faster", "F");
// addButton("Slower", "W");
// addButton("Cycle", "C");
// addButton("Pause", "P");

// 第二个进度条
var range1 = document.createElement("INPUT");
range1.type = "range";
range1.min = 0;
range1.max = 100;
range1.value = 0;
range1.step = 1;
range1.style.position = "absolute";
range1.style.top = 700;
range1.style.left = left_frame_width + 50;
range1.style.zindex = 2;
range1.style.width = "300px";
range1.style.height = "30px";
range1.oninput = function(){
    send_cmd(tick + ' S ' + this.value);
}
// document.body.appendChild(range1);

document.body.appendChild(canvas);

// 所有的 send_cmd 都通过此方法将数据发送到服务器
var send_cmd = function(s) {
    //console.log("send_cmd----------------------");
     // 向后台发送命令
  dealer.send(s);
};

// 单击右键执行 f
canvas.oncontextmenu = function (e) {
    e.preventDefault();
};

// 键盘按下
document.addEventListener("keydown", function (e) {
    send_cmd(tick + ' ' + e.key);
}, false);

// 鼠标按下
canvas.addEventListener("mousedown", function (e) {
    if (e.button === 0) {
        var xy0 = convert_xy_back(e.pageX, e.pageY);
        // console.log(xy0);
        if (xy0[0] >350 || xy0[1] > 350) return;
        x_down = e.pageX;
        y_down = e.pageY;
    }
}, false);

// 鼠标松开
canvas.addEventListener("mouseup", function (e) {
    var xy0 = convert_xy_back(e.pageX, e.pageY);
    if (xy0[0] > 350 || xy0[1] > 350) return;     
    if (e.button === 0) {
        var xy = convert_xy_back(x_down, y_down);
        if (dragging && x_down && y_down) {
            send_cmd([tick, 'B', xy[0], xy[1], xy0[0], xy0[1]].join(" "));
        } else {
            send_cmd([tick, 'L', xy[0], xy[1]].join(" "));
        }
        x_down = null;
        y_down = null;
        dragging = false;
    }
    if (e.button === 2) {
        send_cmd([tick, 'R', xy0[0], xy0[1]].join(" "));
    }
}, false);

// 鼠标移动
canvas.addEventListener("mousemove", function (e) {
    if (x_down && y_down) {
        x_curr = e.pageX;
        y_curr = e.pageY;
        var diffx = x_down - x_curr;
        var diffy = y_down - y_curr;
        dragging = (Math.abs(diffx) + Math.abs(diffy) > 10);
    }
}, false);

// game.rts_map
var onMap = function(m) {
    // 一帧是400，counter 最多是 400
    var counter = 0;
    for (y = 0; y < m.height; y++) {
    	for (x = 0; x < m.width; x++){
            // 战争迷雾     m.slots 是一个 400个数字的矩阵，每个数字对应不同的颜色，用来显示战争迷雾
            var color = cell_colors[m.slots[counter]];                    // counter = 0   m.slots = #404040
            var x1 = x * cell_size;                                                           // 一个格子的长是 50
            var y1 = y * cell_size;                                                             // 宽  50
            ctx.beginPath();
            ctx.fillStyle = color;
            ctx.lineWidth = 1;                                              // 格子之间的线宽  1
		    ctx.rect(x1, y1, rect_size, rect_size);     // 在位置 x1，y2 处 创建 长 宽 为 rect_size 的矩形
		    ctx.strokeStyle = 'black';                                                // 矩形 用 black 填充 
		    ctx.stroke();                                                           // 将上述定义绘制出来
		    ctx.fillRect(x1, y1, rect_size, rect_size);             // 绘制已填色的矩形
		    ctx.closePath();
            counter += 1;
    	}
	}

};

var draw_hp = function(bbox, states, font_color, player_color){
    var x1 = bbox[0];
    var y1 = bbox[1];
    var x2 = bbox[2];
    var y2 = bbox[3];
    var hp_ratio = states[0];
    var state_str = states[1];
    var margin = 2;
    ctx.fillStyle = 'black';
    ctx.lineWidth = margin;
     // 开始一条路径
    ctx.beginPath();
        // 绘制一个矩阵
    ctx.rect(x1, y1, x2 - x1, y2 - y1);
    ctx.fillRect(x1, y1, x2 - x1, y2 - y1);
        // 血条边框颜色
    ctx.strokeStyle = player_color;
        // 前面的都是设置，绘图在此处
    ctx.stroke();
    ctx.closePath();
    // 控制血条颜色
    var color = 'green';
    if (hp_ratio <= 0.5) color = 'yellow';
    if (hp_ratio <= 0.2) color = 'red';
    ctx.fillStyle = color;
    ctx.fillRect(x1, y1, Math.floor((x2 - x1) * hp_ratio + 0.5), y2 - y1);

    // 如果单位有名字，则显示名字
    if (state_str){
    	ctx.beginPath();
    	ctx.fillStyle = font_color;
    	ctx.font = "10px Arial";
		ctx.fillText(state_str,x2 + 10, Math.floor((y1 + y2) / 2));
		ctx.closePath();
    }
}

// 单位的绘制实现        isSelected 是否被选中    u 是 unit
var onUnit = function(u, isSelected) {
        // 玩家血条边框颜色
    var player_color = player_colors[u.player_id];          // unit 内部的 player_id，用于区分单位是哪一方的
    var p =  u.p;                                                                            //  单位此刻的位置                                  
    var last_p = u.last_p;                                                              // 上一刻的位置
    var diffx = p.x - last_p.x;                                                          // 用于判断使用哪种图片
    var diffy = p.y - last_p.y;
    var ori = "down";                                                                       // 默认 向下
    // 绘制方向
    if (Math.abs(diffx) > Math.abs(diffy)) {                                // x 的改变 比 y 的大，优先向左或者向右
        if (diffx >= 0) {
            ori = "right";
        } else {
            ori = "left";
        }
    } else {
        if (diffy >= 0) {
            ori = "down"
        } else {
            ori = "up"
        }
    }
    var xy = convert_xy(p.x, p.y);
    // 绘制各种单位的图片
    // u.player_id 判断绘制哪种图片
    draw_sprites(sprites[unit_names_minirts[u.player_id][u.unit_type]], xy[0], xy[1], ori);      // u.unit_type 给数据，unit_names_minirts选择图片类型，sprites 选择对应地址，draw_sprites 加载图片

    var hp_ratio = u.hp / u.max_hp;         // 掉血的比例
    var state_str;
    // 都执行
    if ("cmd" in u) {
        if (u.cmd.cmd[0] != 'I') {
            // 农民的名字   G1  G0 
            state_str = u.cmd.cmd[0] + u.cmd.state;
        }
    }
    var x1 = xy[0] - unit_size / 2;
    var y1 = xy[1] - 27;
    var x2 = x1 + unit_size;
    var y2 = y1 + 5;
        // 绘制血条。后两个参数：字体颜色、血条边框颜色
    draw_hp([x1, y1, x2, y2], [hp_ratio, state_str], 'white', player_color);
    if (isSelected) {
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.rect(x1 - 2, xy[1] - unit_size / 2 - 2, unit_size + 4, unit_size + 4);
        ctx.strokeStyle = player_color;
        ctx.stroke();
        ctx.closePath();
    }
};

// 绘制子弹
var onBullet = function(bullet) {
    var xy = convert_xy(bullet.p.x, bullet.p.y);
        // 加载子弹图片
    draw_sprites(bullets, xy[0], xy[1], bullet.state);
}

// 显示 玩家资源的 文本信息
var onPlayerStats = function(player) {
    if (player.player_id == 2) {
        unit_names_minirts = unit_names_flag;
    }
    var x1 = left_frame_width + 10;
    var y1 = (player.player_id + 1) * 50;
    var label = ["PlayerId", player.player_id, "Resource", player.resource].join(" ");
    ctx.beginPath()
	ctx.fillStyle = "Black";
	ctx.font = "15px Arial";
	ctx.fillText(label, x1, y1);
    ctx.closePath();
}

// 玩家视角
// 该函数并未真正执行，进入该函数是为了从下一帧画面开始，接受某个玩家传来的信息
var onPlayerSeenUnits = function(m) {
    if ("units" in m) {
        // 未执行
        var oldAlpha = ctx.globalAlpha;
        ctx.globalAlpha = 0.3;

        for (var i in m.units) {
            onUnit(m.units[i], false);
        }

        ctx.globalAlpha = oldAlpha;
    }
}

// 空白出显示游戏单位状态
var draw_state = function(u) {
    var x1 = left_frame_width + 10;
    var y1 = 150;
    var x2 = left_frame_width + 100;
    var y2 = 300;
    var title = unit_names_minirts[u.unit_type] + ' ' + u.cmd.cmd + '[' + u.cmd.state + ']';
    ctx.fillText(title, x1, y1);
    y1 += 20;
    var ratio = u.hp / u.max_hp;
    var label = "HP: " + u.hp + " / " + u.max_hp;
    draw_hp([x1, y1, x1 + 100, y1 + 15], [ratio, label], 'black', '');
    for (var i in u.cds) {
        var cd = u.cds[i];
        if (cd.cd > 0) {
            var curr = Math.min(tick - cd.last, cd.cd);
            ratio = curr / cd.cd;
            var label = cd.name + ": " + curr + " / " + cd.cd;
            y1 += 20;
            draw_hp([x1, y1, x1 + 100, y1 + 15], [ratio, label], 'black', '');
        }
    }
}

var convert_xy = function(x, y){
    var xc = x * cell_size + Math.floor(rect_size / 2);
    var yc = y * cell_size + Math.floor(rect_size / 2);

    var x1 = xc - Math.floor(unit_size / 2);
    var y1 = yc + Math.floor(rect_size / 2) - unit_size;
    return [x1, y1];
};

var convert_xy_back = function(x, y){
    var xx = x / cell_size - 0.5;
    var yy = y / cell_size - 0.5;
    return [xx, yy];
};

var load_sprites = function(spec) {
    // 默认行为
    var specReady = false;
    var specImage = new Image();
    specImage.onload = function () {
		specReady = true;
	};
	specImage.src = spec["_file"];
    spec["image"] = specImage;
    return spec;
};

// 绘制图片函数
var draw_sprites = function(spec, px, py, ori) {    // 图片 、
    var image = spec["image"]
    var width = image.width;
    var height = image.height;
    if (!("_sizes" in spec)) {          // 判断是否需要对图片进行切割
        ctx.drawImage(image, px - width / 2, py - height / 2);      // 图片，在画布上放的位置的x，y坐标
    } else {
        // 需要对图像进行切割
        var sw = spec["_sizes"][0];                                                     // 原图像的宽度
        var sh = spec["_sizes"][1];                                                     // 原图像的高度
        var nw = Math.floor(width / sw);
        var nh = Math.floor(height/ sh);
        var xidx = spec[ori][0];
        var yidx = spec[ori][1];                                                                // 
        var cx = xidx[Math.floor(tick / 3) % xidx.length] * sw;     // 开始剪切的x坐标
        var cy = yidx[Math.floor(tick / 3) % yidx.length] * sh;       // 开始剪切的y坐标
        //  px - sw / 2, py - sh / 2        代表图像在画布上的 x   y 坐标
        ctx.drawImage(image, cx, cy, sw, sh, px - sw / 2, py - sh / 2, sw, sh);     // 剪切图像，并在画布上定位被剪切的部分
    }
};

var myrange = function (j, k){
	var n = k - j;
	return Array.from(new Array(n), (x,i) => i + j);
};

// 加载图片
var sprites = {};

// sprites["RANGE_ATTACKER"] = load_sprites({
//     "up" : [myrange(15, 22), [0]],
//     "down": [myrange(15, 22), [1]],
//     "left": [[16], myrange(2, 9)],
//     "right": [[15], myrange(2, 9)],
//     "_file" : "imgs/tiles.png",
//     "_sizes" : [32, 32]
// });

// sprites["MELEE_ATTACKER"] = load_sprites({
//     "up" : [myrange(15, 22), [9]],
//     "down": [myrange(15, 22), [10]],
//     "left": [[20], myrange(2, 9)],
//     "right": [[21], myrange(2, 9)],
//     "_file" : "imgs/tiles.png",
//     "_sizes" : [32, 32]
// });
sprites["RANGE_ATTACKER1"] = load_sprites({
    "up" : [myrange(15, 22), [0]],
    "down": [myrange(15, 22), [1]],
    "left": [[16], myrange(2, 9)],
    "right": [[15], myrange(2, 9)],
    "_file" : "imgs/tile11.png",
    "_sizes" : [32, 32]
});

sprites["MELEE_ATTACKER1"] = load_sprites({
    "up" : [myrange(15, 22), [9]],
    "down": [myrange(15, 22), [10]],
    "left": [[20], myrange(2, 9)],
    "right": [[21], myrange(2, 9)],
    "_file" : "imgs/tile11.png",
    "_sizes" : [32, 32]
});
sprites["RANGE_ATTACKER2"] = load_sprites({
    "up" : [myrange(15, 22), [0]],
    "down": [myrange(15, 22), [1]],
    "left": [[16], myrange(2, 9)],
    "right": [[15], myrange(2, 9)],
    "_file" : "imgs/tiles.png",
    "_sizes" : [32, 32]
});

sprites["MELEE_ATTACKER2"] = load_sprites({
    "up" : [myrange(15, 22), [9]],
    "down": [myrange(15, 22), [10]],
    "left": [[20], myrange(2, 9)],
    "right": [[21], myrange(2, 9)],
    "_file" : "imgs/tiles.png",
    "_sizes" : [32, 32]
});

// 资源
sprites["RESOURCE"] = load_sprites({
    "_file" : "imgs/mineral1.png",
});
// 基地
sprites["BASE"] = load_sprites({
    "_file" : "imgs/base.png"
});
// 兵营
sprites["BARRACKS"] = load_sprites({
    "_file" : "imgs/barracks.png",
});

// 未执行
var targets = load_sprites({
    "attack" : [[11], [6]],
    "move" : [[14], [6]],
    "_file" : "imgs/tiles.png",
    "_sizes" : [32, 32]
});

sprites["WORKER"] = load_sprites({
    "up" : [myrange(9, 12), [7]],
    "down" : [myrange(9, 12), [4]],
    "left" : [myrange(9, 12), [5]],
    "right" : [myrange(9, 12), [6]],
    "_file" : "imgs/People4.png",
    "_sizes" : [32, 32]
});

// 子弹
var bullets = load_sprites({
    "BULLET_READY" : [[7], [0]],        // 子弹类型
    "BULLET_EXPLODE1" : [[0], [0]],     // 三种状态
    "BULLET_EXPLODE2" : [[1], [0]],
    "BULLET_EXPLODE3": [[2], [0]],
    "_file" : "imgs/tiles.png",
    "_sizes" : [32, 32]
});

// capture the flag
sprites["FLAG"] = load_sprites({
    "_file" : "imgs/mineral1.png"
});

sprites["FLAG_ATHLETE"] = load_sprites({
    "up" : [myrange(9, 12), [7]],
    "down" : [myrange(9, 12), [4]],
    "left" : [myrange(9, 12), [5]],
    "right" : [myrange(9, 12), [6]],
    "_file" : "imgs/People4.png",
    "_sizes" : [32, 32]
});

sprites["FLAG_BASE"] = load_sprites({
    "_file" : "imgs/base.png",
});


// 渲染
// 每一次数据都渲染一次
var render = function (game) {
    tick = game.tick;
    // Tick
    // ctx.beginPath()
	// ctx.fillStyle = "Black";
	// ctx.font = "15px Arial";
    // var label = "Tick: " + tick;
    //    // tick 位置
	// ctx.fillText(label, left_frame_width + 10, 20);
    // ctx.closePath();

    // 加载地图
    onMap(game.rts_map);
    if (! game.spectator) {         // game.spectator = true   
        // 切换成玩家视角
        // rts_map 信息变为某个玩家传来的，下一帧画面生效
       onPlayerSeenUnits(game.rts_map);
    }

    var all_units = {};
    var selected = {};
    for (var i in game.players) {       // 两个字典，{player_id: 0; resource: 0}
        onPlayerStats(game.players[i]);         // {player_id: 1; resource: 0}    {player_id: 0; resource: 0}
    }
    for (var i in game.units) {             // 基地，坦克，兵，资源 的信息
        var unit = game.units[i];
        all_units[unit.id] = unit;

        var s_units = game.selected_units;                      // 选中后执行
        var isSelected = (s_units && s_units.indexOf(unit.id) >= 0);           
        if (isSelected) {
            selected[unit.id] = unit;
        }
        
        // 绘制 game.units 信息
        onUnit(unit, isSelected);
    }
    // 用鼠标圈单位的时候执行
    if (dragging && x_down && y_down) {
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.rect(x_down, y_down, x_curr - x_down, y_curr - y_down);
        ctx.strokeStyle = 'green';
        ctx.stroke();
        ctx.closePath();
    }

    // 显示子弹
    for (var i in game.bullets) {
        // console.log(game.bullets[i]);
        onBullet(game.bullets[i]);
    }
    var len = Object.keys(selected).length;     // 选中几个单位
    // 选中某单位时，空白处显示当前选择单位的状态
    // if (len == 1) {
    //     var idx = Object.keys(selected)[0];     // 选中的单位的编号
    //     var unit = selected[idx];
    //     // console.log(unit);      // 获取选中的单位的所有属性
    //     draw_state(unit);       // 在右边空白出显示具体信息
    // }
    // ctx.beginPath();
	// ctx.fillStyle = "Black";
    // ctx.font = "15px Arial";
    // // 选中多个单位时，空白处显示  len + " units"
    // if (len > 1) {
    //     var label = len + " units";
    // 	ctx.fillText(label ,left_frame_width + 50, 200);
    // }
    // var label = "Current FPS is " + Math.floor(50 * Math.pow(1.3, speed));
    // ctx.fillText(label, left_frame_width + 50, 570);
    // if (game.replay_length) {
    //     range1.value = 100 * game.tick / game.replay_length;
    // }

    // var label = "Current progress_percent is " + range1.value;
    // ctx.fillText(label, left_frame_width + 50, 670);
    // ctx.closePath();
};


var main = function () {
        // 建立连接
  dealer = new WebSocket('ws://localhost:8000');
  // 连接建立成功调用的方法
  dealer.onopen = function(event) {
            // 连接成功后输出
    console.log("WS Opened.");
  }

    // 当收到来自服务器的消息时，被调用
  dealer.onmessage = function (message) {
      // message 是 HTTP所有的信息 
      // s 是游戏中的所有信息
    var s = message.data;   
    // 将 s 转换为 json  存储在 game 中
    var game = JSON.parse(s);
    //console.log(game);
        // 在画布内清空一个矩阵，用于渲染游戏
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    render(game);
  };
};

var then = Date.now();
// 开始
main();
