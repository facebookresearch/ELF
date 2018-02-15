/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

// Create the canvas
var canvas = document.createElement("canvas");
var ctx = canvas.getContext("2d");
// max sizes
var map_x = 40;
var map_y = 40;
var cell_size = 35;

canvas.width = map_x * cell_size + 400;
canvas.height = map_y * cell_size;
var left_frame_width = map_x * cell_size;
var player_colors = ['blue', 'red', 'yellow']

var terrains = ["GROUND", "SAND", "GRASS", "ROCK", "WATER", "FOG"];
var unit_names_minirts = ["RESOURCE", "WORKER", "ENGINEER", "SOLDIER", "TRUCK", "TANK", "CANNON", "FLIGHT", "BARRACK", "FACTORY", "HANGAR", "DEFENSE_TOWER", "BASE"];
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
        if (xy0[0] > map_x || xy0[1] > map_y) return;
        x_down = e.pageX;
        y_down = e.pageY;
    }
}, false);

canvas.addEventListener("mouseup", function (e) {
    var xy0 = convert_xy_back(e.pageX, e.pageY);
    if (xy0[0] > map_x || xy0[1] > map_y) return;
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
            var type = m.slots[counter];
            var spec = terrain_sprites[terrains[type]];
            var x1 = x * cell_size + cell_size / 2;
            var y1 = y * cell_size + cell_size / 2;
            draw_terrain_sprite(spec, x1, y1);
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
    if (state_str) {
    	ctx.beginPath();
    	ctx.fillStyle = font_color;
    	ctx.font = "10px Arial";
      ctx.fillText(state_str,x2 + 10, Math.floor((y1 + y2) / 2));
      ctx.closePath();
    }
}

var onUnit = function(u, isSelected) {
    var player_color = player_colors[u.player_id];
    var sprites = player_sprites[player_color];
    var p =  u.p;
    var last_p = u.last_p;
    var ori = 0;
    var xy = convert_xy(p.x, p.y);

    var spec = sprites[unit_names_minirts[u.unit_type]];
    draw_sprites(spec, xy[0], xy[1], ori);

    var hp_ratio = u.hp / u.max_hp;
    var state_str;
    if ("cmd" in u) {
        if (u.cmd.cmd[0] != 'I') {
            state_str = u.cmd.cmd[0] + u.cmd.state;
        }
    }
    var sw = Math.floor(cell_size * spec["_select_scale"]);
    var sh = Math.floor(cell_size * spec["_select_scale"]);
    var x1 = xy[0] - Math.floor(sw * 0.4);
    var y1 = xy[1] - sh / 2 - 10;
    var x2 = x1 + Math.floor(sw * 0.8);
    var y2 = y1 + 5;
    draw_hp([x1, y1, x2, y2], [hp_ratio, state_str], 'white', player_color);
    if (isSelected) {
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.rect(xy[0] - sw / 2 - 2, xy[1] - sh / 2 - 2, sw + 4, sh + 4);
        ctx.strokeStyle = player_color;
        ctx.stroke();
        ctx.closePath();
    }
};

var onBullet = function(bullet) {
    var xy = convert_xy(bullet.p.x, bullet.p.y);
    draw_bullet(bullets, xy[0], xy[1], bullet.state);
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
        ctx.globalAlpha = oldAlpha;
    }
}

