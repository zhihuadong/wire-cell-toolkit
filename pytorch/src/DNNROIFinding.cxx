#include "WireCellPytorch/DNNROIFinding.h"
#include "WireCellIface/ITrace.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellIface/SimpleTrace.h"

#include "WireCellIface/FrameTools.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Array.h"

#include <h5cpp/all>
#include <torch/script.h> // One-stop header.

#include <string>
#include <vector>

/// macro to register name - concrete pair in the NamedFactory
/// @param NAME - used to configure node in JSON/Jsonnet
/// @parame CONCRETE - C++ concrete type
/// @parame ... - interfaces
WIRECELL_FACTORY(DNNROIFinding, WireCell::Pytorch::DNNROIFinding,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace WireCell;

Pytorch::DNNROIFinding::DNNROIFinding() : m_save_count(0), l(Log::logger("pytorch")) {}

Pytorch::DNNROIFinding::~DNNROIFinding() {}

void Pytorch::DNNROIFinding::configure(const WireCell::Configuration &cfg) {

  auto anode_tn = cfg["anode"].asString();
  m_anode = Factory::find_tn<IAnodePlane>(anode_tn);

  auto model_path = cfg["model"].asString();
  if (model_path.empty()) {
    THROW(ValueError() << errmsg{
              "Must provide output filename to HDF5FrameTap"});
  }

  
  std::string fn = cfg["filename"].asString();
  if (fn.empty()) {
    THROW(ValueError() << errmsg{
              "Must provide output filename to HDF5FrameTap"});
  }

  h5::create(fn, H5F_ACC_TRUNC);

  m_cfg = cfg;

  // load Torch Script Model
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    module = torch::jit::load(m_cfg["model"].asString());
    module.to(at::kCUDA);
    l->info("Model: {} loaded",m_cfg["model"].asString());
  }
  catch (const c10::Error& e) {
    l->critical( "error loading model: {}", m_cfg["model"].asString());
    exit(0);
  }

  m_timers.insert({"frame2eigen",0});
  m_timers.insert({"eigen2tensor",0});
  m_timers.insert({"forward",0});
  m_timers.insert({"tensor2eigen",0});
  m_timers.insert({"h5",0});
}

WireCell::Configuration Pytorch::DNNROIFinding::default_configuration() const {
  Configuration cfg;

  // This number is set to the waveform sample array before any
  // charge is added.
  cfg["baseline"] = 0.0;

  // This number will be multiplied to each waveform sample before
  // casting to dtype.
  cfg["scale"] = 1.0;

  // This number will be added to each scaled waveform sample before
  // casting to dtype.
  cfg["offset"] = 0.0;

  cfg["model"] = "model.pt";

  cfg["anode"] = "AnodePlane";
  cfg["nticks"] = 6000;
  cfg["tick0"]  = 0;

  // taces used as input
  cfg["trace_tags"] = Json::arrayValue;
  
  // decon charge to fill in ROIs
  cfg["decon_charge_tag"] = "decon_charge0";

  cfg["filename"] = "tsmodel-eval.h5";

  return cfg;
}

namespace {
  Array::array_xxf rebin(const Array::array_xxf &in, const unsigned int k) {
    Array::array_xxf out = Array::array_xxf::Zero(in.rows(), in.cols()/k);
    for(unsigned int i=0; i<in.cols(); ++i) {
      out.col(i/k) = out.col(i/k) + in.col(i);
    }
    return out/k;
  }

  Array::array_xxf mask(const Array::array_xxf &in, const Array::array_xxf &mask, const float th = 0.5) {
    Array::array_xxf ret = Eigen::ArrayXXf::Zero(in.rows(),in.cols());
    if(in.rows()!=mask.rows() || in.cols()!=mask.cols()) {
      std::cerr << "error: in.rows()!=mask.rows() || in.cols()!=mask.cols()\n";
      return ret;
    }
    // for(int icol=0; icol<in.cols();++icol) {
    //   auto in_col = in.col(icol);
    //   auto mask_col = mask.col(icol);
    // }
    return (mask>th).select(in, ret);
  }

  Array::array_xxf baseline_subtraction(const Array::array_xxf &in) {
    Array::array_xxf ret = Eigen::ArrayXXf::Zero(in.rows(),in.cols());
    // for(int icol=0; icol<in.cols();++icol) {
    //   auto in_col = in.col(icol);
    //   auto mask_col = mask.col(icol);
    // }
    return ret;
  }
}


Array::array_xxf Pytorch::DNNROIFinding::frame_to_eigen(const IFrame::pointer &inframe, const std::string & tag) const {
  const unsigned int tick_rebin = 10;
  const double scaling = 4000.;
  const int win_cbeg = 800;
  const int win_cend = 1600; // not include
  
  const float baseline = m_cfg["baseline"].asFloat();
  const float scale = m_cfg["scale"].asFloat();
  const float offset = m_cfg["offset"].asFloat();

  const int tick0 = m_cfg["tick0"].asInt();
  const int nticks = m_cfg["nticks"].asInt();
  const int tbeg = tick0;
  const int tend = tick0+nticks-1;
  auto channels = m_anode->channels();
  // const int cbeg = channels.front();
  // const int cend = channels.back();
  const int cbeg = channels.front()+win_cbeg;
  const int cend = channels.front()+win_cend-1;
  l->debug("{}: t: {} - {}; c: {} - {}",
            m_cfg["anode"].asString(),
            tbeg, tend, cbeg, cend);        

  const size_t ncols = nticks;
  const size_t nrows = cend-cbeg+1;

  auto traces = FrameTools::tagged_traces(inframe, tag);
  l->debug("DNNROIFinding: save {} tagged as {}", traces.size(), tag);
  if (traces.empty()) {
      l->warn("DNNROIFinding: no traces for tag: \"{}\"", tag);
      return Array::array_xxf::Zero(nrows/tick_rebin, ncols);
  }

  Array::array_xxf arr = Array::array_xxf::Zero(nrows, ncols) + baseline;
  // FrameTools::fill(arr, traces, channels.begin(), channels.end(), tick0);
  FrameTools::fill(arr, traces, channels.begin()+win_cbeg, channels.begin()+win_cend, tick0);
  arr = arr * scale + offset;

  return rebin(arr,tick_rebin)/scaling;
}

