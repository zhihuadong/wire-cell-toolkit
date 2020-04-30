#include "WireCellSig/Decon2DFilter.h"
#include "WireCellSig/Util.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Array.h"
#include "WireCellUtil/Response.h"
#include "WireCellUtil/FFTBestLength.h"
#include "WireCellUtil/Exceptions.h"

#include "WireCellIface/ITensorSet.h"
#include "WireCellIface/IFilterWaveform.h"

#include "WireCellAux/SimpleTensorSet.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellAux/Util.h"
#include "WireCellAux/TensUtil.h"

WIRECELL_FACTORY(Decon2DFilter, WireCell::Sig::Decon2DFilter, WireCell::ITensorSetFilter, WireCell::IConfigurable)

using namespace WireCell;

Sig::Decon2DFilter::Decon2DFilter()
  : log(Log::logger("sig"))
{
}

Configuration Sig::Decon2DFilter::default_configuration() const
{
    Configuration cfg;

    return cfg;
}

void Sig::Decon2DFilter::configure(const WireCell::Configuration &cfg)
{
    m_cfg = cfg;

    // m_anode = Factory::find_tn<IAnodePlane>(get<std::string>(m_cfg, "anode", "Anode"));
    // if (!m_anode) {
    //     THROW(ValueError() << errmsg{"Sig::Decon2DFilter::configure !m_anode"});
    // }

    // auto cresp_n = get<std::string>(m_cfg, "per_chan_resp", "PerChannelResponse");
    // if (!cresp_n.empty()) {
    //     m_cresp = Factory::find_tn<IChannelResponse>(cresp_n);
    //     if (!m_cresp) {
    //         THROW(ValueError() << errmsg{"Sig::Decon2DFilter::configure !m_cresp"});
    //     }
    // }
    // else {
    //     m_cresp = nullptr;
    //     log->warn("Sig::Decon2DFilter::configure No PerChannelResponse");
    // }

    // m_fresp = Factory::find_tn<IFieldResponse>(get<std::string>(m_cfg, "field_response", "FieldResponse"));
    // if (!m_fresp) {
    //     THROW(ValueError() << errmsg{"Sig::Decon2DFilter::configure !m_fresp"});
    // }
}

namespace {
    std::string dump(const ITensorSet::pointer &itens)
    {
        std::stringstream ss;
        ss << "ITensorSet: ";
        Json::FastWriter jwriter;
        ss << itens->ident() << ", " << jwriter.write(itens->metadata());
        for (auto iten : *itens->tensors()) {
            ss << "shape: [";
            for (auto l : iten->shape()) {
                ss << l << " ";
            }
            ss << "]\n";
        }
        return ss.str();
    }
}  // namespace

