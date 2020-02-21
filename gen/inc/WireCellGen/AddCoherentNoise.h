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
            AddCoherentNoise(const std::string& rng="Random");

            virtual ~AddCoherentNoise();

            /// IFrameFilter
            virtual bool operator()(const input_pointer& inframe, output_pointer& outframe);

            /// IConfigurable
            virtual void configure(const WireCell::Configuration& config);
            virtual WireCell::Configuration default_configuration() const;

            void gen_elec_resp_default();

            size_t argclosest(std::vector<float> const& vec, float value);

            std::complex<double> multiply( std::complex<double> a, std::complex<double> b);

            std::vector<float> get_phase( std::vector<float> ampls, IRandom::pointer rng);

        private:
              IRandom::pointer m_rng;
              std::string m_rng_tn;
	            int m_nsamples;
              int m_fft_length;
              double m_period;
              double m_normalization;

              std::map< int, std::vector<float> > coherent_groups;
              std::map< int, std::vector<float> > phase_groups;

              Waveform::realseq_t m_elec_resp_freq;

              Log::logptr_t log;
	     };
    }
}

#endif
