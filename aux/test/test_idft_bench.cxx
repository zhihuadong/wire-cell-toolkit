/**
   A simple benchmark of IDFT for payloads relevant to WCT
 */

#include "WireCellUtil/TimeKeeper.h"

#include "aux_test_dft_helpers.h"

#include <string>
#include <complex>
#include <iostream>

using namespace WireCell;
using namespace WireCell::Aux::Test;

using benchmark_function = std::function<void()>;
using complex_t = std::complex<float>;

void timeit(TimeKeeper& tk, const std::string& msg, benchmark_function func)
{
    tk("\tINIT\t" + msg);
    func();
    tk("\tFINI\t" + msg);
}

// benchmarks span outer product of:
// - in-place / out-place
// - 1d, 1b, 2d
// - sizes: perfect powers of 2 and with larger prime factors
// - use repitition numbers to keep each test roughly same runtime

using transform_function = std::function<void(const complex_t* in, complex_t* out)>;

const int onedfull = 100'000'000;
void doit(TimeKeeper& tk, const std::string& name, int nrows, int ncols, bool inplace, transform_function func)
{
    const int size = nrows*ncols;
    const int ntimes = onedfull / size;
    std::stringstream ss;
    ss << "\t(" << nrows << "," << ncols << ")\t" << ntimes << "\t";
    std::string s = ss.str();
    
    if (inplace) {
        timeit(tk, s + "in-place\t" + name, [&]() {
            std::vector<complex_t> in(size);
            for (int count=0; count<ntimes; ++count) {
                func(in.data(), in.data());
            }});
    }
    else {
        timeit(tk, s + "separate\t" + name, [&]() {
            std::vector<complex_t> in(size), out(size);
            for (int count=0; count<ntimes; ++count) {
                func(in.data(), out.data());
            }});
    }
}

int main(int argc, char* argv[])
{
    auto idft = make_dft_args(argc, argv);

    TimeKeeper tk("IDFT benchmark\t\tsize\t\tcount\toverlap\t\ttran");

    int size = 10'000;
    doit(tk, "fwd1d", 1, size, false, [&](const complex_t* in, complex_t* out) {
        idft->fwd1d(in, out, size);
    });

    doit(tk, "inv1d", 1, size, false, [&](const complex_t* in, complex_t* out) {
        idft->inv1d(in, out, size);
    });

    int nrows = 1000;
    int ncols = 1000;
    doit(tk, "fwd2d", nrows, ncols, false, [&](const complex_t* in, complex_t* out) {
        idft->fwd2d(in, out, nrows, ncols);
    });
    doit(tk, "inv2d", nrows, ncols, false, [&](const complex_t* in, complex_t* out) {
        idft->inv2d(in, out, nrows, ncols);
    });

    doit(tk, "fwd1b0", nrows, ncols, false, [&](const complex_t* in, complex_t* out) {
        idft->fwd1b(in, out, nrows, ncols, 0);
    });
    doit(tk, "inv1b0", nrows, ncols, false, [&](const complex_t* in, complex_t* out) {
        idft->inv1b(in, out, nrows, ncols, 0);
    });
    doit(tk, "fwd1b1", nrows, ncols, false, [&](const complex_t* in, complex_t* out) {
        idft->fwd1b(in, out, nrows, ncols, 1);
    });
    doit(tk, "inv1b1", nrows, ncols, false, [&](const complex_t* in, complex_t* out) {
        idft->inv1b(in, out, nrows, ncols, 1);
    });


    std::cerr << tk.summary() << std::endl;

    return 0;
}
