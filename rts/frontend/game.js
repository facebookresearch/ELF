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
var SCALER = 1.0;
var cell_size = 30;

canvas.width = map_x * cell_size + 400;
canvas.height = map_y * cell_size;
var left_frame_width = map_x * cell_size;
var player_colors = ['blue', 'red', 'yellow']

var terrains = ["GROUND", "SAND", "GRASS", "ROCK", "WATER", "FOG"];
var unit_names_minirts = ["RESOURCE", "PEASANT", "SWORDMAN", "SPEARMAN", "CAVALRY", "ARCHER", "DRAGON", "CATAPULT", "BARRACK", "BLACKSMITH", "STABLE", "WORKSHOP", "GUARD_TOWER", "TOWN_HALL"];
var unit_id = {
  "RESOURCE": 0,
  "PEASANT": 1,
  "SWORDMAN": 2,
  "SPEARMAN": 3,
  "CAVALRY": 4,
  "ARCHER": 5,
  "DRAGON": 6,
  "CATAPULT": 7,
  "BARRACK": 8,
  "BLACKSMITH": 9,
  "STABLE": 10,
  "WORKSHOP": 11,
  "GUARD_TOWER": 12,
  "TOWN_HALL": 13};

var x_down = null;
var y_down = null;
var x_curr;
var y_curr;
var dragging = false;
var tick = 0;
var dealer;
var speed = 0;
var min_speed = -10;
var max_speed = 5;
var last_state = null;

var scale = function(x) {
  var m = x / 2;
  return m + Math.floor((x - m) * SCALER);
}



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


var button_left = left_frame_width + scale(30);

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

var resize = function() {
  var min_w = 400;
  var min_h = 400;
  var max_w = 1200;
  var max_h = 1200;
  var min_cz = 15;
  var max_cz = 30;
  var dw = 200;
  var dh = 200;
  var w = Math.min(Math.max(window.innerWidth - dw, min_w), max_w);
  var h = Math.min(Math.max(window.innerHeight - dh, min_h), max_h);
  var pw = 1.0 * (w - min_w) / (max_w - min_w);
  var ph = 1.0 * (h - min_h) / (max_h - min_h);
  SCALER = Math.min(pw, ph);
  cell_size = min_cz + Math.floor(SCALER * (max_cz - min_cz));

  canvas.width = map_x * cell_size + scale(400);
  canvas.height = map_y * cell_size;
  left_frame_width = map_x * cell_size;
}

window.onresize = function(e) {
  resize();
}

window.onload = function(e) {
  resize();
}


function make_cursor(color) {
    var cursor = document.createElement('canvas');
    var ctx = cursor.getContext('2d');

    cursor.width = 16;
    cursor.height = 16;

    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.arc(8, 8, 7, 0, 2 * Math.PI, false);
    ctx.moveTo(0, 8);
    ctx.lineTo(15, 8);
    ctx.moveTo(8, 0);
    ctx.lineTo(8, 15);
    ctx.stroke();
    ctx.closePath();
    return 'url(' + cursor.toDataURL() + '), auto';
}

var attack_cursor = make_cursor('red');
var move_cursor = make_cursor('green');
var gather_cursor = make_cursor('cyan');
var build_cursor = make_cursor('grey');

canvas.oncontextmenu = function (e) {
    e.preventDefault();
};


function are_unit_types_selected(types) {
    if (last_state === null) return false;
    var selected = last_state.selected_units;
    if (!selected) return false;
    var any = false;
    for (var i in last_state.units) {
        var unit = last_state.units[i];
        if (selected.indexOf(unit.id) >= 0) {
            if (types.indexOf(unit.unit_type) >= 0) {
                any = true;
            } else {
                return false;
            }
        }
    }
    return any;
}

function are_workers_selected() {
    var worker_ty = [unit_id["PEASANT"]];
    return are_unit_types_selected(worker_ty);
}

function are_units_selected() {
    var unit_ty = [
      unit_id["PEASANT"],
      unit_id["SWORDMAN"],
      unit_id["SPEARMAN"],
      unit_id["CAVALRY"],
      unit_id["ARCHER"],
      unit_id["DRAGON"],
      unit_id["CATAPULT"]];
    return are_unit_types_selected(unit_ty);
}

function are_towers_selected() {
    var tower_ty = [unit_id["GUARD_TOWER"]];
    return are_unit_types_selected(tower_ty);
}

function get_unit_type(id) {
    if (last_state === null) return -1;
    for (var i in last_state.units) {
        if (id === last_state.units[i].id) {
            return last_state.units[i].unit_type;
        }
    }
    return -1;
}

