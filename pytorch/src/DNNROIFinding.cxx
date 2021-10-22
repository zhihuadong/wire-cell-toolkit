#include "WireCellPytorch/DNNROIFinding.h"
#include "WireCellPytorch/Util.h"
#include "WireCellIface/ITrace.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellAux/FrameTools.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Array.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/TimeKeeper.h"

#include <string>
#include <vector>

// Uncomment to enable local writing of HDF5 file for debugging
// purposes.  A temporary HDF5 dependency must be added in
// wscript_build.  Do not commit with this.

// #define DNNROI_HDF5_DEBUG

// FIXME: consider changing this to instead write out numpy.tar.bz2
// streams which does not require HDF5 dependency and could then
// become a dynamic option.

#ifdef DNNROI_HDF5_DEBUG
#include <h5cpp/all>
#endif

/// macro to register name - concrete pair in the NamedFactory
/// @param NAME - used to configure node in JSON/Jsonnet
/// @parame CONCRETE - C++ concrete type
/// @parame ... - interfaces
WIRECELL_FACTORY(DNNROIFinding, WireCell::Pytorch::DNNROIFinding, WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;

Pytorch::DNNROIFinding::DNNROIFinding()
    : Aux::Logger("DNNROIFinding", "torch")
    , m_save_count(0)
{
}

Pytorch::DNNROIFinding::~DNNROIFinding() {}

void Pytorch::DNNROIFinding::configure(const WireCell::Configuration& cfg)
{
    m_cfg = cfg;

    if (m_cfg["intags"].size() == 0) {
        log->critical("No intags provided!");
        THROW(ValueError() << errmsg{"No intags provided!"});
    }

    auto anode_tn = cfg["anode"].asString();
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);

#ifdef DNNROI_HDF5_DEBUG
    {
        std::string fn = cfg["evalfile"].asString();
        if (fn.empty()) {
            THROW(ValueError() << errmsg{"Must provide output evalfile to DNNROIFinding"});
        }
        h5::create(fn, H5F_ACC_TRUNC);
    }
#endif

    auto torch_tn = cfg["torch_script"].asString();
    
    m_torch_script = Factory::find_maybe_tn<ITensorSetFilter>(torch_tn);
    if (!m_torch_script) {
        m_torch_service = Factory::find_tn<ITensorForward>(torch_tn);
    }
}

WireCell::Configuration Pytorch::DNNROIFinding::default_configuration() const
{
    Configuration cfg;

    // anode
    cfg["anode"] = "AnodePlane";

    // DNN needs consistent scaling with trained model
    cfg["scale"] = 1.0 / 4000;

    // offset for frame to eigen for DNN input
    cfg["offset"] = 0.0;

    // Caution: channel counts here are relative to the first channel
    // in the anode.  They are not detector-wide absolute channel IDs.
    // FIXME: and this selection will totally break on any detector
    // with non-contigous channel IDs w/in a plane.  Better to change
    // this configuration to be a plane identifier.
    cfg["cbeg"] = 800;
    cfg["cend"] = 1600;

    cfg["tick0"] = 0;
    cfg["nticks"] = 6000;

    cfg["charge_thresh"] = 0.7;

    // TorchScript model
    cfg["torch_script"] = "TorchScript:dnn_roi";

    // taces used as input
    cfg["intags"] = Json::arrayValue;

    // tick/slice
    cfg["tick_per_slice"] = 10;

    // decon charge to fill in ROIs
    cfg["decon_charge_tag"] = "decon_charge0";

    // evaluation output
    cfg["evalfile"] = "tsmodel-eval.h5";

    // output trace tag
    cfg["outtag"] = "dnn_sp";

    return cfg;
}

namespace {

    // used in sparsifying below.  Could use C++17 lambdas....
    bool ispositive(float x) { return x > 0.0; }
    bool isZero(float x) { return x == 0.0; }

