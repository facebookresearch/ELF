#include <iostream>
#include "sgf.h"

int main() {
    Sgf sgf;
    sgf.Load("sample1.sgf");
    cout << sgf.PrintHeader() << endl;
    cout << sgf.PrintMainVariation() << endl;
    return 0;
}
