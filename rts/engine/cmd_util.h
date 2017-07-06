/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _CMD_UTILS_H_
#define _CMD_UTILS_H_

#include <vector>
#include <map>
#include <string>
#include <stdexcept>

namespace CmdLineUtils {

struct OptionContent {
    bool has_default_value;
    std::string default_value;

    OptionContent() : has_default_value(false) { }
};

template <typename T> struct Converter;

template<> struct Converter<std::string> { static std::string Convert(const std::string &s) { return s; } };
template<> struct Converter<int> { static int Convert(const std::string &s) { return std::stoi(s); } };
template<> struct Converter<float> { static int Convert(const std::string &s) { return std::stof(s); } };
template<> struct Converter<bool> { static int Convert(const std::string &s) { return std::stoi(s) == 1; } };

class CmdLineParser {
private:
    std::vector<std::string> _ordered_args;
    std::map<std::string, OptionContent> _options_args;

    // Parsed contents.
    std::map<std::string, std::string> _parsed_items;

public:
    // Example specifier:
    //   n m k --verbose[1] --max_iteration
    explicit CmdLineParser(const std::string& specifier);

    std::string PrintHelper() const;
    bool Parse(int argc, char *argv[]);
    std::string PrintParsed() const;

    bool HasItem(const std::string& key) const {
        auto it = _parsed_items.find(key);
        return it != _parsed_items.end();
    }

    template <typename T>
    T GetItem(const std::string& key) const {
        auto it = _parsed_items.find(key);
        if (it != _parsed_items.end()) return Converter<T>::Convert(it->second);
        else throw std::invalid_argument("The argument " + key + " is not found");
    }

    template <typename T>
    T GetItem(const std::string& key, const T &default_value) const {
        auto it = _parsed_items.find(key);
        if (it != _parsed_items.end()) return Converter<T>::Convert(it->second);
        else return default_value;
    }
};

}

#endif
