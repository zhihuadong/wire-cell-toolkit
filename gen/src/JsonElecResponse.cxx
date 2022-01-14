#include "WireCellGen/JsonElecResponse.h"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Response.h"
#include "WireCellUtil/Units.h"

#include <iostream>
#include <algorithm>

WIRECELL_FACTORY(JsonElecResponse, WireCell::Gen::JsonElecResponse, WireCell::IWaveform, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

// Generate waveform given a binning and a tablulated function
WireCell::Waveform::realseq_t generate(const WireCell::Binning& tbins,
    const Waveform::realseq_t& times, const Waveform::realseq_t& amps)
{
    const int nsamples = tbins.nbins();
    WireCell::Waveform::realseq_t ret(nsamples, 0.0);
    for (int ind = 0; ind < nsamples; ++ind) {
        const double time = tbins.center(ind);
        if (time < times.front() or time > times.back())
            ret.at(ind) = 0;
        else { // find the element that is greater or equal
            auto it = std::lower_bound(times.begin(), times.end(), time);
            auto k = it - times.begin();
            if (*it == time) ret.at(ind) = amps.at(k);
            else { // interpolation
                ret.at(ind) = amps.at(k-1) + (amps.at(k)-amps.at(k-1))/(times.at(k)-times.at(k-1))*(time - times.at(k-1));
            }
        }
    }
    return ret;
}

Gen::JsonElecResponse::JsonElecResponse(int nticks,  double t0, double postgain,
                                        double tick, std::string filename)
{
    m_cfg["filename"] = filename;
    m_cfg["postgain"] = postgain;
    m_cfg["start"] = t0;
    m_cfg["tick"] = tick;
    m_cfg["nticks"] = nticks;
}
WireCell::Configuration Gen::JsonElecResponse::default_configuration() const { return m_cfg; }
void Gen::JsonElecResponse::configure(const WireCell::Configuration& cfg)
{
    m_cfg = cfg;
    auto filename = m_cfg["filename"].asString();
    if (filename.empty()) {
        THROW(ValueError() << errmsg{"must supply a JsonElecResponse filename"});
    }

    auto top = Persist::load(filename);
    auto jtimes = top["times"];
    auto jamps = top["amplitudes"];
    if (jtimes.size() != jamps.size()) {
        THROW(ValueError() << errmsg{"inconsistent dimensions in file " + filename});
    }
    const int nsamp = jtimes.size();
    m_times.resize(nsamp);
    m_amps.resize(nsamp);

    if (jtimes.isNull()) {
        THROW(ValueError() << errmsg{"no values given in file " + filename});
    }
    const double tick = waveform_period();
    for (int i=0; i<nsamp; i++) {
        m_times.at(i) = jtimes[i].asFloat();
        m_amps.at(i) = jamps[i].asFloat();
    }

    const int nbins = m_cfg["nticks"].asInt();
    const double t0 = waveform_start();
    Binning bins(nbins, t0, t0 + nbins * tick);
    m_wave = generate(bins, m_times, m_amps);
    Waveform::scale(m_wave, m_cfg["postgain"].asDouble());
}

double Gen::JsonElecResponse::waveform_start() const { return m_cfg["start"].asDouble(); }

double Gen::JsonElecResponse::waveform_period() const { return m_cfg["tick"].asDouble(); }

const IWaveform::sequence_type& Gen::JsonElecResponse::waveform_samples() const { return m_wave; }

IWaveform::sequence_type Gen::JsonElecResponse::waveform_samples(const WireCell::Binning& tbins) const
{
    sequence_type rebinned_wave = generate(tbins, m_times, m_amps);
    Waveform::scale(rebinned_wave, m_cfg["postgain"].asDouble());
    return rebinned_wave;
}

