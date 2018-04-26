/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include <iostream>
#include "sgf.h"

int main() {
    Sgf sgf;
    sgf.Load("sample1.sgf");
    cout << sgf.PrintHeader() << endl;
    cout << sgf.PrintMainVariation() << endl;
    return 0;
}
