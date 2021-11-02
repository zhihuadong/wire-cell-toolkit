#include "WireCellAux/DftTools.h"

using namespace WireCell;
using namespace WireCell::Aux;

compvec_t Aux::r2c(const realvec_t& r)
{
    compvec_t cret(r.size());
    std::transform(r.begin(), r.end(), cret.begin(),
                   [](const real_t& r) { return complex_t(r, 0); });
    return cret;
}
realvec_t Aux::c2r(const compvec_t& c)
{
    realvec_t rret(c.size());
    std::transform(c.begin(), c.end(), rret.begin(),
                   [](const complex_t& c) { return std::real(c); });
    return rret;
}

// Transform a real IS, return same size FS.
compvec_t Aux::dft(IDFT::pointer dft, const realvec_t& seq)
{
    compvec_t cseq = Aux::r2c(seq);
    compvec_t cret(cseq.size());
    dft->fwd1d(cseq.data(), cret.data(), cret.size());
    return cret;
}
        
// Transform complex FS to IS and return real part
realvec_t Aux::idft(IDFT::pointer dft, const compvec_t& spec)
{
    compvec_t cret(spec.size());
    dft->inv1d(spec.data(), cret.data(), cret.size());
    return Aux::c2r(cret);
}

using array_xxf_rm = Eigen::Array<real_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using array_xxc_rm = Eigen::Array<complex_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;


// Transform a real IS, return same size FS.
array_xxc Aux::dft(IDFT::pointer trans, const array_xxf& arr)
{
    int stride = arr.rows();
    int nstrides = arr.cols();
    array_xxc ret(stride, nstrides);

    if (!arr.IsRowMajor) {
        stride = arr.cols();
        nstrides = arr.rows();
    }

    size_t size = stride*nstrides;
    compvec_t carr(size);
    std::transform(arr.data(), arr.data()+size, carr.begin(),
                   [](const real_t& r) { return complex_t(r,0); });
    
    trans->fwd2d(carr.data(), ret.data(), stride, nstrides);
    return ret;
}

// Transform complex FS to IS and return real part
array_xxf Aux::idft(IDFT::pointer trans, const array_xxc& arr)
{
    int stride = arr.rows();
    int nstrides = arr.cols();
    array_xxf ret(stride, nstrides);

    if (!arr.IsRowMajor) {
        stride = arr.cols();
        nstrides = arr.rows();
    }

    size_t size = stride*nstrides;
    compvec_t cret(size);
    trans->inv2d(arr.data(), cret.data(), stride, nstrides);

    std::transform(cret.begin(), cret.end(), ret.data(),
                   [](const complex_t& c) { return std::real(c); });
    return ret;
}
