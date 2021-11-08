#include "WireCellAux/DftTools.h"
#include "WireCellAux/FftwDFT.h"
#include "WireCellUtil/Waveform.h"

#include <iostream>
#include <memory>

using namespace WireCell;

using real_t = float;
using RV = std::vector<real_t>;
using complex_t = std::complex<real_t>;
using CV = std::vector<complex_t>;

void test_1d(IDFT::pointer dft)
{
    RV rimp(64, 0);
    rimp[1] = 1.0;

    auto cimp = Aux::fwd(dft, Waveform::complex(rimp));
    for (auto c : cimp) {
        std::cerr << c << " ";
    }
    std::cerr << "\n";

    RV rimp2 = Waveform::real(Aux::inv(dft, cimp));
    for (auto r : rimp2) {
        std::cerr << r << " ";
    }
    std::cerr << "\n";
    for (int ind=0; ind<64; ++ind) {
        if (ind == 1) {
            assert(std::abs(rimp2[ind]-1.0) < 1e-6);
            continue;
        }
        assert(std::abs(rimp2[ind]) < 1e-6);
    }
}

using FA = Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic>;

void test_2d(IDFT::pointer dft)
{
    const int nrows=16;
    const int ncols=8;
    FA r = FA::Zero(nrows, ncols);
    r(10,1) = 1.0;
    std::cerr << r << std::endl;
    auto c = Aux::fwd(dft, r.cast<complex_t>());
    std::cerr << c << std::endl;
    FA r2 = Aux::inv(dft, c).real();
    std::cerr << r2 << std::endl;
    for (int irow=0; irow<nrows; ++irow) {
        for (int icol=0; icol<ncols; ++icol) {
            if (irow==10 and icol==1) {
                assert(std::abs(r2(irow, icol)-1.0) < 1e-6);
                continue;
            }
            assert(std::abs(r2(irow, icol)) < 1e-6);    
        }
    }
}

template<typename array_type>
void dump(std::string name, const array_type& arr)
{
    std::cerr << name << ":(" << arr.rows() << "," << arr.cols() << ") row-major:" << arr.IsRowMajor << "\n";
}

void test_2d_transpose(IDFT::pointer dft)
{
    const int nrows=16;
    const int ncols=8;

    FA r = FA::Zero(nrows, ncols); // shape:(16,8)
    dump("r", r);

    // do not remove the auto in this next line
    auto rt = r.transpose();    // shape:(8,16)
    dump("rt", rt);
    rt(1,10) = 1.0;

    auto c = Aux::fwd(dft, rt.cast<complex_t>());
    dump("c", c);

    auto r2 = Aux::inv(dft, c).real();
    dump("r2",r2);

    // transpose access
    const int nrowst = r2.rows();
    const int ncolst = r2.cols();

    for (int irow=0; irow<nrowst; ++irow) {
        for (int icol=0; icol<ncolst; ++icol) {
            float val = rt(irow, icol);
            float val2 = r2(irow, icol); // access with transposed indices
            std::cerr << "(" << irow << ","<< icol << "):" << val << " ? " << val2 << "\n";
            if (irow==1 and icol==10) {
                assert(std::abs(val-1.0) < 1e-6);
                continue;
            }
            assert(std::abs(val) < 1e-6);    
        }
        std::cerr << "\n";
    }

    
}

void test_1b(IDFT::pointer dft, int axis)
{
    const int nrows=8; 
    const int ncols=4;
    FA r = FA::Zero(nrows, ncols);
    r(6,1) = 1.0;
    dump("impulse", r);
    std::cerr << r << std::endl;
    auto c = Aux::fwd(dft, r.cast<complex_t>(), axis);
    dump("spectra", c);
    if (axis==0) {
        
    }
    std::cerr << c << std::endl;
}
void test_1bt(IDFT::pointer dft, int axis)
{
    const int nrows=8; 
    const int ncols=4;
    FA r = FA::Zero(nrows, ncols);
    r(6,1) = 1.0;
    auto rc = r.cast<complex_t>();
    auto rct = rc.transpose();
    dump("impulse.T", rct);
    std::cerr << rct << std::endl;
    auto c = Aux::fwd(dft, rct, axis);
    dump("spectra", c);
    std::cerr << c << std::endl;
}

int main()
{
    auto dft = std::make_shared<Aux::FftwDFT>();

    test_1d(dft);
    test_2d(dft);
    test_2d_transpose(dft);
    test_1b(dft, 0);
    test_1b(dft, 1);
    test_1bt(dft, 0);
    test_1bt(dft, 1);
    return 0;
}
