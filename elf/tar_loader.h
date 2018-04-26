/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include <string>
#include <vector>
#include "microtar.h"

namespace elf {

namespace tar {

extern bool file_is_tar(const std::string& filename);

class TarLoader {
private:
  mtar_t tar;
public:
  TarLoader(const std::string &tar_filename);
  std::vector<std::string> List();
  std::string Load(const std::string &filename);
  ~TarLoader();
};

class TarWriter {
private:
  mtar_t tar;
public:
  TarWriter(const std::string &tar_filename);
  void Write(const std::string &filename, const std::string &contents);
  ~TarWriter();
};

}

}
