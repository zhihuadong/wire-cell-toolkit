#include "WireCellGen/WarmElecResponse.h"

#include "WireCellUtil/FFTBestLength.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Response.h"

#include <iostream>

WIRECELL_FACTORY(WarmElecResponse, WireCell::Gen::WarmElecResponse,
                 WireCell::IWaveform, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

Gen::WarmElecResponse::WarmElecResponse(int nticks, double t0, double gain, double shaping, double postgain, double tick)
{
    m_cfg["gain"] = gain;
    m_cfg["shaping"] = shaping;
    m_cfg["postgain"] = postgain;
    m_cfg["start"] = t0;
    m_cfg["tick"] = tick;
    m_cfg["nticks"] = nticks;
}
WireCell::Configuration Gen::WarmElecResponse::default_configuration() const
{
    return m_cfg;
}
void Gen::WarmElecResponse::configure(const WireCell::Configuration& cfg)
{
    m_cfg = cfg;

    Response::WarmElec we(m_cfg["gain"].asDouble(), m_cfg["shaping"].asDouble());
    const int nbins = m_cfg["nticks"].asInt();
    const double t0 = waveform_start();
    const double tick = waveform_period();

    // Choose the best bin-size for optimal performances
    const int m_fft_nbins = fft_best_length(nbins);

    Binning bins(nbins, t0, t0+  m_fft_nbins*tick);
    m_wave = we.generate(bins);
    Waveform::scale(m_wave, m_cfg["postgain"].asDouble());
}

double Gen::WarmElecResponse::waveform_start() const
{
    return m_cfg["start"].asDouble();
}

double Gen::WarmElecResponse::waveform_period() const
{
    return m_cfg["tick"].asDouble();
}

const IWaveform::sequence_type& Gen::WarmElecResponse::waveform_samples() const
{
    return m_wave;
}
