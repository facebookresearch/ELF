#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <cstdarg>
#include <map>
#ifdef _MSC_VER
#include <filesystem>
using namespace std::tr2::sys;
#else
#include <libgen.h>
#endif
using namespace std;

#include "strutils.hh"

namespace elf {

void __m_assert_check__(bool val, const char *expr, const char *file, const char *func, int line) {
    if (val)
        return;
    c_fprintf(COLOR_RED, stderr, "assertion \"%s\" failed, in %s, (%s:%d)\n",
            expr, func, file, line);
    abort();
    //exit(1);
}


void __print_debug__(const char *file, const char *func, int line, const char *fmt, ...) {
    static map<int, string> colormap;
    if (! colormap[line].length()) {
        int color = std::hash<int>()(line) % 5;
        colormap[line] = TERM_COLOR(color);
    }

#ifdef _MSC_VER
  std::tr2::sys::path _fbase(file);
  auto fbase = _fbase.stem().c_str();
#else
  char *fdup = strdup(file);
  char *fbase = basename(fdup);
#endif
  c_fprintf(colormap[line].c_str(), stderr, "[%s@%s:%d] ", func, fbase, line);
  free(fdup);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}


void error_exit(const char *msg) {
    c_fprintf(COLOR_RED, stderr, "error: %s\n", msg);
    exit(1);
}

}
