// A wire cell version of the jsonnet cli.

#include "WireCellUtil/Persist.h"
#include <iostream>

using namespace std;
using namespace WireCell;

int main(int argc, char* argv[])
{
    Persist::Parser parser;
    auto jdat = parser.load(argv[1]);
    cout << jdat << endl;

    return 0;
}
