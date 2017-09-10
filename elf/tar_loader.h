#pragma once

#include <string>
#include <vector>
#include "microtar.h"

class TarLoader {
private:
  mtar_t tar;
public:
  TarLoader(const std::string &tar_filename);
  std::vector<std::string> List();
  std::string Load(const std::string &filename);
  ~TarLoader();
};
