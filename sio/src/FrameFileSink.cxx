#include "WireCellSio/FrameFileSink.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellAux/FrameTools.h"

// These are found at *compile* time in util/inc/.
#include "custard/custard_boost.hpp"
#include "custard/custard_pigenc.hpp"

WIRECELL_FACTORY(FrameFileSink, WireCell::Sio::FrameFileSink,
                 WireCell::IFrameSink, WireCell::IConfigurable)

using namespace WireCell;

Sio::FrameFileSink::FrameFileSink()
  : log(Log::logger("io"))
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

    // Select which traces to consider.
    cfg["tags"] = Json::arrayValue;

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

void Sio::FrameFileSink::one_tag(const IFrame::pointer& frame,
                                 const std::string& tag)
{
    ITrace::vector traces;
    if (tag == "*") {
        // The designer of IFrame was a dumb dumb in chosing to return
        // a shared vector.  He is also to blame for writing this
        // comment.
        auto sv = frame->traces();
        traces.insert(traces.begin(), sv->begin(), sv->end());
        log->debug("FrameFileSink: all traces [{}], frame: {}",
                   traces.size(), frame->ident());
    }
    else {
        traces = Aux::tagged_traces(frame, tag);
        log->debug("FrameFileSink: tag: {} traces [{}], frame: {}",
                   tag, traces.size(), frame->ident());
    }
    if (traces.empty()) {
        log->warn("FrameFileSink: tag: {}, frame: {}.  ZERO TRACES",
                   tag, frame->ident());
        return;
    }

    auto channels = Aux::channels(traces);
    std::sort(channels.begin(), channels.end());
    auto chbeg = channels.begin();
    auto chend = std::unique(chbeg, channels.end());
    auto tbinmm = Aux::tbin_range(traces);

    const size_t ncols = tbinmm.second - tbinmm.first;
    const size_t nrows = std::distance(chbeg, chend);
    log->debug("FrameFileSink: saving ncols={} nrows={}", ncols, nrows);

    Array::array_xxf arr = Array::array_xxf::Zero(nrows, ncols) + m_baseline;
    Aux::fill(arr, traces, channels.begin(), chend, tbinmm.first);
    arr = arr * m_scale + m_offset;

    {  // the 2D frame array
        const std::string aname = String::format("frame_%s_%d.npy", tag.c_str(), frame->ident());
        if (m_digitize) {
            Array::array_xxs sarr = arr.cast<short>();
            custard::eigen_sink(m_out, aname, sarr);
        }
        else {
            custard::eigen_sink(m_out, aname, arr);
        }
        log->debug("FrameFileSink: saved {} with {} channels {} ticks @t={} ms qtot={}", aname, nrows, ncols,
                   frame->time() / units::ms, arr.sum());
    }

    {  // the channel array
        const std::string aname = String::format("channels_%s_%d", tag.c_str(), frame->ident());
        custard::vector_sink(m_out, aname, channels);
    }

    {  // the tick array
        const std::string aname = String::format("tickinfo_%s_%d", tag.c_str(), frame->ident());
        const std::vector<double> tickinfo{frame->time(), frame->tick(), (double) tbinmm.first};
        custard::vector_sink(m_out, aname, tickinfo);
    }

}
bool Sio::FrameFileSink::operator()(const IFrame::pointer& frame)
{
    if (! frame) { // eos
        log->debug("FrameFileSink: EOS, closing {}", m_outname);
        m_out.pop();
        return true;
    }

    for (auto tag : m_tags) {
        one_tag(frame, tag);
    }
    return true;
}
