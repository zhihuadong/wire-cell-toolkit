#include "WireCellSio/FrameFileSink.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/Stream.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellAux/FrameTools.h"


WIRECELL_FACTORY(FrameFileSink, WireCell::Sio::FrameFileSink,
                 WireCell::INamed,
                 WireCell::IFrameSink, WireCell::ITerminal,
                 WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Stream;

Sio::FrameFileSink::FrameFileSink()
    : Aux::Logger("FrameFileSink", "io")
{
}

Sio::FrameFileSink::~FrameFileSink()
{
}

void Sio::FrameFileSink::finalize()
{
    log->debug("closing {} after {} calls",
               m_outname, m_count);
    m_out.pop();
}

WireCell::Configuration Sio::FrameFileSink::default_configuration() const
{
    Configuration cfg;
    // Output tar file name.
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
    output_filters(m_out, m_outname);
    if (m_out.size() < 2) {     // must have at least get tar filter + file sink.
        THROW(ValueError() << errmsg{"FrameFileSink: unsupported outname: " + m_outname});
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
    }
    else {
        traces = Aux::tagged_traces(frame, tag);
    }
    if (traces.empty()) {
        log->warn("call={} frame={} ntraces={} tag=\"{}\" zero traces",
                   m_count, frame->ident(),traces.size(), tag);
        return;
    }
    log->debug("call={} frame={} ntraces={} tag=\"{}\"",
               m_count, frame->ident(),traces.size(), tag);


    auto channels = Aux::channels(traces);
    std::sort(channels.begin(), channels.end());
    auto chbeg = channels.begin();
    auto chend = std::unique(chbeg, channels.end());
    auto tbinmm = Aux::tbin_range(traces);

    const size_t ncols = tbinmm.second - tbinmm.first;
    const size_t nrows = std::distance(chbeg, chend);

    Array::array_xxf arr = Array::array_xxf::Zero(nrows, ncols) + m_baseline;
    Aux::fill(arr, traces, channels.begin(), chend, tbinmm.first);
    arr = arr * m_scale + m_offset;

    {  // the 2D frame array
        const std::string aname = String::format("frame_%s_%d.npy", tag.c_str(), frame->ident());
        if (m_digitize) {
            Array::array_xxs sarr = arr.cast<short>();
            write(m_out, aname, sarr);
        }
        else {
            write(m_out, aname, arr);
        }
    }

    {  // the channel array
        const std::string aname = String::format("channels_%s_%d.npy", tag.c_str(), frame->ident());
        write(m_out, aname, channels);
    }

    {  // the tick array
        const std::string aname = String::format("tickinfo_%s_%d.npy", tag.c_str(), frame->ident());
        const std::vector<double> tickinfo{frame->time(), frame->tick(), (double) tbinmm.first};
        write(m_out, aname, tickinfo);
    }
    m_out.flush();

}
bool Sio::FrameFileSink::operator()(const IFrame::pointer& frame)
{
    if (! frame) { // eos
        log->debug("EOS at call={}", m_count);
        ++m_count;
        return true;
    }

    for (auto tag : m_tags) {
        one_tag(frame, tag);
    }
    ++m_count;
    return true;
}
