#include "WireCellAux/TaggedTensorSetFrame.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellUtil/NamedFactory.h"


WIRECELL_FACTORY(TaggedTensorSetFrame, WireCell::Aux::TaggedTensorSetFrame,
                 WireCell::ITensorSetFrame, WireCell::IConfigurable)

using namespace WireCell;

WireCell::Configuration Aux::TaggedTensorSetFrame::default_configuration() const
{
    // Each tag will look up a tensor.
    Configuration cfg;
    cfg["tensors"] = Json::arrayValue;
    return cfg;
}

void Aux::TaggedTensorSetFrame::configure(const WireCell::Configuration& config)
{
    m_tags.clear();
    for (auto jten : config["tensors"]) {
        // no tag means empty tag
        m_tags.insert(get<std::string>(jten, "tag", ""));
    }
}

bool Aux::TaggedTensorSetFrame::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {                  // pass on EOS
        return true;
    }
    
    auto traces = new std::vector<ITrace::pointer>;

    auto md = in->metadata();
    auto jtens = md["tensors"];

    size_t ntens = jtens.size();
    for (size_t iten=0; iten<ntens; ++iten) {

        auto jten = jtens[(int)iten];
        std::string tag = get<std::string>(jten, "tag", "");
        if (m_tags.find(tag) == m_tags.end()) {
            continue;           // we don't want this one
        }

        size_t iwf = jten["waveform"].asInt();
        auto wf_ten = in->tensors()->at(iwf);
        auto wf_shape = wf_ten->shape();
        size_t nchans = wf_shape[0];
        size_t nticks = wf_shape[1];
        int tbin = get<int>(jten, "tbin", 0);

        Eigen::Map<const Eigen::ArrayXXf> wf_arr((const float*)wf_ten->data(), nchans, nticks);
        
        size_t ich = jten["channels"].asInt();
        auto ch_ten = in->tensors()->at(ich);
        Eigen::Map<const Eigen::ArrayXi> ch_arr((const int*)ch_ten->data(), nchans);

        for (size_t ind=0; ind<nchans; ++ind) {
            SimpleTrace* st = new SimpleTrace(ch_arr[ind], tbin, nticks);
            auto& q = st->charge();
            for (size_t itick=0; itick<nticks; ++itick) {
                q[itick] = wf_arr(ind,itick);
            }
            traces->push_back(ITrace::pointer(st));
        }
    }

    SimpleFrame* frame = 
        new SimpleFrame(get(md, "ident", 0),
                        get(md, "time", 0.0),
                        ITrace::shared_vector(traces),
                        get(md, "tick", 0.5*units::microsecond));

    // Do second pass for tagged indices and summary...
    for (size_t iten=0; iten<ntens; ++iten) {

        auto jten = jtens[(int)iten];
        std::string tag = get<std::string>(jten, "tag", "");
        if (m_tags.find(tag) == m_tags.end()) {
            continue;           // we don't want this one
        }

        size_t ich = jten["channels"].asInt();
        auto ch_ten = in->tensors()->at(ich);
        size_t nchans = ch_ten->shape()[0];

        const size_t index_start = traces->size();
        IFrame::trace_list_t indices;
        for (size_t ind=0; ind<nchans; ++ind) {
            indices.push_back(index_start + ind);
        }

        IFrame::trace_summary_t summary;
        auto jsum = jten["summary"];
        if (! jsum.isNull()) {
            size_t isum = jsum.asInt();
            auto sum_ten = in->tensors()->at(isum);
            Eigen::Map<const Eigen::ArrayXd> sum_arr((const double*)sum_ten->data(), nchans);
            summary.resize(nchans);
            for (size_t ind=0; ind<nchans; ++ind) {
                summary[ind] = sum_arr(ind);
            }
        }
        frame->tag_traces(tag, indices, summary);
    }

    out = IFrame::pointer(frame);
    return true;
}
        
Aux::TaggedTensorSetFrame::TaggedTensorSetFrame()
{
}
Aux::TaggedTensorSetFrame::~TaggedTensorSetFrame()
{
}