var draw_state = function(u) {
    var x1 = left_frame_width + 10;
    var y1 = 150;
    var x2 = left_frame_width + 100;
    var y2 = 300;
    var title = unit_names_minirts[u.unit_type] + ' ' + u.cmd.cmd + '[' + u.cmd.state + ']';
    ctx.fillStyle = "Green";
    ctx.font = "15px Arial";
    ctx.fillText(title, x1, y1, 300);
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
    var xc = x * cell_size + Math.floor(cell_size / 2);
    var yc = y * cell_size + Math.floor(cell_size / 2);
    return [xc, yc]
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

var load_player_sprites = function(player) {
    var sprites = {};
    sprites["RESOURCE"] = load_sprites({
        "_file" : "imgs/mineral1.png",
        "_scale": 1.2,
        "_select_scale" : 1
    });
    sprites["WORKER"] = load_sprites({
        "_file": "rts/" + player + "/worker.png",
        "_scale": 1.5,
        "_select_scale" : 0.7
    });
    sprites["ENGINEER"] = load_sprites({
        "_file": "rts/" + player + "/engineer.png",
        "_scale": 1.5,
        "_select_scale" : 0.7
    });
    sprites["SOLDIER"] = load_sprites({
        "_file": "rts/" + player + "/soldier.png",
        "_scale": 1.5,
        "_select_scale" : 0.7
    });
    sprites["TRUCK"] = load_sprites({
        "_file": "rts/" + player + "/truck.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["TANK"] = load_sprites({
        "_file": "rts/" + player + "/tank.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["CANNON"] = load_sprites({
        "_file": "rts/" + player + "/cannon.png",
        "_scale": 2,
        "_select_scale" : 1.3
    });
    sprites["FLIGHT"] = load_sprites({
        "_file": "rts/" + player + "/flight.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["BARRACK"] = load_sprites({
        "_file": "rts/" + player + "/barrack.png",
        "_scale": 1.5,
        "_select_scale" : 1.3

    });
    sprites["FACTORY"] = load_sprites({
        "_file": "rts/" + player + "/factory.png",
        "_scale": 1.5,
        "_select_scale" : 1.3
    });
    sprites["HANGAR"] = load_sprites({
        "_file": "rts/" + player + "/hangar.png",
        "_scale": 1.5,
        "_select_scale" : 1.3
    });
    sprites["DEFENSE_TOWER"] = load_sprites({
        "_file": "rts/" + player + "/defense_tower.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["BASE"] = load_sprites({
        "_file": "rts/" + player + "/base.png",
        "_scale": 1.7,
        "_select_scale" : 1.4
    });
    return sprites;
}

var draw_bullet = function(spec, px, py, ori) {
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

var draw_sprites = function(spec, px, py, ori) {
    var image = spec["image"];
    var scale = spec["_scale"];
    var w = Math.floor(cell_size * scale);
    var h = Math.floor(cell_size * scale);
    ctx.drawImage(image, px - w / 2, py - h / 2, w, h);
    // for debug
    //ctx.beginPath();
    //ctx.arc(px, py, 3, 0, 2 * Math.PI, false);
    //ctx.fillStyle = 'black';
    //ctx.fill();
    //ctx.closePath();
}

var draw_terrain_sprite = function(spec, px, py) {
    var image = spec["image"];
    ctx.drawImage(image, px - cell_size / 2, py - cell_size / 2, cell_size, cell_size);
};

var bullets = load_sprites({
    "BULLET_READY" : [[7], [0]],
    "BULLET_EXPLODE1" : [[0], [0]],
    "BULLET_EXPLODE2" : [[1], [0]],
    "BULLET_EXPLODE3": [[2], [0]],
    "_file" : "imgs/tiles.png",
    "_sizes" : [32, 32]
});

var player_sprites = {
  "blue" : load_player_sprites("blue"),
  "red"  : load_player_sprites("red")
};

var terrain_sprites = {};

terrain_sprites["GROUND"] = load_sprites({
  "_file" : "rts/terrain/ground.png"
});

terrain_sprites["SAND"] = load_sprites({
  "_file" : "rts/terrain/sand.png"
});

terrain_sprites["GRASS"] = load_sprites({
  "_file" : "rts/terrain/grass.png"
});

terrain_sprites["ROCK"] = load_sprites({
  "_file" : "rts/terrain/rock.png"
});

terrain_sprites["WATER"] = load_sprites({
  "_file" : "rts/terrain/water.png"

});

terrain_sprites["FOG"] = load_sprites({
  "_file" : "rts/terrain/fog.png"
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
    map_x = game.rts_map.width;
    map_y = game.rts_map.height;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    render(game);
  };
};

var then = Date.now();
main();
