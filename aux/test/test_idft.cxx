// Test IDFT implementations.
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IDFT.h"

#include <chrono>
#include <vector>
#include <thread>
#include <iostream>

using namespace WireCell;


static
void test_1d_zero(IDFT::pointer dft, int size = 1024)
{
    std::vector<IDFT::complex_t> inter(size,0), freq(size,0);

    dft->fwd1d(inter.data(), freq.data(), inter.size());
    dft->inv1d(freq.data(), inter.data(), freq.size());

    auto tot = Waveform::sum(inter);
    assert(std::real(tot) == 0);
}
static
void test_2d_zero(IDFT::pointer dft, int size = 1024)
{
    int stride=size, nstrides=size;
    std::vector<IDFT::complex_t> inter(stride*nstrides,0);
    std::vector<IDFT::complex_t> freq(stride*nstrides,0);

    dft->fwd2d(inter.data(), freq.data(), stride, nstrides);
    dft->inv2d(freq.data(), inter.data(), stride, nstrides);

    auto tot = Waveform::sum(inter);
    assert(std::real(tot) == 0);
}

void fwdrev(IDFT::pointer dft, int id, int ntimes, int size)
{
    int stride=size, nstrides=size;
    std::vector<IDFT::complex_t> inter(stride*nstrides,0);
    std::vector<IDFT::complex_t> freq(stride*nstrides,0);

    // std::cerr << "running " << id << std::endl;

    while (ntimes) {
        //std::cerr << ntimes << "\n";
        dft->fwd2d(inter.data(), freq.data(), stride, nstrides);
        dft->inv2d(freq.data(), inter.data(), stride, nstrides);

        --ntimes;        
        auto tot = Waveform::sum(inter);
        assert(std::real(tot) == 0);
    }
    //std::cerr << "finished " << id << std::endl;
}

static
void test_2d_threads(IDFT::pointer dft, int nthreads, int nloops, int size = 1024)
{
    using namespace std::chrono;

    steady_clock::time_point t1 = steady_clock::now();

    std::vector<std::thread> workers;

    //std::cerr << "Starting workers\n";
    for (int ind=0; ind<nthreads; ++ind) {
        workers.emplace_back(fwdrev, dft, ind, nloops, size);
    }
    //std::cerr << "Waiting for workers\n";
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> dt1 = duration_cast<duration<double>>(t2 - t1);
    std::cerr << "ndfts: " << nthreads*nloops
              << " " << nthreads << " " << nloops
              << " " << dt1.count() << std::endl;
}

int main(int argc, char* argv[])
{
    // fixme, add CLI parsing to add plugins, config and name another
    // dft.  For now, just use the one in aux.
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellAux");
    std::string dft_tn = "FftwDFT";

    // creates
    auto idft = Factory::lookup_tn<IDFT>(dft_tn);
    assert(idft);
    {                          // configure before use if configurable
        auto icfg = Factory::find_maybe_tn<IConfigurable>(dft_tn);
        if (icfg) {
            auto cfg = icfg->default_configuration();
            icfg->configure(cfg);
        }
    }

    test_1d_zero(idft);
    test_2d_zero(idft);

    std::vector<int> sizes = {128,256,512,1024};
    for (auto size : sizes) {
        int ndouble=3, ntot=2*16384/size;
        while (ndouble) {
            int nthread = 1<<ndouble;
            int nloop = ntot/nthread;
            --ndouble;
            std::cerr << "size=" << size << " nthread=" << nthread << " nloop=" << nloop << "\n";
            test_2d_threads(idft, nthread, nloop, size);
        }
    }

    return 0;
}
