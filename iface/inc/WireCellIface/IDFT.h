/** 
    Interface to perform discrete single-precision Fourier transforms.
*/

#ifndef WIRECELL_IDFT
#define WIRECELL_IDFT

#include "WireCellUtil/IComponent.h"
#include <complex>

namespace WireCell {

    class IDFT  : public IComponent<IDFT> {
      public:
        virtual ~IDFT();

        /// The type for the signal in each bin.
        using scalar_t = float;

        /// The type for the spectrum in each bin.
        using complex_t = std::complex<scalar_t>;

        // 1D 

        virtual 
        void fwd1d(const complex_t* in, complex_t* out,
                   int stride) const = 0;

        virtual 
        void inv1d(const complex_t* in, complex_t* out,
                   int stride) const = 0;

        // batched 1D ("1b")

        virtual 
        void fwd1b(const complex_t* in, complex_t* out,
                   int stride, int nstrides) const;
        virtual 
        void inv1b(const complex_t* in, complex_t* out,
                   int stride, int nstrides) const;


        // 2D, transform both dimensions

        virtual 
        void fwd2d(const complex_t* in, complex_t* out,
                   int stride, int nstrides) const = 0;
        virtual 
        void inv2d(const complex_t* in, complex_t* out,
                   int stride, int nstrides) const = 0;

     };
}


#endif