function is_build_cmd_allowed(key, types) {
    if (last_state === null) return false;
    var selected = last_state.selected_units;
    if (!selected) return false;
    if (selected.length != 1) return false;
    var id = selected[0];
    var ty = get_unit_type(id);
    if (types.indexOf(ty) < 0) {
        return false;
    }
    var def = last_state.gamedef.units[ty];
    for (var i in def.build_skills) {
        if (key == def.build_skills[i].hotkey) {
            return true;
        }
    }
    return false;

}

function is_worker_cmd_allowed(key) {
    var worker_types = [unit_id["PEASANT"]];
    console.log(worker_types);
    return is_build_cmd_allowed(key, worker_types);
}

function is_building_cmd_allowed(key) {
    var building_types = [
      unit_id["BARRACK"],
      unit_id["BLACKSMITH"],
      unit_id["STABLE"],
      unit_id["WORKSHOP"],
      unit_id["TOWN_HALL"]];
    return is_build_cmd_allowed(key, building_types);
}

document.addEventListener("keydown", function (e) {
    if (e.key == 'a') {
      if (are_units_selected() || are_towers_selected()) {
        document.body.style.cursor = attack_cursor;
        send_cmd(tick + ' ' + e.key);
      }
    }
    else if (e.key == 'g') {
      if (are_workers_selected()) {
        document.body.style.cursor = gather_cursor;
        send_cmd(tick + ' ' + e.key);
      }
    }
    else if (e.key == 'm') {
      if (are_units_selected()) {
        document.body.style.cursor = move_cursor;
        send_cmd(tick + ' ' + e.key);
      }
    }
    else if (is_worker_cmd_allowed(e.key)) {
        document.body.style.cursor = build_cursor;
        send_cmd(tick + ' ' + e.key);
    }
    else if (is_building_cmd_allowed(e.key)) {
        send_cmd(tick + ' ' + e.key);
    }
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
        document.body.style.cursor = 'default';
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
            var seen_before = false;
            if (m.seen_terrain != null) {
                seen_before = m.seen_terrain[counter];
            }
            var spec = terrain_sprites[terrains[type]];
            var x1 = x * cell_size + cell_size / 2;
            var y1 = y * cell_size + cell_size / 2;
            draw_terrain_sprite(spec, x1, y1, seen_before);
            counter += 1;
    	}
	}
};

var draw_hp = function(bbox, states, font_color, player_color, fill_color, progress){
    var x1 = bbox[0];
    var y1 = bbox[1];
    var x2 = bbox[2];
    var y2 = bbox[3];
    var hp_ratio = states[0];
    var state_str = states[1];
    var margin = scale(3);
    ctx.fillStyle = 'black';
    ctx.lineWidth = margin;
    ctx.beginPath();
    ctx.rect(x1, y1, x2 - x1, y2 - y1);
    ctx.fillRect(x1, y1, x2 - x1, y2 - y1);
    ctx.strokeStyle = player_color;
    ctx.stroke();
    ctx.closePath();
    var color = fill_color;
    if (progress && hp_ratio <= 0.5) color = 'yellow';
    if (progress && hp_ratio <= 0.2) color = 'red';
    ctx.fillStyle = color;
    ctx.fillRect(x1, y1, Math.floor((x2 - x1) * hp_ratio + 0.5), y2 - y1);
    /*
    if (state_str) {
        ctx.beginPath();
        ctx.fillStyle = font_color;
        ctx.font = "10px Arial";
        ctx.fillText(state_str, x2 + 10, y1 + cell_size * 0.3);
        ctx.closePath();
    }
    */
}

var onUnit = function(u, isSelected) {
    var player_color = player_colors[u.player_id];
    var sprites = player_sprites[player_color];
    var p =  u.p;
    var last_p = u.last_p;
    var xy = convert_xy(p.x, p.y);

    var spec = sprites[unit_names_minirts[u.unit_type]];
    draw_sprites(spec, xy[0], xy[1], null);

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
    var y2 = y1 + scale(5);
    draw_hp([x1, y1, x2, y2], [hp_ratio, state_str], 'white', player_color, 'green', true);
    if (isSelected) {
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.rect(xy[0] - sw / 2 - scale(2), xy[1] - sh / 2 - scale(2), sw + scale(4), sh + scale(4));
        ctx.strokeStyle = player_color;
        ctx.stroke();
        ctx.closePath();
    }
};

var onBullet = function(bullet) {
    var xy = convert_xy(bullet.p.x, bullet.p.y);
    draw_bullet(bullets, xy[0], xy[1], bullet.state);
}

