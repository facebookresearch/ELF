# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import os
import sys
import re
import argparse
from collections import defaultdict

def type_modifier(t):
    return t if t in ["int", "float"] else "const %s&" % t

def init_modifier(t, n, d):
    ret = type_modifier(t) + " " + n
    if d is not None:
        ret += " = " + d
    return ret

def get_class_and_enum_name(s):
    classname = "Cmd" + s
    enum_name = s[0].upper()
    for i in range(len(s) - 1):
        if s[i].islower() and s[i + 1].isupper():
            enum_name += "_"
        enum_name += s[i + 1].upper()
    return classname, enum_name

template = """
#define $enum_name $idx

class $classname : public $baseclass {
protected:
$var_decl
$override_run

public:
    explicit $classname() { }
    explicit $classname(UnitId id$var_init_list) : $baseclass(id)$var_initializer { }
    CmdType type() const override { return $enum_name; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<$classname>(new $classname(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->$baseclass::PrintInfo() $var_ostream;
        return ss.str();
    }
$accessors
$serializer
};
"""

def read_cmd_def(def_filename, content, classes):
    idx = None
    for line in open(def_filename, "r"):
        line = line.strip()
        # Simply parse the line and construct the Cmd definition.
        m = re.match(r"CMD_(.*?)\((.*?)\);", line)
        if not m: continue

        if m.group(1) == "DURATIVE":
            baseclass = "CmdDurative"
            override_run = "    bool run(const GameEnv& env, CmdReceiver *) override;"

        elif m.group(1) == "IMMEDIATE":
            baseclass = "CmdImmediate"
            override_run = "    bool run(GameEnv* env, CmdReceiver *) override;"

        elif m.group(1) == "START":
            idx = int(m.group(2))
            continue
        else:
            continue

        items = [v.strip() for v in m.group(2).split(",")]
        classname, enum_name = get_class_and_enum_name(items[0])
        classes[classname] = dict(bases=["CmdBase", baseclass], enum=enum_name)
        types = items[1::2]
        names = items[2::2]

        # Check default argument.
        defaults = map(lambda x: x.split("=")[1].strip() if len(x.split("=")) == 2 else None, names)
        names = [ n.split("=")[0].strip() for n in names ]

        s_name = "SERIALIZER_DERIVED"
        if len(names) == 0: s_name += "0"
        serializer = "    " + s_name + "(%s, %s" % (classname, baseclass) + "".join(", _%s" % n for n in names) + ");"

        # Compute the variable declaration and variable assignments.
        symbols = dict(
            idx = str(idx),
            baseclass = baseclass,
            override_run = override_run,
            enum_name = enum_name,
            classname = classname,
            var_decl = "\n".join("    %s _%s;" % (t, n) for t, n in zip(types, names)) + "\n",
            var_init_list = "".join(", " + init_modifier(t, n, d) for t, n, d in zip(types, names, defaults)),
            serializer = serializer,
            var_initializer = "".join(", _%s(%s)" % (n, n) for n in names),
            var_ostream = " ".join("<< \" [%s]: \" << _%s" % (n, n) for n in names),
            accessors = "\n".join("    %s %s() const { return _%s; }" % (type_modifier(t), n, n) for t, n in zip(types, names))
        )
        text = template
        for k, v in symbols.items():
            text = text.replace("$" + k, v)

        content.append(text)
        idx += 1

parser = argparse.ArgumentParser()
parser.add_argument("--def_file", type=str)
parser.add_argument("--name", type=str)

args = parser.parse_args()

content = []
classes = defaultdict(list)

cmd_def = args.def_file + ".def"
read_cmd_def(cmd_def, content, classes)

cmd_header = args.def_file + ".gen.h"

output = open(cmd_header, "w")
output.write("#pragma once\n")
for text in content:
    output.write(text)

output.write("\n")
output.write("inline void reg_%s() {\n" % args.name)
for class_name, info in classes.items():
    output.write("    CmdTypeLookup::RegCmdType(%s, \"%s\");\n" % (info["enum"], info["enum"]))
    for baseclass in info["bases"]:
        output.write("    SERIALIZER_ANCHOR_FUNC(%s, %s);\n" % (baseclass, class_name))
output.write("}\n")
output.close()

