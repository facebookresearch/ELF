/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

// Create the canvas
var canvas = document.createElement("canvas");
var ctx = canvas.getContext("2d");
canvas.width = 1400;
canvas.height = 1000;
var left_frame_width = 1000;
var cell_size = 50;
var rect_size = 50;
var unit_size = 32;
var cell_colors = ['#404040', 'blue', 'black'];
var player_colors = ['blue', 'red', 'yellow']

var unit_names_minirts = ["RESOURCE", "WORKER", "MELEE_ATTACKER", "RANGE_ATTACKER", "BARRACKS", "BASE"];
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

document.body.appendChild(range2);

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

addButton("Faster", "F");
addButton("Slower", "W");
addButton("Cycle", "C");
addButton("Pause", "P");

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
document.body.appendChild(range1);

document.body.appendChild(canvas);

var send_cmd = function(s) {
  dealer.send(s);
};

canvas.oncontextmenu = function (e) {
    e.preventDefault();
};

document.addEventListener("keydown", function (e) {
    send_cmd(tick + ' ' + e.key);
}, false);

canvas.addEventListener("mousedown", function (e) {
    if (e.button === 0) {
        var xy0 = convert_xy_back(e.pageX, e.pageY);
        if (xy0[0] > 20 || xy0[1] > 20) return;
        x_down = e.pageX;
        y_down = e.pageY;
    }
}, false);

