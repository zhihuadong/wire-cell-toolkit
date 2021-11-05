#include "WireCellAux/DftTools.h"

using namespace WireCell;
using namespace WireCell::Aux;

/*
  Big fat warning to future me: Passing by reference means the input
  array may carry the .IsRowMajor optimization for implementing
  transpose().  An extra copy would remove that complication but this
  interface tries to keep it.
 */

using ROWM = Eigen::Array<Aux::complex_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using COLM = Eigen::Array<Aux::complex_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;

template<typename trans>
Aux::dft_array_t doit(const Aux::dft_array_t& arr, trans func)
{
    // Nominally, memory is in column-major order
    const Aux::complex_t* in_data = arr.data();
    int stride = arr.rows();
    int nstrides = arr.cols();

    // except when it isn't
    bool flipped = arr.IsRowMajor;
    if (flipped) {
        stride = arr.cols();
        nstrides = arr.rows();
    }

    Aux::dft_vector_t out_vec(nstrides*stride);
    func(in_data, out_vec.data(), nstrides, stride);

    if (flipped) {
        return Eigen::Map<ROWM>(out_vec.data(), arr.rows(), arr.cols());
    }
    return Eigen::Map<COLM>(out_vec.data(), arr.rows(), arr.cols());

}

Aux::dft_array_t Aux::fwd(IDFT::pointer dft, const Aux::dft_array_t& arr)
{
    return doit(arr, [&](const complex_t* in_data,
                         complex_t* out_data,
                         int nstrides, int stride) {
        dft->fwd2d(in_data, out_data, nstrides, stride);
    });
}

Aux::dft_array_t Aux::inv(IDFT::pointer dft, const Aux::dft_array_t& arr)
{
    return doit(arr, [&](const complex_t* in_data,
                         complex_t* out_data,
                         int nstrides, int stride) {
        dft->inv2d(in_data, out_data, nstrides, stride);
    });
}
