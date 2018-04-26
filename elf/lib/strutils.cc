/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "strutils.hh"
#include "debugutils.hh"
#include <cstdarg>
#include <sstream>

using namespace std;

namespace {
inline bool char_in_string(int ch, const std::string &str) {
    return str.find(ch) != std::string::npos;
}
}

namespace elf {

std::string TERM_COLOR(int k) {
    // k = 0 ~ 4
    std::ostringstream ss;
    ss << "\x1b[3" << k + 2 << "m";
    return ss.str();
}

void c_printf(const char* col, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("%s", col);
    vprintf(fmt, ap);
    printf(COLOR_RESET);
    va_end(ap);
}

void c_fprintf(const char* col, FILE* fp, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(fp, "%s", col);
    vfprintf(fp, fmt, ap);
    fprintf(fp, COLOR_RESET);
    va_end(ap);
}


std::string ssprintf(const char *fmt, ...) {
    int size = 100;
    char *p = (char *)malloc(size);

    va_list ap;

    std::string ret;

    while (true) {
        va_start(ap, fmt);
        int n = vsnprintf(p, size, fmt, ap);
        va_end(ap);

        if (n < 0) {
            free(p);
            return "";
        }

        if (n < size) {
            ret = p;
            free(p);
            return ret;
        }

        size = n + 1;

        char *np = (char *)realloc(p, size);
        if (np == nullptr) {
            free(p);
            return "";
        } else
            p = np;
    }
}

std::vector<std::string> strsplit(const std::string &str, const std::string &sep) {
    std::vector<std::string> ret;

    auto is_sep = [&](int c) -> bool {
        if (sep.length())
            return char_in_string(c, sep);
        return isblank(c);
    };

    // split("\n\naabb\n\nbbcc\n") -> ["aabb", "bbcc"]. same as python
    for (size_t i = 0; i < str.length(); ) {
        while (i < str.length() && is_sep(str[i]))
            i ++;
        size_t j = i + 1;

        while (j < str.length() && !is_sep(str[j]))
            j ++;
        ret.emplace_back(str.substr(i, j - i));

        i = j + 1;
        while (i < str.length() && is_sep(str[i]))
            i ++;
    }
    return ret;
}


string squeeze_path(const string& path) {
  if (path.empty()) return path;
#ifdef _WIN32
  fprintf(stderr, "squeeze_path() doesn't support windows!");
  abort();
#endif
  auto splits = strsplit(path, "/");
  vector<string> res;
  for (auto& p: splits) {
    if (p.compare(".") == 0)
      continue;
    if (p.compare("..") == 0 &&
        res.size() &&
        res.back().compare("..") != 0) {
      res.pop_back();
    } else {
      res.emplace_back(move(p));
    }
  }
  string ret;
  if (path[0] == '/')
    ret += '/';
  for (auto& p : res) {
    ret += p;
    ret += '/';
  }
  if (res.size() > 0)
    if (path.size() < 2 || path.back() != '/')
      ret.pop_back();
  return ret;
}

}
