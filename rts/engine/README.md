

## Install dependencies:

If you don't want to use system protobuf:
```bash
cd /SOME/DOWNLOAD/DIR
wget https://github.com/google/protobuf/releases/download/v3.2.0/protobuf-cpp-3.2.0.tar.gz
tar xzvf protobuf-cpp-3.2.0.tar.gz && cd protobuf-3.2.0
./autogen.sh && ./configure --prefix=/INSTALL/DIR
make && make install
# add /INSTALL/DIR/lib to $PKG_CONFIG_PATH
```

If don't want to use system libzmq:
```bash
cd /SOME/DOWNLOAD/DIR
wget https://github.com/zeromq/libzmq/releases/download/v4.2.2/zeromq-4.2.2.tar.gz
tar xzvf zeromq-4.2.2.tar.gz && cd zeromq-4.2.2
./autogen.sh && ./configure --prefix=/INSTALL/DIR
make && make install
# add /INSTALL/DIR/lib to $PKG_CONFIG_PATH
```

## Build:
```bash
/INSTALL/DIR/bin/protoc message.proto --cpp_out=.
make -f Makefile2 gen
make -f Makefile2
```