bool Pytorch::DNNROIFinding::operator()(const IFrame::pointer &inframe,
                                      IFrame::pointer &outframe) {

  outframe = inframe;
  if (!inframe) {
      l->debug("DNNROIFinding: EOS");
      outframe = nullptr;
      return true;
  }

  if (m_cfg["trace_tags"].size() != 3) {
    return true;
  }

  h5::fd_t fd = h5::open(m_cfg["filename"].asString(), H5F_ACC_RDWR);
  std::clock_t start;
  double duration = 0;

  // frame to eigen
  start = std::clock();
  duration = 0;
  std::vector<Array::array_xxf> ch_eigen;
  for (auto jtag : m_cfg["trace_tags"]) {
    const std::string tag = jtag.asString();
    ch_eigen.push_back(frame_to_eigen(inframe, tag));
  }
  duration += (std::clock() - start) / (double)CLOCKS_PER_SEC;
  l->info("frame2eigen: {}", duration);
  m_timers["frame2eigen"] += duration;

  start = std::clock();
  duration = 0;
  // eigen to tensor
  torch::Tensor ch[3];
  for(unsigned int i=0; i<3; ++i) {
    ch[i] = torch::from_blob(ch_eigen[i].data(), {ch_eigen[i].cols(),ch_eigen[i].rows()});
    // const int ncols = ch_eigen[i].cols();
    // const int nrows = ch_eigen[i].rows();
    // std::cout << "ncols: " << ncols << "nrows: " << nrows << std::endl;
    // h5::write<float>(fd, String::format("/%d/frame_%s%d%d", m_save_count, "ch", i, 0), ch_eigen[i].data(), h5::count{ncols, nrows}, h5::chunk{ncols, nrows} | h5::gzip{2});
  }
  auto img = torch::stack({ch[0], ch[1], ch[2]}, 0);
  auto batch = torch::stack({torch::transpose(img,1,2)}, 0);

  // std::cout << "batch.shape: {"
  // << batch.size(0) << ", "
  // << batch.size(1) << ", "
  // << batch.size(2) << ", "
  // << batch.size(3) << "} "
  // << std::endl;

  // Create a vector of inputs.
  std::vector<torch::jit::IValue> inputs;
  inputs.push_back(batch.cuda());

  duration += (std::clock() - start) / (double)CLOCKS_PER_SEC;
  l->info("eigen2tensor: {}", duration);
  m_timers["eigen2tensor"] += duration;
  
  start = std::clock();
  duration = 0;
  // Execute the model and turn its output into a tensor.
  torch::Tensor output = module.forward(inputs).toTensor().cpu();

  duration += (std::clock() - start) / (double)CLOCKS_PER_SEC;
  l->info("forward: {}", duration);
  m_timers["forward"] += duration;

  // std::cout << "output.shape: {"
  // << output.size(0) << ", "
  // << output.size(1) << ", "
  // << output.size(2) << ", "
  // << output.size(3) << "} "
  // << std::endl;
  
  start = std::clock();
  duration = 0;
  // tensor to eigen
  Eigen::Map<Eigen::ArrayXXf> out_e(output[0][0].data<float>(), output.size(3), output.size(2));

  duration += (std::clock() - start) / (double)CLOCKS_PER_SEC;
  l->info("tensor2eigen: {}", duration);
  m_timers["tensor2eigen"] += duration;

  // decon charge frame to eigen
  Array::array_xxf decon_charge_eigen = frame_to_eigen(inframe, m_cfg["decon_charge_tag"].asString());
  l->info("ncols: {} nrows: {}", decon_charge_eigen.cols(), decon_charge_eigen.rows());

  // apply ROI
  auto sp_charge = mask(decon_charge_eigen.transpose(), out_e, 0.7);
  
  start = std::clock();
  duration = 0;
  // eigen to frame
  {
    const int ncols = out_e.cols();
    const int nrows = out_e.rows();
    l->info("ncols: {} nrows: {}", ncols, nrows);
    std::string aname = String::format("/%d/frame_%s%d", m_save_count, "dlroi", 0);
    h5::write<float>(fd, aname, out_e.data(), h5::count{ncols, nrows}, h5::chunk{ncols, nrows} | h5::gzip{2});

    aname = String::format("/%d/frame_%s%d", m_save_count, "dlcharge", 0);
    h5::write<float>(fd, aname, sp_charge.data(), h5::count{ncols, nrows}, h5::chunk{ncols, nrows} | h5::gzip{2});
  }
  duration += (std::clock() - start) / (double)CLOCKS_PER_SEC;
  l->info("h5: {}", duration);
  m_timers["h5"] += duration;

  for (auto pair : m_timers) {
    l->info("{} : {}",pair.first, pair.second);
  }

  ++m_save_count;
  return true;
}

// Local Variables:
// mode: c++
// c-basic-offset: 2
// End:
