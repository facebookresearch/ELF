#include "tar_loader.h"

bool file_is_tar(const std::string& filename) {
  return options.filename.substr(options.filename.find_last_of(".") + 1) == "tar"
}

TarLoader::TarLoader(const char *tar_filename) {
  mtar_open(&tar, filename.c_str(), "r");
}

std::vector<string> TarLoader::List() {
  /* Print all file names and sizes */
  std::vector<string> v;
  mtar_header_t h;
  while ((mtar_read_header(&tar, &h)) != MTAR_ENULLRECORD) {
    v.append(string(h.name()));
    mtar_next(&tar);
  }
}

string TarLoader::Load(string filename) {
  /* Load and print contents of file "test.txt" */
  mtar_header_t h;
  mtar_find(&tar, filename.c_str(), &h);
  char* p = calloc(1, h.size + 1);
  mtar_read_data(&tar, p, h.size);
  std::string s = string(p);
  delete[] p;
  return s;
}

TarLoader::~TarLoader() {
    mtar_close(&tar);
}
