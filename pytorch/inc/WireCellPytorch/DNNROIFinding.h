/** Wrapper for Troch Script Model
 *
 */

#ifndef WIRECELLPYTORCH_TSMODEL
#define WIRECELLPYTORCH_TSMODEL

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameSink.h"
#include "WireCellIface/IFrameFilter.h"
#include "WireCellUtil/Array.h"
#include "WireCellUtil/Logging.h"


#include <torch/script.h> // One-stop header.

namespace WireCell {
namespace Pytorch {

class DNNROIFinding : public IFrameFilter, public IConfigurable {
public:
  DNNROIFinding();
  virtual ~DNNROIFinding();

  /// working operation - interface from IFrameFilter
  /// executed when called by pgrapher
  virtual bool operator()(const IFrame::pointer &inframe, IFrame::pointer& outframe);

  /// interfaces from IConfigurable

  /// exeexecuted once at node creation
  virtual WireCell::Configuration default_configuration() const;

  /// executed once after node creation
  virtual void configure(const WireCell::Configuration &config);

private:

  Array::array_xxf frame_to_eigen(const IFrame::pointer &inframe, const std::string & tag) const;

  Configuration m_cfg; /// copy of configuration
  IAnodePlane::pointer m_anode; /// pointer to some APA, needed to associate chnnel ID to planes

  int m_save_count;   // count frames saved
  
  /// SPD logger
  Log::logptr_t l;
};
} // namespace Pytorch
} // namespace WireCell

#endif