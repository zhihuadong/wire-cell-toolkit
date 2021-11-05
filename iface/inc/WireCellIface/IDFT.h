/** 
    Interface to perform discrete single-precision Fourier transforms.

    Note, implementations MUST NOT normalize forward transforms and
    MUST normalize reverse/inverse transforms by 1/n where n is the
    number of elements in the 1D array being reverse transformed.

    The number "stride" describes how many elements of the array are
    contiguous.  For "C-order" aka row-major ordering of 2D arrays,
    stride is the size of a row, aka number of columns.

    The number "nstrides" describe how many arrays of length "stride"
    are placed end-to-end in the memory.  For "C-order" aka row-major
    ordering of 2D arrays, the "nstrides" counts the size of the
    columns, aka the number of rows.  With this ordering, the
    (nstrides, stride) pair maps to the usual (nrows, ncols).
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
                   int nstrides, int stride) const;
        virtual 
        void inv1b(const complex_t* in, complex_t* out,
                   int nstrides, int stride) const;


        // 2D, transform both dimensions

        virtual 
        void fwd2d(const complex_t* in, complex_t* out,
                   int nstrides, int stride) const = 0;
        virtual 
        void inv2d(const complex_t* in, complex_t* out,
                   int nstrides, int stride) const = 0;

     };
}


#endif
