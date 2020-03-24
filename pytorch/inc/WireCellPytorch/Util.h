/** Utilities
 */

#ifndef WIRECELLPYTORCH_UTIL
#define WIRECELLPYTORCH_UTIL

#include "WireCellIface/ITensorSet.h"

#include <torch/script.h> // One-stop header.

namespace WireCell
{
namespace Pytorch
{
ITensorSet::pointer to_itensor(const std::vector<torch::IValue> &inputs);
std::vector<torch::IValue> from_itensor(const ITensorSet::pointer &inputs);

}; // namespace Pytorch
} // namespace WireCell

#endif // WIRECELLPYTORCH_UTIL