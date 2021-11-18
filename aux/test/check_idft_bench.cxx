/**
   A simple benchmark of IDFT for payloads relevant to WCT
 */

#include "aux_test_dft_helpers.h"

using namespace WireCell;
using namespace WireCell::Aux::Test;

using benchmark_function = std::function<void()>;
using complex_t = std::complex<float>;


// benchmarks span outer product of:
// - in-place / out-place
// - 1d, 1b, 2d
// - sizes: perfect powers of 2 and with larger prime factors
// - use repitition numbers to keep each test roughly same runtime

using transform_function = std::function<void(const complex_t* in, complex_t* out)>;

const int nominal = 100'000'000;
void doit(Stopwatch& sw, const std::string& name, int nrows, int ncols, transform_function func)
{
    const int size = nrows*ncols;
    const int ntimes = std::max(1, nominal / size);
    std::cerr << name << ": (" << nrows << "," << ncols << ") x "<<ntimes<<"\n";

    std::vector<complex_t> in(size), out(size);

    sw([&](){func(in.data(), in.data());}, {
            {"nrows",nrows}, {"ncols",ncols}, {"func",name}, {"ntimes",1}, {"first",true}, {"in-place",true},
        });

    sw([&](){
        for (int count=0; count<ntimes+1; ++count) {
            func(in.data(), in.data());
        }}, {
            {"nrows",nrows}, {"ncols",ncols}, {"func",name}, {"ntimes",ntimes}, {"first",false}, {"in-place",true},
        });

    sw([&](){func(in.data(), out.data());}, {
            {"nrows",nrows}, {"ncols",ncols}, {"func",name}, {"ntimes",1}, {"first",true}, {"in-place",false},
        });

    sw([&](){
        for (int count=0; count<ntimes+1; ++count) {
            func(in.data(), out.data());
        }}, {
            {"nrows",nrows}, {"ncols",ncols}, {"func",name}, {"ntimes",ntimes}, {"first",false}, {"in-place",false},
        });
}

int main(int argc, char* argv[])
{
    auto args = make_dft_args(argc, argv);
    auto idft = make_dft(args.tn, args.pi, args.cfg);

    Stopwatch sw({
            {"typename",args.tn},
            {"plugin",args.pi},
            {"config", object_t::parse(Persist::dumps(args.cfg))}, 
            {"config_file",args.cfg_name}});

    std::string cname = args.cfg_name;
    auto slash = cname.rfind("/");
    if (slash != std::string::npos) {
        cname = cname.substr(slash+1);
    }
    cname = cname.substr(0, cname.rfind("."));
    std::string fname = argv[0];
    fname += "_" + args.pi + "_" + args.tn + "_" + cname + ".json";
    std::cerr << "writing to: " << fname << std::endl;
    

    std::vector<int> oned_sizes{500, 512, 1000, 1024, 4096, 6000, 8192, 10000, 16384};
    for (auto size : oned_sizes) {
        doit(sw, "fwd1d", 1, size, [&](const complex_t* in, complex_t* out) {
            idft->fwd1d(in, out, size);
        });

        doit(sw, "inv1d", 1, size, [&](const complex_t* in, complex_t* out) {
            idft->inv1d(in, out, size);
        });
    }

    // channel count from some detectors plus powers of 2
    std::vector<std::pair<int,int>> twod_sizes{
        {800,6000}, {960,6000}, // protodune u/v and w 3ms
        {2400, 9595}, {3456, 9595}, // uboone u/v daq size
        {1024, 1024}, {2048, 2048}, {4096, 4096}, // perfect powers of 2
    };
    for (auto& [nrows,ncols] : twod_sizes) {

        doit(sw, "fwd2d", nrows, ncols, [&](const complex_t* in, complex_t* out) {
            idft->fwd2d(in, out, nrows, ncols);
        });
        doit(sw, "inv2d", nrows, ncols, [&](const complex_t* in, complex_t* out) {
            idft->inv2d(in, out, nrows, ncols);
        });

        doit(sw, "fwd1b0", nrows, ncols, [&](const complex_t* in, complex_t* out) {
            idft->fwd1b(in, out, nrows, ncols, 0);
        });
        doit(sw, "inv1b0", nrows, ncols, [&](const complex_t* in, complex_t* out) {
            idft->inv1b(in, out, nrows, ncols, 0);
        });
        doit(sw, "fwd1b1", nrows, ncols, [&](const complex_t* in, complex_t* out) {
            idft->fwd1b(in, out, nrows, ncols, 1);
        });
        doit(sw, "inv1b1", nrows, ncols, [&](const complex_t* in, complex_t* out) {
            idft->inv1b(in, out, nrows, ncols, 1);
        });
    }
    
    sw.save(fname);

    return 0;
}
