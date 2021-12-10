// This is some "private" code shared by a couple of components in gen.

#include "WireCellIface/IRandom.h"
#include "WireCellUtil/Waveform.h"

#include <vector>

namespace WireCell::Gen::Noise {
    // Generate a time series waveform given a spectral amplitude
    // WireCell::Waveform::realseq_t generate_waveform(const std::vector<float>& spec, IRandom::pointer rng,
    //                                                 double replace = 0.02);

    // Generate specific noise spectrum.  Caller likely wants to Aux::inv() it and take Waveform::real().
    WireCell::Waveform::compseq_t generate_spectrum(const std::vector<float>& spec, IRandom::pointer rng,
                                                    double replace = 0.02);
}
