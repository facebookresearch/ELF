# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import cffi
import sys
import os

def parse_header(basedir, header_name, loaded_header):
    content = []
    macros = { }
    export_start = False
    loaded_header.add(header_name)

    for line in open(os.path.join(basedir, header_name), "r"):
        save = True
        line = line.strip()
        if line.startswith("#"):
            # precompiled macro
            if line.startswith("#include"):
                items = list(filter(lambda x : len(x) > 0, line.split(" ")))
                new_header_file = items[1].strip()[1:-1]
                if not new_header_file in loaded_header:
                    content_new, macros_new = parse_header(basedir, new_header_file, loaded_header)
                    content += content_new
                    macros.update(macros_new)

            if not line.startswith("#define"):
                # Omit all precompiled macros that are not definition.
                save = False
            elif line.startswith("#define _"):
                # Omit definition of internal variable.
                save = False
            else:
                # Parse the macro definition.
                items = list(filter(lambda x: len(x) > 0, line.split(" ")))
                symbol = items[1].strip()
                value = int(items[2])
                macros[symbol] = value

        elif line.startswith("extern \"C\""):
            export_start = True
            save = False
        elif line.startswith("} // extern \"C\""):
            export_start = False
            save = False
        elif line.startswith("//"):
            save = False

        if export_start and save:
            content.append(line)

    return content, macros

def initialize_ffi(header_filename = "python_options.h", dll_filename = "main.so", file_dir = None):
    # Load header files.
    if file_dir is None:
        file_dir = os.path.dirname(__file__)
    header_content, symbols = parse_header(file_dir, header_filename, set())
    header = "\n".join(header_content)

    ffi = cffi.FFI()
    ffi.cdef(header)

    C = ffi.dlopen(os.path.join(file_dir, dll_filename))
    return dict(ffi = ffi, C = C, symbols = symbols)

__all__ = ["_initialize_ffi", "parse_header"]
