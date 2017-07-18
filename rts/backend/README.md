# BackEnd

Compilation
========
Note that due to [this](https://github.com/nlohmann/json#supported-compilers), only GCC >= 4.9 is supported here. Also you need zeromq 4.0.4 and libczmq 3.0.2, both from source. This is due to the issues that `CZMQ-ZWSStock` can only use a specific version of zeromq and czmq. We welcome any better solutions.

First compile `./rts/game_MC` (or other two games), then compile this folder by
```bash
make minirts GAME_DIR="../game_MC"
```
to get an standalone executable file. 
