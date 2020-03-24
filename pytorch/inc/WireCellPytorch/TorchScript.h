/** A wrapper for pytorch torchscript
 */

#ifndef WIRECELLPYTORCH_TORCHSCRIPT
#define WIRECELLPYTORCH_TORCHSCRIPT

#include "WireCellIface/IConfigurable.h"
#include "WireCellPytorch/ITorchScript.h"
#include "WireCellUtil/Logging.h"

#include <torch/script.h> // One-stop header.

namespace WireCell {
namespace Pytorch {
class TorchScript : public ITorchScript, public IConfigurable {
public:
  TorchScript();
  virtual ~TorchScript() {}

  // IConfigurable interface
  virtual void configure(const WireCell::Configuration &config);
  virtual WireCell::Configuration default_configuration() const;

  // ITorchScript interface
  virtual int ident() const { return m_ident; }
  virtual bool gpu() const {return get<bool>(m_cfg, "gpu", false);}
  virtual ITensorSet::pointer forward(const ITensorSet::pointer &inputs);

private:
  int m_ident;
  Log::logptr_t l;
  Configuration m_cfg; /// copy of configuration

  torch::jit::script::Module m_module;

  std::unordered_map<std::string, float> m_timers;

};
} // namespace Pytorch
} // namespace WireCell

#endif // WIRECELLPYTORCH_TORCHSCRIPT