canvas.addEventListener("mouseup", function (e) {
    var xy0 = convert_xy_back(e.pageX, e.pageY);
    if (xy0[0] > 20 || xy0[1] > 20) return;
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

canvas.addEventListener("mousemove", function (e) {
    if (x_down && y_down) {
        x_curr = e.pageX;
        y_curr = e.pageY;
        var diffx = x_down - x_curr;
        var diffy = y_down - y_curr;
        dragging = (Math.abs(diffx) + Math.abs(diffy) > 10);
    }
}, false);


var onMap = function(m) {
    var counter = 0;
    for (y = 0; y < m.height; y++) {
    	for (x = 0; x < m.width; x++){
    		var color = cell_colors[m.slots[counter]];
            var x1 = x * cell_size;
            var y1 = y * cell_size;
            ctx.beginPath();
            ctx.fillStyle = color;
            ctx.lineWidth = 1;
		    ctx.rect(x1, y1, rect_size, rect_size);
		    ctx.strokeStyle = 'black';
		    ctx.stroke();
		    ctx.fillRect(x1, y1, rect_size, rect_size);
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
    ctx.beginPath();
    ctx.rect(x1, y1, x2 - x1, y2 - y1);
    ctx.fillRect(x1, y1, x2 - x1, y2 - y1);
    ctx.strokeStyle = player_color;
    ctx.stroke();
    ctx.closePath();
    var color = 'green';
    if (hp_ratio <= 0.5) color = 'yellow';
    if (hp_ratio <= 0.2) color = 'red';
    ctx.fillStyle = color;
    ctx.fillRect(x1, y1, Math.floor((x2 - x1) * hp_ratio + 0.5), y2 - y1);
    if (state_str){
    	ctx.beginPath();
    	ctx.fillStyle = font_color;
    	ctx.font = "10px Arial";
		ctx.fillText(state_str,x2 + 10, Math.floor((y1 + y2) / 2));
		ctx.closePath();
    }
}

var onUnit = function(u, isSelected) {
    var player_color = player_colors[u.player_id];
    var p =  u.p;
    var last_p = u.last_p;
    var diffx = p.x - last_p.x;
    var diffy = p.y - last_p.y;
    var ori = "down";
    if (Math.abs(diffx) > Math.abs(diffy)) {
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

    draw_sprites(sprites[unit_names_minirts[u.unit_type]], xy[0], xy[1], ori);

    var hp_ratio = u.hp / u.max_hp;
    var state_str;
    if ("cmd" in u) {
        if (u.cmd.cmd[0] != 'I') {
            state_str = u.cmd.cmd[0] + u.cmd.state;
        }
    }
    var x1 = xy[0] - unit_size / 2;
    var y1 = xy[1] - 27;
    var x2 = x1 + unit_size;
    var y2 = y1 + 5;
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

var onBullet = function(bullet) {
    var xy = convert_xy(bullet.p.x, bullet.p.y);
    draw_sprites(bullets, xy[0], xy[1], bullet.state);
}

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

// Draw units that have been seen.
var onPlayerSeenUnits = function(m) {
    if ("units" in m) {
        var oldAlpha = ctx.globalAlpha;
        ctx.globalAlpha = 0.3;

        for (var i in m.units) {
            onUnit(m.units[i], false);
        }
        // console.log(m.units.length)

        ctx.globalAlpha = oldAlpha;
    }
}

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
    // Default behavior.
    var specReady = false;
    var specImage = new Image();
    specImage.onload = function () {
		specReady = true;
	};
	specImage.src = spec["_file"];
    spec["image"] = specImage;
    return spec;
};

var draw_sprites = function(spec, px, py, ori) {
    var image = spec["image"]
    var width = image.width;
    var height = image.height;
    if (!("_sizes" in spec)) {
        ctx.drawImage(image, px - width / 2, py - height / 2);
    } else {
        var sw = spec["_sizes"][0];
        var sh = spec["_sizes"][1];
        var nw = Math.floor(width / sw);
        var nh = Math.floor(height/ sh);
        var xidx = spec[ori][0];
        var yidx = spec[ori][1];
        var cx = xidx[Math.floor(tick / 3) % xidx.length] * sw;
        var cy = yidx[Math.floor(tick / 3) % yidx.length] * sh;
        ctx.drawImage(image, cx, cy, sw, sh, px - sw / 2, py - sh / 2, sw, sh);
    }
};

var myrange = function (j, k){
	var n = k - j;
	return Array.from(new Array(n), (x,i) => i + j);
};

// load pics
var sprites = {};

sprites["RANGE_ATTACKER"] = load_sprites({
    "up" : [myrange(15, 22), [0]],
    "down": [myrange(15, 22), [1]],
    "left": [[16], myrange(2, 9)],
    "right": [[15], myrange(2, 9)],
    "_file" : "imgs/tiles.png",
    "_sizes" : [32, 32]
});

sprites["MELEE_ATTACKER"] = load_sprites({
    "up" : [myrange(15, 22), [9]],
    "down": [myrange(15, 22), [10]],
    "left": [[20], myrange(2, 9)],
    "right": [[21], myrange(2, 9)],
    "_file" : "imgs/tiles.png",
    "_sizes" : [32, 32]
});

sprites["RESOURCE"] = load_sprites({
    "_file" : "imgs/mineral1.png",
});

sprites["BASE"] = load_sprites({
    "_file" : "imgs/base.png"
});

sprites["BARRACKS"] = load_sprites({
    "_file" : "imgs/barracks.png",
});

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

var bullets = load_sprites({
    "BULLET_READY" : [[7], [0]],
    "BULLET_EXPLODE1" : [[0], [0]],
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


var render = function (game) {
    tick = game.tick;
    ctx.beginPath()
	ctx.fillStyle = "Black";
	ctx.font = "15px Arial";
    var label = "Tick: " + tick;
	ctx.fillText(label, left_frame_width + 10, 20);
    ctx.closePath();
    onMap(game.rts_map);
    if (! game.spectator) {
       onPlayerSeenUnits(game.rts_map);
    }

    var all_units = {};
    var selected = {};
    for (var i in game.players) {
        onPlayerStats(game.players[i]);
    }
    for (var i in game.units) {
        var unit = game.units[i];
        all_units[unit.id] = unit;

        var s_units = game.selected_units;
        var isSelected = (s_units && s_units.indexOf(unit.id) >= 0);
        if (isSelected) {
            selected[unit.id] = unit;
        }

        onUnit(unit, isSelected);
    }
    if (dragging && x_down && y_down) {
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.rect(x_down, y_down, x_curr - x_down, y_curr - y_down);
        ctx.strokeStyle = 'green';
        ctx.stroke();
        ctx.closePath();
    }
    for (var i in game.bullets) {
        onBullet(game.bullets[i]);
    }
    var len = Object.keys(selected).length;
    if (len == 1) {
        var idx = Object.keys(selected)[0];
        var unit = selected[idx];
        draw_state(unit);
    }
    ctx.beginPath();
	ctx.fillStyle = "Black";
	ctx.font = "15px Arial";
    if (len > 1) {
        var label = len + " units";
    	ctx.fillText(label ,left_frame_width + 50, 200);
    }
    var label = "Current FPS is " + Math.floor(50 * Math.pow(1.3, speed));
    ctx.fillText(label, left_frame_width + 50, 570);
    if (game.replay_length) {
        range1.value = 100 * game.tick / game.replay_length;
    }

    var label = "Current progress_percent is " + range1.value;
    ctx.fillText(label, left_frame_width + 50, 670);
    ctx.closePath();
};

var main = function () {
  dealer = new WebSocket('ws://localhost:8000');
  dealer.onopen = function(event) {
    console.log("WS Opened.");
  }

  dealer.onmessage = function (message) {
    var s = message.data;
    var game = JSON.parse(s);
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    render(game);
  };
};

var then = Date.now();
main();
