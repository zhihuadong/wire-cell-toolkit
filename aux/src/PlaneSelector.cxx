#include "WireCellAux/PlaneSelector.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellAux/FrameTools.h"
#include "WireCellAux/PlaneTools.h"

#include "WireCellUtil/NamedFactory.h"

#include <sstream>

WIRECELL_FACTORY(PlaneSelector, WireCell::Aux::PlaneSelector,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;

Aux::PlaneSelector::PlaneSelector()
    : Aux::Logger("PlaneSelector", "glue")
{
}

Aux::PlaneSelector::~PlaneSelector() {}

WireCell::Configuration Aux::PlaneSelector::default_configuration() const
{
    Configuration cfg;

    /// The anode to match
    cfg["anode"] = "AnodePlane";

    /// The plane number.  Currently this is interpreted as the plane
    /// index in [0,1,2].  For 2-faced anodes, this will select
    /// channels with this plane index from both faces in face-minor
    /// order (first face 0 channels then face 1 channels).
    cfg["plane"] = 0;

    // A future extension may add a "method" config parameter which is
    // used to interpret "plane" number against "ident", "ilayer" or
    // "index" (which "index" being the default and current
    // interpretation).

    /// Only traces with these tags will be considered.  If empty then
    /// the frame as a whole is considered.
    cfg["tags"] = Json::arrayValue;

    /// Rules to govern the output frame and trace tags based on input
    /// frame and trace tags.  There is no map from input frame or
    /// trace tags then it will be kept for the output frame or trace
    /// tag.
    cfg["tag_rules"] = Json::arrayValue;

    return cfg;
}

void Aux::PlaneSelector::configure(const WireCell::Configuration& cfg)
{
    int wire_plane_index = get(cfg, "plane", 0);
    
    // We only need the anode temporarily to build set of channel IDs
    // that we care about.
    std::string anode_tn = get<std::string>(cfg, "anode", "AnodePlane");
    auto anode = Factory::find_tn<IAnodePlane>(anode_tn);
    auto chans = Aux::plane_channels(anode, wire_plane_index);
    m_chids.clear();
    for (const auto& ichan : chans) {
        m_chids.insert(ichan->ident());
    }
    
    auto jtags = cfg["tags"];
    m_tags.clear();
    for (auto jtag : cfg["tags"]) {
        m_tags.push_back(jtag.asString());
    }

    auto tr = cfg["tag_rules"];
    m_ft.configure(tr);
}


bool Aux::PlaneSelector::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        log->debug("see EOS at call={}", m_count);
        ++m_count;
        return true;  // eos
    }

    ITrace::vector out_traces;

    const auto& in_traces = in->traces();
    const size_t nin = in_traces->size();

    // Fill (smaller) output trace vector and map input to output
    // trace indices.
    std::unordered_map<size_t, size_t> in2out;
    for (size_t in_ind = 0; in_ind < nin; ++in_ind) {
        const auto& in_trace = in_traces->at(in_ind);
        int chid = in_trace->channel();
        if (m_chids.find(chid) == m_chids.end()) {
            continue;
        }
        in2out[in_ind] = out_traces.size();
        out_traces.push_back(in_trace);
    }
    
    // basic output frame ready
    auto sf = new SimpleFrame(in->ident(), in->time(), out_traces, in->tick());

    // tagged traces need to be filtered down with index translation
    // to include only those traces in the smaller output frame.
    for (const auto& tag : m_tags) {
        IFrame::trace_list_t keep;
        for (const auto& ind : in->tagged_traces(tag)) {
            int chid = in_traces->at(ind)->channel();
            auto got = in2out.find(chid);
            if (got == in2out.end()) {
                continue;
            }
            keep.push_back(got->second);
            // fixme: we are ignoring possible summaries....
        }

        if (keep.empty()) {
            log->debug("call={} no traces remain for {}", m_count, tag);
            continue;
        }

        auto new_tags = m_ft.transform(0, "trace", {tag});
        if (new_tags.empty()) {
            new_tags.insert(tag);
        }
        for (auto new_tag : new_tags) {
            sf->tag_traces(new_tag, keep);
        }
    }

    std::vector<std::string> frame_tags = in->frame_tags();
    if (frame_tags.empty()) { frame_tags.push_back(""); }
    for (auto new_tag : m_ft.transform(0, "frame", frame_tags)) {    
        sf->tag_frame(new_tag);
    }

    out = IFrame::pointer(sf);

    std::stringstream info;
    info << "input " << Aux::taginfo(in) << " output: " << Aux::taginfo(out);
    log->debug(info.str());

    return true;
}
