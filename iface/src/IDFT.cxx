#include "WireCellIface/IDFT.h"

using namespace WireCell;

IDFT::~IDFT() {}

// Trivial default "batched" implementations.  If your concrete
// implementation provides some kind of "batch optimization", such as
// with some GPU FFTs, override these methods!

void IDFT::fwd1b(const complex_t* in, complex_t* out,
                 int nstrides, int stride) const
{
    for (int istride=0; istride<nstrides; ++istride) {
        fwd1d(in+istride*stride, out+istride*stride, stride);
    }
}

void IDFT::inv1b(const complex_t* in, complex_t* out,
                 int nstrides, int stride) const
{
    for (int istride=0; istride<nstrides; ++istride) {
        inv1d(in+istride*stride, out+istride*stride, stride);
    }
}