bool Sig::Decon2DFilter::operator()(const ITensorSet::pointer &in, ITensorSet::pointer &out)
{
    log->debug("Decon2DFilter: start");

    if (!in) {
        out = nullptr;
        return true;
    }
    auto set_md = in->metadata();

    auto tag = get<std::string>(m_cfg, "tag", "trace_tag");

    auto iwf_ten = Aux::get_tens(in, tag, "waveform");
    if (!iwf_ten) {
        log->error("Tensor->Frame: failed to get waveform tensor for tag {}", tag);
        THROW(ValueError() << errmsg{"Sig::Decon2DFilter::operator() !iwf_ten"});
    }
    auto wf_shape = iwf_ten->shape();
    size_t nchans = wf_shape[0];
    size_t nticks = wf_shape[1];
    log->debug("iwf_ten->shape: {} {}", nchans, nticks);

    auto ch_ten = Aux::get_tens(in, tag, "channels");
    if (!ch_ten) {
        log->error("Tensor->Frame: failed to get channels tensor for tag {}", tag);
        THROW(ValueError() << errmsg{"Sig::Decon2DFilter::operator() !ch_ten"});
    }
    Eigen::Map<const Eigen::ArrayXi> ch_arr((const int *) ch_ten->data(), nchans);

    bool have_summary = true;
    auto sum_ten = Aux::get_tens(in, tag, "summary");
    if (!sum_ten) {
        log->debug("Tensor->Frame: no optional summary tensor for tag {}", tag);
        have_summary = false;
    }

    int iplane = get<int>(m_cfg, "iplane", 0);
    log->debug("iplane {}", iplane);

    const std::vector<std::string> filter_names{"Wire_ind", "Wire_ind", "Wire_col"};

    // bins
    // double m_period = get(set_md, "tick", 0.5 * units::microsecond);
    // int m_fft_nwires = nchans;
    int m_pad_nwires = 0;
    // int m_fft_nticks = nticks;
    // int m_pad_nticks = 0;

    // FIXME: figure this out
    int m_nwires = nchans;
    int m_nticks = nticks;

    // time offset
    // double m_fine_time_offset = get<double>(m_cfg, "ftoffset", 0.0 * units::microsecond);
    // double m_coarse_time_offset = get<double>(m_cfg, "ctoffset", -8.0 * units::microsecond);
    // auto fr = m_fresp->field_response();
    // double m_intrinsic_time_offset = fr.origin / fr.speed;

    // gain, shaping time, other applification factors
    // double m_gain = get<double>(m_cfg, "gain", 14.0 * units::mV / units::fC);
    // double m_shaping_time = get<double>(m_cfg, "shaping_time", 2.2 * units::microsecond);
    // double m_inter_gain = get<double>(m_cfg, "inter_gain", 1.2);
    // double m_ADC_mV = get<double>(m_cfg, "ADC_mV", 4096 / (2000. * units::mV));

    // TensorSet to Eigen
    WireCell::Array::array_xxc c_data = Aux::itensor_to_eigen_array<std::complex<float>>(iwf_ten);
    log->debug("c_data: {} {}", c_data.rows(), c_data.cols());

    // apply software filter on time
    Waveform::realseq_t roi_hf_filter_wf;

    // FIXME: need to handle collection
    auto ncr1 = Factory::find<IFilterWaveform>("HfFilter", "Wiener_tight_U");
    roi_hf_filter_wf = ncr1->filter_waveform(c_data.cols());
    auto ncr2 = Factory::find<IFilterWaveform>("LfFilter", "ROI_tight_lf");
    auto temp_filter = ncr2->filter_waveform(c_data.cols());
    for (size_t i = 0; i != roi_hf_filter_wf.size(); i++) {
        roi_hf_filter_wf.at(i) *= temp_filter.at(i);
    }

    Array::array_xxc c_data_afterfilter(c_data.rows(), c_data.cols());
    for (int irow = 0; irow < c_data.rows(); ++irow) {
        for (int icol = 0; icol < c_data.cols(); ++icol) {
            c_data_afterfilter(irow, icol) = c_data(irow, icol) * roi_hf_filter_wf.at(icol);
        }
    }

    // do the second round of inverse FFT on wire
    Array::array_xxf tm_r_data = Array::idft_cr(c_data_afterfilter, 0);
    Array::array_xxf r_data = tm_r_data.block(m_pad_nwires, 0, m_nwires, m_nticks);
    Sig::restore_baseline(r_data);

    // Eigen to TensorSet
    // auto owf_ten = Aux::eigen_array_to_simple_tensor<std::complex<float> >(c_data);

    // Debug: inverte fft and save out
    auto owf_ten = Aux::eigen_array_to_simple_tensor<float>(r_data);

    // save back to ITensorSet
    ITensor::vector *itv = new ITensor::vector;
    auto iwf_md = iwf_ten->metadata();
    auto &owf_md = owf_ten->metadata();
    owf_md["tag"] = tag;
    owf_md["pad"] = get<float>(iwf_md, "pad", 0.0);
    owf_md["tbin"] = get<float>(iwf_md, "tbin", 0.0);
    owf_md["type"] = "waveform";
    itv->push_back(ITensor::pointer(owf_ten));

    // FIXME need to change ch_ten tag to outtag
    itv->push_back(ch_ten);
    if (have_summary) {
        itv->push_back(sum_ten);
    }

    Configuration oset_md(in->metadata());
    oset_md["tags"] = Json::arrayValue;
    oset_md["tags"].append(tag);

    out = std::make_shared<Aux::SimpleTensorSet>(in->ident(), oset_md, ITensor::shared_vector(itv));

    log->debug("Decon2DFilter: end");

    return true;
}