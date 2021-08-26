#include "WireCellSio/FrameFileSource.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/Stream.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include "WireCellAux/FrameTools.h"

#include <tuple>

WIRECELL_FACTORY(FrameFileSource, WireCell::Sio::FrameFileSource,
                 WireCell::INamed,
                 WireCell::IFrameSource, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::Stream;
using namespace WireCell::String;

Sio::FrameFileSource::FrameFileSource()
    : Aux::Logger("FrameFileSource", "io")
{
}

Sio::FrameFileSource::~FrameFileSource()
{
}

WireCell::Configuration Sio::FrameFileSource::default_configuration() const
{
    Configuration cfg;
    // Input tar stream
    cfg["inname"] = m_inname;

    // Set of traces to consider.
    cfg["tags"] = Json::arrayValue;

    return cfg;
}

void Sio::FrameFileSource::configure(const WireCell::Configuration& cfg)
{
    m_inname = get(cfg, "inname", m_inname);

    m_in.clear();
    input_filters(m_in, m_inname);
    if (m_in.size() < 2) {     // must have at least get tar filter + file source.
        THROW(ValueError() << errmsg{"FrameFielSource: unsupported inname: " + m_inname});
    }

    m_tags.clear();
    for (auto jtag : cfg["tags"]) {
        m_tags.push_back(jtag.asString());
    }
    if (m_tags.empty()) {
        m_tags.push_back("*");
    }
}

bool Sio::FrameFileSource::matches(const std::string& tag)
{
    for (const auto& maybe : m_tags) {
        if (maybe == "*") {
            return true;
        }
        if (maybe == tag) {
            return true;
        }
    }
    return false;    
}

// ident, type, tag, ext
using filemd_t = std::tuple<int, std::string, std::string, std::string>;

static
filemd_t parse_filename(const std::string& fname)
{
    auto parts = split(fname, "_");
    auto rparts = split(parts[2], ".");
    int ident = std::atoi(rparts[0].c_str());
    return std::make_tuple(ident, parts[0], parts[1], rparts[1]);
}

static
filemd_t read_pig(std::istream& si, pigenc::File& pig)
{
    std::string fname{""};
    size_t fsize{0};
    custard::read(si, fname, fsize);
    pig.read(si);
    return parse_filename(fname);
}

// Read one file set from tar stream and save to fraem if tag matches
IFrame::pointer Sio::FrameFileSource::next()
{
    // Suffer full eager loading
    pigenc::File fpig, cpig, tpig;
    auto fmd = read_pig(m_in, fpig);
    auto cmd = read_pig(m_in, cpig);
    auto tmd = read_pig(m_in, tpig);

    if (!m_in) {
        log->error("call={}, frame read fail with file={}", m_count, m_inname);
        return nullptr;
    }

    // fixme: here we assume f,c,t are properly lock-step ordered and
    // just check frame.
    const std::string havetag = std::get<1>(fmd);
    if (! matches(havetag)) {
        log->debug("call={}, skipping tag=\"{}\" file={}",
                   m_count, havetag, m_inname);
        return next();
    }

    const int ident = std::get<0>(fmd);

    std::vector<int> channels;
    bool ok = pigenc::stl::load(cpig, channels);
    if (!ok) {
        return nullptr;
    }

    std::vector<double> tickinfo;
    ok = pigenc::stl::load(tpig, tickinfo);
    if (!ok) {
        return nullptr;
    }
    // time, tick, min tbin0 as doubles
    double time = tickinfo[0];
    double tick = tickinfo[1];
    int tbin0 = (int)tickinfo[2];

    using array_xxfrw = Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    array_xxfrw arr;
    if (fpig.header().dtype() == "<i2") { // ADC short ints
        Array::array_xxs sarr;
        ok = pigenc::eigen::load(fpig, sarr);
        if (!ok) {
            return nullptr;
        }
        arr = sarr.cast<float>();
    }
    else {
        ok = pigenc::eigen::load(fpig, arr);
        if (!ok) {
            return nullptr;
        }
    }        

    ITrace::vector all_traces;
    IFrame::trace_list_t tagged_traces;

    for (long irow=0; irow < arr.rows(); ++irow) {
        int chid = channels[irow];
        auto row = arr.row(irow);

        ITrace::ChargeSequence charges(row.data(), row.data() + arr.cols());

        auto itrace = std::make_shared<SimpleTrace>(chid, tbin0, charges);
        if (! havetag.empty()) {
            tagged_traces.push_back(all_traces.size());
        }
        all_traces.push_back(itrace);
    }

    auto sframe = new SimpleFrame(ident, time, all_traces, tick);
    if (! havetag.empty()) {
        sframe->tag_traces(havetag, tagged_traces);
    }
    return IFrame::pointer{sframe};
}

bool Sio::FrameFileSource::operator()(IFrame::pointer& frame)
{
    frame = nullptr;
    if (m_eos_sent) {
        return false;
    }
    frame = next();
    if (!frame) {
        m_eos_sent = true;
    }
    return true;
}
