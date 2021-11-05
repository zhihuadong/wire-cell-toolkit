/**
   This provides std::vector and Eigen::Array typed interface to an
   IDFT.
 */

#ifndef WIRECELL_AUX_DFTTOOLS
#define WIRECELL_AUX_DFTTOOLS

#include "WireCellIface/IDFT.h"
#include <vector>
#include <Eigen/Core>

namespace WireCell::Aux {

    using complex_t = IDFT::complex_t;

    // std::vector based functions

    using dft_vector_t = std::vector<complex_t>;

    // 1D with vectors

    inline dft_vector_t fwd(IDFT::pointer dft, const dft_vector_t& seq)
    {
        dft_vector_t ret(seq.size());
        dft->fwd1d(seq.data(), ret.data(), ret.size());
        return ret;
    }

    inline dft_vector_t inv(IDFT::pointer dft, const dft_vector_t& spec)
    {
        dft_vector_t ret(spec.size());
        dft->inv1d(spec.data(), ret.data(), ret.size());
        return ret;
    }

    // Eigen array based functions

    /// A complex, 2D array.  Use Array::cast<type>() if you need to
    /// convert to/from real.
    using dft_array_t = Eigen::ArrayXXcf;
    
    // 2D with Eigen arrays.  Use eg arr.cast<complex_>() to provde
    // from real or arr.cast<float>() to convert result to real.

    // Transform both dimesions.
    dft_array_t fwd(IDFT::pointer dft, const dft_array_t& arr);
    dft_array_t inv(IDFT::pointer dft, const dft_array_t& arr);

    // Transform one dimesions.  For example axis=0 transforms each
    // logical row of the Eigen array so that column=0 of each row
    // would hold the frequency=0 component of each row's spectrum.  
    // array_xxc fwd(IDFT::pointer dft, const array_xxc& arr, int axis);
    // array_xxc inv(IDFT::pointer dft, const array_xxc& arr, int axis);


}

#endif
