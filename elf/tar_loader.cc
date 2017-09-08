#include "tar_loader.h"

TarLoader::TarLoader(const char *tar_filename) {
  mtar_open(&tar, filename.c_str(), "r");
}

std::vector<string> List() {
  /* Print all file names and sizes */
  std::vector<string> v;
  mtar_header_t h;
  while ((mtar_read_header(&tar, &h)) != MTAR_ENULLRECORD) {
    v.append(string(h.name()));
    mtar_next(&tar);
  }
}

string Load(string filename) {
  /* Load and print contents of file "test.txt" */
  mtar_header_t h;
  mtar_find(&tar, filename.c_str(), &h);
  char* p = calloc(1, h.size + 1);
  mtar_read_data(&tar, p, h.size);
  return string(p);
}

TarLoader::~TarLoader() {
    mtar_close(&tar);
}