    void eigen_to_traces(const Array::array_xxf& data, ITrace::vector& itraces, IFrame::trace_list_t& indices,
                         const int cbeg, const int nticks, const bool sparse = false)
    {
        // reuse this temporary vector to hold charge for a channel.
        ITrace::ChargeSequence charge(nticks, 0.0);

        for (int ch = cbeg; ch < cbeg + data.cols(); ++ch) {
            for (int itick = 0; itick != nticks; itick++) {
                const float q = data(itick, ch - cbeg);
                charge.at(itick) = q;
            }

            // actually save out
            if (sparse) {
                // Save waveform sparsely by finding contiguous, positive samples.
                std::vector<float>::const_iterator beg = charge.begin(), end = charge.end();
                auto i1 = std::find_if(beg, end, ispositive);  // first start
                while (i1 != end) {
                    // stop at next zero or end and make little temp vector
                    auto i2 = std::find_if(i1, end, isZero);
                    const std::vector<float> q(i1, i2);

                    // save out
                    const int tbin = i1 - beg;
                    SimpleTrace* trace = new SimpleTrace(ch, tbin, q);
                    const size_t trace_index = itraces.size();
                    indices.push_back(trace_index);
                    itraces.push_back(ITrace::pointer(trace));

                    // find start for next loop
                    i1 = std::find_if(i2, end, ispositive);
                }
            }
            else {
                // Save the waveform densely, including zeros.
                SimpleTrace* trace = new SimpleTrace(ch, 0, charge);
                const size_t trace_index = itraces.size();
                indices.push_back(trace_index);
                itraces.push_back(ITrace::pointer(trace));
            }
        }
    }

    Array::array_xxf frame_to_eigen(const ITrace::vector& traces,
                                    //const IFrame::pointer& inframe, const std::string& tag,
                                    const IAnodePlane::pointer anode, const float scale = 1.0, const float offset = 0,
                                    const int win_cbeg = 0, const int win_cend = 800, const int tick0 = 0,
                                    const int nticks = 6000)
    {
        auto channels = anode->channels();
        const int cbeg = channels.front() + win_cbeg;
        const int cend = channels.front() + win_cend - 1;

        const size_t ncols = nticks;
        const size_t nrows = cend - cbeg + 1;
        Array::array_xxf arr = Array::array_xxf::Zero(nrows, ncols);

        // auto traces = Aux::tagged_traces(inframe, tag);
        if (traces.empty()) {
            // std::cout << "[yuhw] frame " << inframe->frame_tags() << " has 0 " << tag << " traces!\n";
            return arr;
        }

        Aux::fill(arr, traces, channels.begin() + win_cbeg, channels.begin() + win_cend, tick0);
        arr = arr * scale + offset;

        return arr;
    }
}  // namespace

ITensorSet::pointer Pytorch::DNNROIFinding::forward(const ITensorSet::pointer& in)
{
    if (m_torch_service) {
        log->debug("call={} forward service", m_save_count);
        return m_torch_service->forward(in);
    }
    ITensorSet::pointer out{nullptr};
    log->debug("call={} forward script", m_save_count);
    (*m_torch_script)(in, out);
    return out;
}

