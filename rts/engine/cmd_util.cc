/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "cmd_util.h"
#include <regex>
#include <iostream>
#include <sstream>
#include <set>

namespace CmdLineUtils {

std::string trim(std::string& str) {
    str.erase(0, str.find_first_not_of(' '));       //prefixing spaces
    str.erase(str.find_last_not_of(' ')+1);         //surfixing spaces
    return str;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (getline(ss, item, delim)) {
        elems.push_back(std::move(item));
    }
    return elems;
}

// Constructor.
CmdLineParser::CmdLineParser(const std::string& specifier) {
    // Parse the specifier.
    std::regex option_regex("--(\\w+)(\\[([\\-0-9A-Za-z_\\.]+)\\])?");
    std::smatch option_match;

    for (const auto &item : split(specifier, ' ')) {
        if (item.empty()) continue;

        if (std::regex_match(item, option_match, option_regex)) {
            // Option.
            auto s = option_match[1];
            OptionContent content;
            if (option_match.size() >= 4 && option_match[3].length() > 0) {
                content.has_default_value = true;
                content.default_value = option_match[3];
            }
            _options_args.emplace(make_pair(s, content));
        } else {
            // Ordered argument.
            _ordered_args.emplace_back(item);
        }
    }
}

std::string CmdLineParser::PrintHelper() const {
    std::stringstream ss;
    for (const auto &ordered_arg : _ordered_args) {
        ss << ordered_arg << " ";
    }
    for (const auto &option_arg : _options_args) {
        ss << "--" << option_arg.first;
        if (option_arg.second.has_default_value) {
            ss << "[" << option_arg.second.default_value << "]";
        }
        ss << " ";
    }
    return ss.str();
}


std::string CmdLineParser::PrintParsed() const {
    std::stringstream ss;
    for (const auto &p : _parsed_items) {
        ss << "\"" << p.first << "\"" << " -> " << "\"" << p.second << "\"" << std::endl;
    }
    return ss.str();
}

bool CmdLineParser::Parse(int argc, char *argv[]) {
    // Insufficient arguments.
    int arg_size = _ordered_args.size();
    if (argc < arg_size + 1) {
        std::cout << "Insufficient argument! " << argc - 1 << " < " << _ordered_args.size() << std::endl;
        return false;
    }

    _parsed_items.clear();

    int i = 1;
    for (;i <= arg_size; ++i) {
        _parsed_items.emplace(make_pair(_ordered_args[i - 1], std::string(argv[i])));
    }

    // Check all the option arguments.
    std::set<std::string> option_specified;

    while (i < argc) {
        // Find a match, remove the leading --
        std::string item = std::string(argv[i] + 2);

        auto it = _options_args.find(item);

        // Unknown match.
        if (it == _options_args.end()) {
            std::cout << "Option " << item << " is unknown!" << std::endl;
            return false;
        }

        option_specified.insert(item);
        // const OptionContent& content = it->second;

        i ++;
        if (i == argc) {
            std::cout << "No argument for option " << item << std::endl;
            return false;
        }

        std::string a = std::string(argv[i]);
        _parsed_items.emplace(make_pair(item, a));

        i++;
    }

    // Check all unmatched options, if they have default values and is not specified, add them.
    for (const auto &option_arg : _options_args) {
        const auto &item = option_arg.first;
        const auto &content = option_arg.second;

        auto it = option_specified.find(item);
        if (it == option_specified.end() && content.has_default_value) {
            _parsed_items.emplace(make_pair(item, content.default_value));
        }
    }
    return true;
}

}
