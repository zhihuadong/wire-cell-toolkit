/** A wrapper for pytorch torchscript
 */

#ifndef WIRECELLPYTORCH_ITORCHSCRIPT
#define WIRECELLPYTORCH_ITORCHSCRIPT

#include "WireCellUtil/IComponent.h"
#include "WireCellIface/ITensorSet.h"

namespace WireCell {
namespace Pytorch {
class ITorchScript : public IComponent<ITorchScript> {
public:
  virtual ~ITorchScript() {}

  /// Return the ident number
  virtual int ident() const = 0;

  /// gpu model or not
  virtual bool gpu() const = 0;

  /// wrapper of the torch script forward function
  virtual ITensorSet::pointer forward(const ITensorSet::pointer &inputs) = 0;
};
} // namespace Pytorch
} // namespace WireCell

#endif // WIRECELLPYTORCH_ITORCHSCRIPT