bool Pytorch::DNNROIFinding::operator()(const IFrame::pointer& inframe, IFrame::pointer& outframe)
{
    outframe = inframe;
    if (!inframe) {
        log->debug("EOS at call={}", m_save_count);
        outframe = nullptr;
        return true;
    }

    TimeKeeper tk(fmt::format("call={}", m_save_count));

    const int cbeg = m_cfg["cbeg"].asInt();
    const int cend = m_cfg["cend"].asInt();
    const int tick0 = m_cfg["tick0"].asInt();
    const int nticks = m_cfg["nticks"].asInt();

    // frame to eigen
    const unsigned int tick_per_slice = m_cfg["tick_per_slice"].asInt();
    std::vector<Array::array_xxf> ch_eigen;
    for (auto jtag : m_cfg["intags"]) {
        const std::string tag = jtag.asString();
        auto traces = Aux::tagged_traces(inframe, tag);
        if (traces.empty()) {
            log->warn("call={} no traces for input tag {}, using zeros", m_save_count, tag);
        }
        else { 
            log->debug("call={} tag={} ntraces={}", m_save_count, tag, traces.size());
        }
        ch_eigen.push_back(
            Array::downsample(frame_to_eigen(traces, m_anode, m_cfg["scale"].asFloat(), m_cfg["offset"].asFloat(),
                                             cbeg, cend, tick0, nticks),
                              tick_per_slice, 1));
    }
    log->debug(tk(fmt::format("call={} frame2eigen chans=[{},{}] ticks=[{},{}]",
                              m_save_count, cbeg, cend, tick0, tick0+nticks)));

    // eigen to tensor
    std::vector<torch::Tensor> ch;
    for (unsigned int i = 0; i < ch_eigen.size(); ++i) {
        ch.push_back(torch::from_blob(ch_eigen[i].data(), {ch_eigen[i].cols(), ch_eigen[i].rows()}));
        // const int ncols = ch_eigen[i].cols();
        // const int nrows = ch_eigen[i].rows();
        // std::cout << "ncols: " << ncols << "nrows: " << nrows << std::endl;
        // h5::write<float>(fd, String::format("/%d/frame_%s%d%d", m_save_count, "ch", i, 0), ch_eigen[i].data(),
        // h5::count{ncols, nrows}, h5::chunk{ncols, nrows} | h5::gzip{2});
    }
    auto img = torch::stack(ch, 0);
    auto batch = torch::stack({torch::transpose(img, 1, 2)}, 0);

    // Create a vector of inputs.
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(batch);

    log->debug(tk(fmt::format("call={} eigen2tensor", m_save_count)));

    // Execute the model and turn its output into a tensor.
    auto iitens = Pytorch::to_itensor(inputs);
    ITensorSet::pointer oitens = forward(iitens);
    if (!oitens or oitens->tensors()->size() != 1) {
        log->critical("call={} unexpected tensor size {} 1= 1",
                      m_save_count, oitens->tensors()->size());
        THROW(ValueError() << errmsg{"oitens->tensors()->size()!=1"});
    }
    torch::Tensor output = Pytorch::from_itensor({oitens}).front().toTensor().cpu();

    log->debug(tk(fmt::format("call={} forward", m_save_count)));

    // tensor to eigen
    Eigen::Map<Eigen::ArrayXXf> out_e(output[0][0].data<float>(), output.size(3), output.size(2));
    auto mask_e = Array::upsample(out_e, tick_per_slice, 0);

    log->debug(tk(fmt::format("call={} tensor2eigen", m_save_count)));

    // decon charge frame to eigen
    
    Array::array_xxf decon_charge_eigen;
    {
        auto dctag = get<std::string>(m_cfg, "decon_charge_tag");
        auto traces = Aux::tagged_traces(inframe, dctag);
        if (traces.empty()) {
            log->warn("call={} no traces for input tag {}, using zeros", m_save_count, dctag);
        }
        else { 
            log->debug("call={} tag={} ntraces={}", m_save_count, dctag, traces.size());
        }
        decon_charge_eigen =
            frame_to_eigen(traces, m_anode, 1., 0., cbeg, cend, tick0, nticks);
    }
    // log->debug("decon_charge_eigen: ncols: {} nrows: {}", decon_charge_eigen.cols(), decon_charge_eigen.rows()); // c600
    // x r800

    // apply ROI
    auto sp_charge = Array::mask(decon_charge_eigen.transpose(), mask_e, m_cfg["charge_thresh"].asFloat() /*0.7*/);
    sp_charge = Array::baseline_subtraction(sp_charge);

#ifdef DNNROI_HDF5_DEBUG
    // hdf5 eval
    {
        h5::fd_t fd = h5::open(m_cfg["evalfile"].asString(), H5F_ACC_RDWR);
        const unsigned long ncols = mask_e.cols();
        const unsigned long nrows = mask_e.rows();
        // l->info("ncols: {} nrows: {}", ncols, nrows);
        std::string aname = String::format("/%d/frame_%s%d", m_save_count, "dlroi", m_anode->ident());
        h5::write<float>(fd, aname, mask_e.data(), h5::count{ncols, nrows}, h5::chunk{ncols, nrows} | h5::gzip{2});

        aname = String::format("/%d/frame_%s%d", m_save_count, "dlcharge", m_anode->ident());
        h5::write<float>(fd, aname, sp_charge.data(), h5::count{ncols, nrows}, h5::chunk{ncols, nrows} | h5::gzip{2});
    }
    log->debug(tk(fmt::format("call={} h5", m_save_count)));
#endif

    // eigen to frame
    ITrace::vector* itraces = new ITrace::vector;
    IFrame::trace_list_t trace_index;
    eigen_to_traces(sp_charge, *itraces, trace_index,
                    m_anode->channels().front() + cbeg,
                    nticks, false);

    SimpleFrame* sframe = new SimpleFrame(inframe->ident(), inframe->time(), ITrace::shared_vector(itraces),
                                          inframe->tick(), inframe->masks());
    sframe->tag_frame("DNNROIFinding");
    sframe->tag_traces(m_cfg["outtag"].asString(), trace_index);

    log->debug("call={} produce {} traces: {}", m_save_count, itraces->size(), trace_index.size());

    outframe = IFrame::pointer(sframe);

    log->debug(tk(fmt::format("call={} eigen2frame", m_save_count)));
     
    ++m_save_count;
    return true;
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
