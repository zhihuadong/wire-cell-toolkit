// Add coherent noise to the waveform
// mailto:ascarpell@bnl.gov

#ifndef WIRECELL_GEN_ADDCOHERENTNOISE
#define WIRECELL_GEN_ADDCOHERENTNOISE

#include "WireCellIface/IFrameFilter.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IRandom.h"
#include "WireCellIface/IChannelSpectrum.h"
#include "WireCellUtil/Waveform.h"
#include "WireCellUtil/Logging.h"

#include <string>

namespace WireCell {
    namespace Gen {

        class AddCoherentNoise : public IFrameFilter, public IConfigurable {
        public:
            AddCoherentNoise();

            virtual ~AddCoherentNoise();

            /// IFrameFilter
            virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);

            /// IConfigurable
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

        private:
	          int m_nsamples;

            Log::logptr_t log;
	     };
    }
}

#endif
