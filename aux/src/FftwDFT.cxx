#include "WireCellAux/FftwDFT.h"
#include <fftw3.h>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>


using namespace WireCell;

using plan_key_t = int64_t;
using plan_type = fftwf_plan;
using plan_map_t = std::unordered_map<plan_key_t, plan_type>;
using plan_val_t = fftwf_complex;

static
plan_key_t make_key(bool inverse, const void * src, void * dst, int n0, int n1)
{
    bool inplace = (dst==src);
    bool aligned = ( (reinterpret_cast<size_t>(src)&15) | (reinterpret_cast<size_t>(dst)&15) ) == 0;
    int64_t key = ( ( (((int64_t)n0) << 30)|(n1<<3 ) | (inverse<<2) | (inplace<<1) | aligned ) << 1 ) + 1;
    return key;
}

static
plan_type get_plan(std::shared_mutex& mutex, plan_map_t& plans, plan_key_t key)
{
    std::shared_lock lock(mutex);
    auto it = plans.find(key);
    if (it == plans.end()) {
        return NULL;
    }
    return it->second;
}


template<typename planner_function>
void doit(std::shared_mutex& mutex, plan_map_t& plans, 
                int fwdrev, plan_val_t* src, plan_val_t* dst, int stride, int nstrides,
                planner_function make_plan)
{
    auto key = make_key(fwdrev == FFTW_BACKWARD, src, dst, stride, nstrides);
    auto plan = get_plan(mutex, plans, key);
    if (!plan) {
        std::unique_lock lock(mutex);
        // Check again in case another thread snakes us.
        auto it = plans.find(key);
        if (it == plans.end()) {
            plan = make_plan();
            plans[key] = plan;
        }
        else {
            plan = it->second;
        }
    }
    fftwf_execute_dft(plan, src, dst);
}


static
plan_val_t* pval_cast( const IDFT::complex_t * p)
{ 
    return const_cast<plan_val_t*>( reinterpret_cast<const plan_val_t*>(p) ); 
}


void Aux::FftwDFT::fwd1d(const complex_t* in, complex_t* out, int stride) const
{
    static std::shared_mutex mutex;
    static plan_map_t plans;
    static const int dir = FFTW_FORWARD;
    auto src = pval_cast(in);
    auto dst = pval_cast(out);
    doit(mutex, plans, dir, src, dst, stride, 0, [&]( ) {
        return fftwf_plan_dft_1d(stride, src, dst, dir, FFTW_ESTIMATE|FFTW_PRESERVE_INPUT);
    });
}
void Aux::FftwDFT::inv1d(const complex_t* in, complex_t* out, int stride) const
{
    static std::shared_mutex mutex;
    static plan_map_t plans;
    static const int dir = FFTW_BACKWARD;
    auto src = pval_cast(in);
    auto dst = pval_cast(out);
    doit(mutex, plans, dir, src, dst, stride, 0, [&]( ) {
        return fftwf_plan_dft_1d(stride, src, dst, dir, FFTW_ESTIMATE|FFTW_PRESERVE_INPUT);
    });
}


void Aux::FftwDFT::fwd2d(const complex_t* in, complex_t* out, int stride, int nstrides) const
{
    static std::shared_mutex mutex;
    static plan_map_t plans;
    static const int dir = FFTW_FORWARD;
    auto src = pval_cast(in);
    auto dst = pval_cast(out);
    doit(mutex, plans, dir, src, dst, stride, nstrides, [&]( ) {
        return fftwf_plan_dft_2d(stride, nstrides, src, dst, dir, FFTW_ESTIMATE|FFTW_PRESERVE_INPUT);
    });
}


void Aux::FftwDFT::inv2d(const complex_t* in, complex_t* out, int stride, int nstrides) const
{
    static std::shared_mutex mutex;
    static plan_map_t plans;
    static const int dir = FFTW_BACKWARD;
    auto src = pval_cast(in);
    auto dst = pval_cast(out);
    doit(mutex, plans, dir, src, dst, stride, nstrides, [&]( ) {
        return fftwf_plan_dft_2d(stride, nstrides, src, dst, dir, FFTW_ESTIMATE|FFTW_PRESERVE_INPUT);
    });
}
