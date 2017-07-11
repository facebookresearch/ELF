# RTS engines

Dependency
============

The simulators are written in C++11, so please compile using gcc >= 4.8.

The engine also comes with a web-based platform-independent visualization interface with JavaScript (thanks Qucheng Gong for getting this to work). A backend runs in the terminal and communicates with the front-end webpage to drive the game and receive any keyboard/mouse feedbacks from the web interface. In order to make the visualization work, please:

1. Install zeromq 4.0.4 from source and its C++ binding (https://github.com/zeromq/cppzmq), and copy them to /usr/local/include (or other places accessible from gcc).

2. Install libczmq 3.0.2 from source.

Due to the issues of `CZMQ-ZWSStock`, only a specific version of zeromq and czmq can be used. We welcome any better solutions.

How to compile
============

For simulators, simply run the following commands:    

```bash
cd ./rts/game_MC
make gen
make
```
And you will see a dynamic link library `minirts.so`, which is used for training. Similarly you can compile the other two games in `./rts/game_TD` and `./rts/game_CF`.

Once you have compiled the simulator, run the following command to compile the standalone backend to enable visualization.

```bash
cd ./rts/backend
make minirts GAME_DIR=../game_MC
```

And you will see an executable `minirts`. Similarly for `./rts/game_TD` and `./rts/game_CF`.

Usage of the standalone backend
============

Help
-----------
Simply run `minirts` to see switches.

Self-play
-------------
For game_MC:
Run `./minirts selfplay` for a simple self-play between two identical rule-based AIs (SimpleAI). Use switch `--seed [num]` to change the initial game seed, which affects the initial quantities and locations of buildings and units for each player.
Run `./minirts selfplay2` for a self-play between different AIs. Player 1 utilizes hit-and-run strategy and is significantly stronger than player 0(SimpleAI).

For game_CF:
Run `./minirts flag_selfplay` for a simple selfplay of Capture the Flag game.

For game_TD:
Run `./minirts td_simple --max_tick 3000` for a simple Tower Defense game. 

Visualization  
-------------

In your terminal run `minirts` to open a server at port 8000:

```bash
cd ./rts/backend
./minirts selfplay --vis_after 0
```

and then open `./rts/frontend/minirts.html` in your browser. You should be able to see two `SIMPLE_AI` competing with each other, as shown in the following figure:

![Game ScreenShot](./rts_intro.png)

Human versus AI  
-----------------------

Try `./minirts humanplay --vis_after 0` if you want to compete with the AI in the webpage. This is only implemented for game_MC.

Game play  
===================

Shortcut
------------

1. Move  
Click one unit, and click an empty place.

2. Attack  
Click one unit, press `a` and click an enemy unit.  
   a. No friendly fire. No “attack on ground” yet.  
   b. The enemy unit will retaliate by attacking back.

3. Gather  
Click one worker, press `t` and click on the resource.  
The worker will move between base and resource with simple path-planning.

4. Build  
   a. BASE: select and press `s` to produce worker.   
   b. BARRACKS: select and press `m` for melee_attacker and `r` for range_attacker.  
   c. WORKER: select, press `b`/`c` and click an empty place to build barracks / base.

Units
------------

1. Worker  
Used to gather and build. Move slow. Also has decent strength as a combat unit.

2. Melee Attacker  
Strong in combat but its attack range is 1.

3. Range Attacker  
Fast movement, long attack range but has low HP. Hit-and-run possible.
