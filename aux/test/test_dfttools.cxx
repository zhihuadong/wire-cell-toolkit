#include "WireCellAux/DftTools.h"
#include "WireCellAux/FftwDFT.h"

#include <iostream>
#include <memory>

using namespace WireCell;
using namespace WireCell::Aux;

void test_1d_imp(IDFT::pointer trans)
{
    realvec_t rimp(64, 0);
    rimp[1] = 1.0;
    auto cimp = dft(trans, rimp);
    for (auto c : cimp) {
        std::cerr << c << " ";
    }
    std::cerr << "\n";
}

int main()
{
    auto trans = std::make_shared<FftwDFT>();

    test_1d_imp(trans);

    return 0;
}
