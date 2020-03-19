/** A wrapper for pytorch torchscript using zio
 */

#ifndef WIRECELLPYTORCH_ZIOTORCHSCRIPT
#define WIRECELLPYTORCH_ZIOTORCHSCRIPT

#include "WireCellIface/IConfigurable.h"
#include "WireCellPytorch/ITorchScript.h"
#include "WireCellUtil/Logging.h"
#include "zio/domo/client.hpp"

#include <torch/script.h> // One-stop header.

namespace WireCell {
namespace Pytorch {
class ZioTorchScript : public ITorchScript, public IConfigurable {
public:
  ZioTorchScript();
  virtual ~ZioTorchScript() {}

  // IConfigurable interface
  virtual void configure(const WireCell::Configuration &config);
  virtual WireCell::Configuration default_configuration() const;

  // ITorchScript interface
  virtual int ident() const { return m_ident; }
  virtual bool gpu() const {return get<bool>(m_cfg, "gpu", false);}
  virtual torch::IValue forward(const std::vector<torch::IValue> &inputs);

private:
  int m_ident;
  Log::logptr_t l;
  Configuration m_cfg; /// copy of configuration

  std::shared_ptr<zio::domo::Client> m_client;

  std::unordered_map<std::string, float> m_timers;

};
} // namespace Pytorch
} // namespace WireCell

#endif // WIRECELLPYTORCH_ZIOTORCHSCRIPT