var onPlayersStats = function(players, game) {
    var x1 = left_frame_width;
    var y1 = 0;
    var label = "";
    for (var i in players) {
        var player = players[i];
        label = label + "PLAYER " + player.player_id + ":" + player.resource + "  ";
    }
    ctx.beginPath()
    ctx.fillStyle = "Black";
    ctx.font = "15px Arial";
    ctx.fillText("TIME: " + game.tick, x1 + cell_size, y1 + cell_size / 2 + 5);
    y1 += scale(25);
    ctx.fillText(label, x1 + cell_size, y1 + cell_size / 2 + scale(5));
    ctx.closePath();
}

// Draw units that have been seen.
var onPlayerSeenUnits = function(m) {
    if ("units" in m) {
        var oldAlpha = ctx.globalAlpha;
        ctx.globalAlpha = 0.5;

        for (var i in m.units) {
            onUnit(m.units[i], false);
        }
        ctx.globalAlpha = oldAlpha;
    }
}

var draw_state = function(u, game) {
    var player_color = player_colors[u.player_id];
    var sprites = player_sprites[player_colors[u.player_id]];
    var x1 = left_frame_width + scale(20);
    var y1 = scale(60);
    var spec = sprites[unit_names_minirts[u.unit_type]];
    draw_sprites(spec, x1 + cell_size / 2, y1 + cell_size, null);

    ctx.beginPath();
    var title = unit_names_minirts[u.unit_type] + ': ' + u.cmd.cmd + '[' + u.cmd.state + ']';
    ctx.fillStyle = "black";
    ctx.font = "10px Arial";
    ctx.fillText(title, x1 + 1.5 * cell_size + scale(5), y1 + cell_size / 2 + scale(5), scale(300));
    ctx.closePath();
    var ratio = u.hp / u.max_hp;
    var label = "HP: " + u.hp + " / " + u.max_hp;
    var x2 = x1 + 1.5 * cell_size + scale(5);
    var y2 = y1 + cell_size / 2 + scale(20);
    draw_hp([x2, y2, x2 + scale(100), y2 + scale(15)], [ratio, label], 'black', player_color, 'green', true);
    if (u.player_id != game.player_id) {
      return;
    }

    var x3 = x1;
    var y3 = y1 + 2 * cell_size;
    for (var i in u.cds) {
        var cd = u.cds[i];
        if (cd.cd > 0) {
            var curr = Math.min(game.tick - cd.last, cd.cd);
            if (cd.last === 0) curr = cd.cd;
            ratio = curr / cd.cd;
            var label = cd.name.split("_")[1] + ": " + curr + " / " + cd.cd;
            draw_hp([x3, y3, x3 + scale(100), y3 + scale(15)], [ratio, label], 'black', player_color, 'magenta', false);
            y3 += scale(20);
        }
    }

    var cmd_names = {
      200: ["ATTACK", "a"],
      201: ["MOVE", "m"],
      202: ["BUILD","b"],
      203: ["GATHER", "g"]
    };
    var def = game.gamedef.units[u.unit_type];
    var title = "[HOTKEY]COMMAND:";
    for (var i in def.allowed_cmds) {
        var cmd = def.allowed_cmds[i];
        var name = cmd_names[cmd.id][0];
        if (name == "BUILD") continue;
        var hotkey = cmd_names[cmd.id][1];
        title = title + ' [' + hotkey + "]" + name;
    }
    ctx.beginPath()
    ctx.fillStyle = "black";
    ctx.font = "12px Arial";
    y3 += scale(20);
    ctx.fillText(title, x3, y3);
    ctx.closePath();
    y3 += scale(40);

    for (var i in def.build_skills) {
        var skill = def.build_skills[i];
        var unit_name = unit_names_minirts[skill.unit_type];
        var spec = sprites[unit_name];

        draw_sprites(spec, x3 + cell_size / 2, y3 + cell_size / 2, 1);
        ctx.beginPath();
        var title = "[" + skill.hotkey + "]BUILD " + unit_name + " COST " + skill.price + " GOLD";
        ctx.fillStyle = "black";
        ctx.font = "12px Arial";
        ctx.fillText(title, x3 + cell_size + scale(10), y3 + cell_size / 2 + scale(3));
        ctx.closePath();
        y3 += cell_size + scale(10);
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
        "_file" : "rts/medieval/" + player + "/coin.png",
        "_scale": 1.2,
        "_select_scale" : 1
    });
    sprites["PEASANT"] = load_sprites({
        "_file": "rts/medieval/" + player + "/peasant.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["SWORDMAN"] = load_sprites({
        "_file": "rts/medieval/" + player + "/swordman.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["SPEARMAN"] = load_sprites({
        "_file": "rts/medieval/" + player + "/spearman.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["CAVALRY"] = load_sprites({
        "_file": "rts/medieval/" + player + "/cavalry.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["ARCHER"] = load_sprites({
        "_file": "rts/medieval/" + player + "/archer.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["DRAGON"] = load_sprites({
        "_file": "rts/medieval/" + player + "/dragon.png",
        "_scale": 2,
        "_select_scale" : 1.5
    });
    sprites["CATAPULT"] = load_sprites({
        "_file": "rts/medieval/" + player + "/catapult.png",
        "_scale": 2,
        "_select_scale" : 1.5
    });
    sprites["BARRACK"] = load_sprites({
        "_file": "rts/medieval/" + player + "/barrack.png",
        "_scale": 2,
        "_select_scale" : 1.2

    });
    sprites["BLACKSMITH"] = load_sprites({
        "_file": "rts/medieval/" + player + "/blacksmith.png",
        "_scale": 2,
        "_select_scale" : 1.2
    });
    sprites["STABLE"] = load_sprites({
        "_file": "rts/medieval/" + player + "/stable.png",
        "_scale": 2,
        "_select_scale" : 1.2
    });
    sprites["WORKSHOP"] = load_sprites({
        "_file": "rts/medieval/" + player + "/workshop.png",
        "_scale": 2,
        "_select_scale" : 1.2
    });
    sprites["GUARD_TOWER"] = load_sprites({
        "_file": "rts/medieval/" + player + "/guard_tower.png",
        "_scale": 1.5,
        "_select_scale" : 1
    });
    sprites["TOWN_HALL"] = load_sprites({
        "_file": "rts/medieval/" + player + "/town_hall.png",
        "_scale": 2.5,
        "_select_scale" : 1.5
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

var draw_sprites = function(spec, px, py, scale) {
    var image = spec["image"];
    if (scale === null) {
        scale = spec["_scale"];
    }
    var w = Math.floor(cell_size * scale);
    var h = Math.floor(cell_size * scale);
    ctx.drawImage(image, px - w / 2, py - 0.7 * h, w, h);
    // for debug
    //ctx.beginPath();
    //ctx.arc(px, py, 3, 0, 2 * Math.PI, false);
    //ctx.fillStyle = 'black';
    //ctx.fill();
    //ctx.closePath();
}

var draw_terrain_sprite = function(spec, px, py, seen_before) {
    var fog_image = terrain_sprites["FOG"]["image"];
    var image = spec["image"];
    if (seen_before) {
        ctx.drawImage(fog_image, px - cell_size / 2, py - cell_size / 2, cell_size, cell_size);
        var oldAlpha = ctx.globalAlpha;
        ctx.globalAlpha = 0.3;
        ctx.drawImage(image, px - cell_size / 2, py - cell_size / 2, cell_size, cell_size);
        ctx.globalAlpha = oldAlpha;
    } else {
        ctx.drawImage(image, px - cell_size / 2, py - cell_size / 2, cell_size, cell_size);
    }
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
    onMap(game.rts_map);
    if (! game.spectator) {
       onPlayerSeenUnits(game.rts_map);
    }

    var all_units = {};
    var selected = {};
    onPlayersStats(game.players, game);
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
        draw_state(unit, game);
    }
    ctx.beginPath();
    ctx.fillStyle = "Black";
    ctx.font = "15px Arial";
    if (len > 1) {
        var label = len + " units";
    	  ctx.fillText(label ,left_frame_width + scale(50), scale(200));
    }
    //var label = "Current FPS is " + Math.floor((scale(50)) * Math.pow(1.3, speed));
    //ctx.fillText(label, left_frame_width + scale(50), scale(370));
    if (game.replay_length) {
        //range1.value = 100 * game.tick / game.replay_length;
    }

    /*
    var label = "Current progress_percent is " + range1.value;
    ctx.fillText(label, left_frame_width + 25 + scale(25), 330 + scale(340));
    */
    ctx.closePath();
};

var main = function () {
  var param = new URLSearchParams(window.location.search);
  if (param.has("player")) {
      var port =  param.get("player");
  } else {
      var port = "0";
  }
  dealer = new WebSocket('ws://localhost:800' + port);
  dealer.onopen = function(event) {
      console.log("WS Opened.");
  }

  dealer.onmessage = function (message) {
    var s = message.data;
    var game = JSON.parse(s);
    last_state = game;
    map_x = game.rts_map.width;
    map_y = game.rts_map.height;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    render(game);
  };
};

main();
