#pragma once

#include <string>
#include <cstring>
#include <vector>
#ifdef _WIN32
#define __attribute__(x)
#endif


namespace elf {

std::string TERM_COLOR(int k);

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

void c_printf(const char* col, const char* fmt, ...);

void c_fprintf(const char* col, FILE* fp, const char* fmt, ...);

__attribute__ (( format( printf, 1, 2 ) ))
std::string ssprintf(const char *fmt, ...);

inline bool endswith(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    auto l1 = strlen(str), l2 = strlen(suffix);
    if (l2 > l1) return false;
    return strncmp(str + l1 - l2, suffix, l2) == 0;
}

std::vector<std::string> strsplit(
    const std::string &str, const std::string &sep);


// 'a/b/../c/d/e/../f/./g' -> a/c/d/f/g
std::string squeeze_path(const std::string&);

}
