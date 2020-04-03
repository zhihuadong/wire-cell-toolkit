/** Utilities
 */

#ifndef WIRECELLAUX_UTIL
#define WIRECELLAUX_UTIL

#include "WireCellIface/ITensorSet.h"
#include "WireCellIface/ITensor.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellUtil/Exceptions.h"

#include <Eigen/Core>

#include <iostream>

namespace WireCell
{
namespace Aux
{

std::string dump(const ITensor::pointer iten);

template<typename ElementType>
inline Eigen::Array<ElementType, Eigen::Dynamic, Eigen::Dynamic> itensor_to_eigen_array(const ITensor::pointer iten)
{
    if (!iten)
    {
        THROW(ValueError() << errmsg{"(iten == null"});
    }

    if (iten->shape().size() != 2)
    {
        THROW(ValueError() << errmsg{"iten->shape().size()!=2"});
    }

    auto nrows = iten->shape()[0];
    auto ncols = iten->shape()[1];
    Eigen::Map< const Eigen::Array<ElementType, Eigen::Dynamic, Eigen::Dynamic> > arr((const ElementType *)iten->data(), nrows, ncols);

    return arr;
}

template<typename ElementType>
inline ITensor::pointer eigen_array_to_itensor(const Eigen::Array<ElementType, Eigen::Dynamic, Eigen::Dynamic> & arr)
{
    std::vector<size_t> shape = {(size_t)arr.rows(), (size_t)arr.cols()};
    SimpleTensor<ElementType> *st = new SimpleTensor<ElementType>(shape);
    auto dst = (ElementType *)st->data();
    auto src = (ElementType *)arr.data();
    size_t size = sizeof(ElementType) * arr.rows() * arr.cols();
    memcpy(dst, src, size);

    return ITensor::pointer(st);
}

}; // namespace Aux
} // namespace WireCell

#endif // WIRECELLSIG_UTIL