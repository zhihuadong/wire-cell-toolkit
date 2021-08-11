#include "WireCellSio/FrameFileSink.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellAux/FrameTools.h"

#include "custard/boost_custard.hpp"
#include "custard/pigenc.hpp"

WIRECELL_FACTORY(FrameFileSink, WireCell::Sio::FrameFileSink,
                 WireCell::IFrameSink, WireCell::IConfigurable)

using namespace WireCell;

Sio::FrameFileSink::FrameFileSink()
  : m_drift_speed(1.6 * units::mm / units::us)
  , log(Log::logger("io"))
{
}

Sio::FrameFileSink::~FrameFileSink()
{
}

WireCell::Configuration Sio::FrameFileSink::default_configuration() const
{
    Configuration cfg;
    // output json file.  A "%d" type format code may be included to be resolved by a frame identifier.
    cfg["outname"] = m_outname;

    cfg["tags"] = Json::arrayvalue;

    // If digitize is true, then samples as 16 bit ints.  Otherwise
    // save as 32 bit floats.
    cfg["digitize"] = false;

    // This number is set to the waveform sample array before any
    // charge is added.
    cfg["baseline"] = 0.0;

    // This number will be multiplied to each waveform sample before
    // casting to dtype.
    cfg["scale"] = 1.0;

    // This number will be added to each scaled waveform sample before
    // casting to dtype.
    cfg["offset"] = 0.0;
    return cfg;
}

void Sio::FrameFileSink::configure(const WireCell::Configuration& cfg)
{
    m_outname = get(cfg, "outname", m_outname);

    m_out.clear();
    custard::output_filters(m_out, m_outname);
    if (m_out.empty()) {
        THROW(ValueError() << errmsg{"FrameFielSink: unsupported outname: " + m_outname});
    }

    m_tags.clear();
    for (auto jtag : cfg["tags"]) {
        m_tags.push_back(jtag.asString());
    }
    if (m_tags.empty()) {
        m_tags.push_back("*");
    }

    m_digitize = get<bool>(cfg, "digitize", m_digitize);
    m_baseline = get(cfg, "baseline", m_baseline);
    m_scale = get(cfg, "scale", m_scale);
    m_offset = get(cfg, "offset", m_offset);
}

bool Sio::FrameFileSink::one_tag(const IFrame::pointer& frame,
                                 const std::string& tag)
{
    ITrace::shared_vector traces
    if (tag == "*") {
        traces = frame->traces();
    }
    else {
        traces = Aux::tagged_traces(frame, tag);
    }

    auto channels = Aux::channels(traces);
    std::sort(channels.begin(), channels.end());
    auto chbeg = channels.begin();
    auto chend = std::unique(chbeg, channels.end());
    auto tbinmm = Aux::tbin_range(traces);

    const size_t ncols = tbinmm.second - tbinmm.first;
    const size_t nrows = std::distance(chbeg, chend);
    l->debug("FrameFileSink: saving ncols={} nrows={}", ncols, nrows);

    Array::array_xxf arr = Array::array_xxf::Zero(nrows, ncols) + m_baseline;
    Aux::fill(arr, traces, channels.begin(), chend, tbinmm.first);
    arr = arr * m_scale + m_offset;

    {  // the 2D frame array
        const std::string aname = String::format("frame_%s_%d.npy", tag.c_str(), m_save_count);
        if (digitize) {
            Array::array_xxs sarr = arr.cast<short>();
            // const short* sdata = sarr.data();
            // cnpy::npz_save(fname, aname, sdata, {ncols, nrows}, mode);
            save2d(sarr, aname, fname, mode);
        }
        else {
            // cnpy::npz_save(fname, aname, arr.data(), {ncols, nrows}, mode);
            save2d(arr, aname, fname, mode);
        }
        l->debug("NumpyFrameSaver: saved {} with {} channels {} ticks @t={} ms qtot={}", aname, nrows, ncols,
                 inframe->time() / units::ms, arr.sum());
    }

    {  // the channel array
        const std::string aname = String::format("channels_%s_%d", tag.c_str(), m_save_count);
        cnpy::npz_save(fname, aname, channels.data(), {nrows}, mode);
            
    }

    {  // the tick array
        const std::string aname = String::format("tickinfo_%s_%d", tag.c_str(), m_save_count);
        const std::vector<double> tickinfo{inframe->time(), inframe->tick(), (double) tbinmm.first};
        cnpy::npz_save(fname, aname, tickinfo.data(), {3}, mode);
    }



    void raster(WireCell::Array::array_xxf& block, WireCell::ITrace::vector traces,
                    const std::vector<int>& channels);
}
bool Sio::FrameFileSink::operator()(const IFrame::pointer& frame)
{
    for (auto tag : m_tags) {
        one_tag(frame, tag);
    }

    auto top = 
    std::stringstream topss;
    topss << top;
    auto tops = topss.str();
    std::stringstream sizess;
    sizess << tops.size();
    auto sizes = sizess.str();
    auto cname = Aux::name(cluster);
    m_out << cname << "\n" << sizes << "\n" << tops;

    if (m_output_frame) {
        log->warn("ClusterFileSink: frame output not yet implemented");
    }
    return true;
}
