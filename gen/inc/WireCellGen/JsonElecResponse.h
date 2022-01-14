/** An ElecResponse waveform is that due to sending a unit charge
 * pulse into an amplifier with gain and shaping and a particular
 * response function electronics. This JsonElecResponse defines a
 * response function from a json input. */

#ifndef WIRECELL_GEN_JSONELECRESPONSE
#define WIRECELL_GEN_JSONELECRESPONSE

#include "WireCellIface/IWaveform.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Response.h"

namespace WireCell {
    namespace Gen {

        class JsonElecResponse : public IWaveform, public IConfigurable {
           public:
            JsonElecResponse(int nticks = 10000, double t0 = 0,
                             double postgain = 1.0, double tick = 0.5 * units::us,
                             std::string filename="");
            // IConfigurable interface
            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            // The starting point of the sampling
            virtual double waveform_start() const;
            // The sampling period aka bin width
            virtual double waveform_period() const;
            // The collection of samples
            virtual const sequence_type& waveform_samples() const;
            // The collection of sample rebinned
            virtual sequence_type waveform_samples(const WireCell::Binning& tbins) const;

           private:
            Configuration m_cfg;
            mutable sequence_type m_wave;
            Waveform::realseq_t m_times;
            Waveform::realseq_t m_amps;
        };
    }  // namespace Gen
}  // namespace WireCell

#endif
