# BackEnd

Compilation
========
Note that due to [this](https://github.com/nlohmann/json#supported-compilers), only GCC >= 4.9 is supported here.

First compile `./rts/game_MC` (or other two games), then compile this folder by
```bash
make GAME_DIR="../game_MC"
```
to get an standalone executable file.
