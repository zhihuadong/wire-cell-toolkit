#ifndef WIRECELLAUX_FFTWDFT
#define WIRECELLAUX_FFTWDFT

#include "WireCellIface/IDFT.h"

namespace WireCell::Aux {

    /** 
        FftwDFT provides IDFT based on FFTW3.        
    */
    class FftwDFT : public IDFT {
      public:
        
        FftwDFT();
        virtual ~FftwDFT();

        // 1d 

        virtual 
        void fwd1d(const complex_t* in, complex_t* out,
                   int stride) const;

        virtual 
        void inv1d(const complex_t* in, complex_t* out,
                   int stride) const;

        // batched 1D ("1b") - rely on base implementation

        // 2d

        virtual 
        void fwd2d(const complex_t* in, complex_t* out,
                   int stride, int nstrides) const;
        virtual 
        void inv2d(const complex_t* in, complex_t* out,
                   int stride, int nstrides) const;


    };
}

#endif
