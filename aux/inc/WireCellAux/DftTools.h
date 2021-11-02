/**
   High level functions related to DFTs.

   Most take an IDFT::pointer to a DFT implementation and return an
   allocated result.  Use IDFT directly to control allocation.

   There are std::vector and Eigen array functions.

   Abbreviations:

   - IS is interval space aka time / distance
   - FS is frequency space aka frequency / periodicity 

   Price to pay for simple API is a lack of optimizations:

   - When a real valued array is invovled, all arrays are full size.
     That is, no half-size optimization will be exposed to the caller.

   - These functions tend to make more copies than may be needed if
     IDFT is called directly.  In addition to real/complex conversion,
     using std::vector or Eigen array instead of raw memory leads to
     more copies.
 */

#ifndef WIRECELL_AUX_DFTTOOLS
#define WIRECELL_AUX_DFTTOOLS

#include "WireCellIface/IDFT.h"
#include <vector>
#include <Eigen/Core>

namespace WireCell::Aux {

    using real_t = IDFT::scalar_t;
    using complex_t = IDFT::complex_t;

    // std::vector based functions

    using realvec_t = std::vector<real_t>;
    using compvec_t = std::vector<complex_t>;

    // 1D with vectors

    // Transform a real IS, return same size FS.
    compvec_t dft(IDFT::pointer dft, const realvec_t& seq);
        
    // Transform complex FS to IS and return real part
    realvec_t idft(IDFT::pointer dft, const compvec_t& spec);

    compvec_t r2c(const realvec_t& r);
    realvec_t c2r(const compvec_t& c);


    // Eigen array based functions

    /// Real 1D array
    using array_xf = Eigen::ArrayXf;

    /// Complex 1D array
    using array_xc = Eigen::ArrayXcf;

    /// A real, 2D array
    using array_xxf = Eigen::ArrayXXf;

    /// A complex, 2D array
    using array_xxc = Eigen::ArrayXXcf;
    
    // 2D with Eigen arrays

    // Transform a real IS, return same size FS.
    array_xxc dft(IDFT::pointer dft, const array_xxf& arr);

    // Transform complex FS to IS and return real part
    array_xxf idft(IDFT::pointer dft, const array_xxc& arr);


}

#endif
