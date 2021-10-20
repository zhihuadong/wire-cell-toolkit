#include "WireCellGen/FrameFanout.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellIface/SimpleFrame.h"

WIRECELL_FACTORY(FrameFanout, WireCell::Gen::FrameFanout,
                 WireCell::INamed,
                 WireCell::IFrameFanout, WireCell::IConfigurable)

using namespace WireCell;

Gen::FrameFanout::FrameFanout(size_t multiplicity)
    : Aux::Logger("FrameFanout", "glue")
    , m_multiplicity(multiplicity)
{
}
Gen::FrameFanout::~FrameFanout() {}

WireCell::Configuration Gen::FrameFanout::default_configuration() const
{
    Configuration cfg;
    // How many output ports
    cfg["multiplicity"] = (int) m_multiplicity;
    // If "tag_rules" is not given or empty then the frame is fanned
    // out in a trivial manner.
    // 
    // Tag rules are an array, one element per output port.  Each
    // element is an object keyed with "frame" or "trace".  Each of
    // their values are an object keyed by a regular expression
    // (regex) and with values that are a single tag or an array of
    // tags.
    //
    // The frame traces are forwarded in any case, tag filtering only
    // affects tags (and their trace indices in the case of trace
    // tags).
    //
    // Each frame tag found in the input is sent through the rules and
    // the resulting output tags are added to the set of frame tags of
    // the output.
    //
    // Trace tag work similarly.  Each trace tag is sent through the
    // rules and tags that the trace rule returns will be placed in
    // the output frame along with the trace index vector of the input
    // trace tag. 
    // 
    // See util/test_tagrules for examples.
    cfg["tag_rules"] = Json::arrayValue;
    return cfg;
}
void Gen::FrameFanout::configure(const WireCell::Configuration& cfg)
{
    int m = get<int>(cfg, "multiplicity", (int) m_multiplicity);
    if (m <= 0) {
        THROW(ValueError() << errmsg{"FrameFanout multiplicity must be positive"});
    }
    m_multiplicity = m;

    auto tr = cfg["tag_rules"];
    if (tr.isNull() or tr.empty()) {
        m_trivial = true;
        log->debug("trivial {}-way fanout", m_multiplicity);
        return;
    }
    m_trivial = false;
    m_ft.configure(tr);
    log->debug("rule based {}-way fanout", m_multiplicity);
}

std::vector<std::string> Gen::FrameFanout::output_types()
{
    const std::string tname = std::string(typeid(output_type).name());
    std::vector<std::string> ret(m_multiplicity, tname);
    return ret;
}

bool Gen::FrameFanout::operator()(const input_pointer& in, output_vector& outv)
{
    outv.resize(m_multiplicity);

    std::stringstream taginfo;
    taginfo << "#" << m_count << ": ";
    ++m_count;

    if (!in) {  //  pass on EOS
        for (size_t ind = 0; ind < m_multiplicity; ++ind) {
            outv[ind] = in;
        }
        taginfo << "see EOS";
        log->debug(taginfo.str());
        return true;
    }

    if (m_trivial) {
        for (size_t ind = 0; ind < m_multiplicity; ++ind) {
            outv[ind] = in;
        }
        taginfo << "frame tags: [";
        std::string comma="";
        for (auto tag : in->frame_tags()) {
            taginfo << comma << "\"" << tag << "\"";
            comma=",";
        }
        taginfo << "] traces: " << in->traces()->size() << " tagged: [";
        comma = "";
        for (auto tag : in->trace_tags()) {
            taginfo << comma << "\"" << tag << "\"";
            comma=",";
        }
        taginfo << "]";
        log->debug(taginfo.str());
        return true;
    }

    // O.w. we are rule driven.

    auto fintags = in->frame_tags();
    // Add empty tag to allow rules to create an output tag even if none exist.
    // No equivalent should be made for trace tags.
    fintags.push_back("");

    taginfo << "found input frame tags:[";
    std::string comma="";
    for (auto tag : fintags) {
        taginfo << comma << "\"" << tag << "\"";
        comma=",";
    }
    taginfo << "] traces: " << in->traces()->size() << " --> ";

    for (size_t ind = 0; ind < m_multiplicity; ++ind) {
        // Basic frame stays the same.
        auto sfout = new SimpleFrame(in->ident(), in->time(), *in->traces(), in->tick());

        // Transform any frame tags based on a per output port ruleset
        auto fouttags = m_ft.transform(ind, "frame", fintags);

        taginfo << " out#" << ind << ": frame tags:[";
        comma = "";
        for (auto ftag : fouttags) {
            sfout->tag_frame(ftag);
            taginfo << comma << "\"" << ftag << "\"";
            comma = ",";
        }
        taginfo << "], trace tags mapping: {";

        for (auto inttag : in->trace_tags()) {
            tagrules::tagset_t touttags = m_ft.transform(ind, "trace", inttag);
            if (touttags.empty()) {
                taginfo << " (\"" << inttag << "\") -> [skip]";
                continue;
            }
            const auto& traces = in->tagged_traces(inttag);
            const auto& summary = in->trace_summary(inttag);

            taginfo << " (\"" << inttag << "\") -> [";
            comma="";
            for (auto otag : touttags) {
                sfout->tag_traces(otag, traces, summary);
                taginfo << comma << "\"" << otag << "\"";
                comma=",";
            }
            taginfo << "]";
        };
        taginfo << " }";

        outv[ind] = IFrame::pointer(sfout);
    }

    log->debug(taginfo.str());

    return true;
}
