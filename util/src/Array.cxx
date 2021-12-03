#include "WireCellUtil/Array.h"
#include "WireCellUtil/Exceptions.h"

#include <algorithm>
#include <complex>

using namespace WireCell;
using namespace WireCell::Array;


WireCell::Array::array_xxf WireCell::Array::downsample(const Array::array_xxf& in, const unsigned int k, const int dim)
{
    if (dim == 0) {
        Array::array_xxf out = Array::array_xxf::Zero(in.rows() / k, in.cols());
        for (unsigned int i = 0; i < in.rows(); ++i) {
            out.row(i / k) = out.row(i / k) + in.row(i);
        }
        return out / k;
    }
    if (dim == 1) {
        Array::array_xxf out = Array::array_xxf::Zero(in.rows(), in.cols() / k);
        for (unsigned int i = 0; i < in.cols(); ++i) {
            out.col(i / k) = out.col(i / k) + in.col(i);
        }
        return out / k;
    }
    THROW(ValueError() << errmsg{"dim should be 0 or 1"});
}

WireCell::Array::array_xxf WireCell::Array::upsample(const Array::array_xxf& in, const unsigned int k, const int dim)
{
    if (dim == 0) {
        Array::array_xxf out = Array::array_xxf::Zero(in.rows() * k, in.cols());
        for (unsigned int i = 0; i < in.rows() * k; ++i) {
            out.row(i) = in.row(i / k);
        }
        return out;
    }
    if (dim == 1) {
        Array::array_xxf out = Array::array_xxf::Zero(in.rows(), in.cols() * k);
        for (unsigned int i = 0; i < in.cols() * k; ++i) {
            out.col(i) = in.col(i / k);
        }
        return out;
    }
    THROW(ValueError() << errmsg{"dim should be 0 or 1"});
}

WireCell::Array::array_xxf WireCell::Array::mask(const Array::array_xxf& in, const Array::array_xxf& mask,
                                                 const float th)
{
    Array::array_xxf ret = Eigen::ArrayXXf::Zero(in.rows(), in.cols());
    if (in.rows() != mask.rows() || in.cols() != mask.cols()) {
        THROW(ValueError() << errmsg{"in.rows()!=mask.rows() || in.cols()!=mask.cols()"});
    }
    return (mask > th).select(in, ret);
}

WireCell::Array::array_xxf WireCell::Array::baseline_subtraction(const Array::array_xxf& in)
{
    Array::array_xxf ret = Eigen::ArrayXXf::Zero(in.rows(), in.cols());
    for (int ich = 0; ich < in.cols(); ++ich) {
        int sta = 0;
        int end = 0;
        for (int it = 0; it < in.rows(); ++it) {
            if (in(it, ich) == 0) {
                if (sta < end) {
                    for (int i = sta; i < end + 1; ++i) {
                        ret(i, ich) =
                            in(i, ich) - (in(sta, ich) + (i - sta) * (in(end, ich) - in(sta, ich)) / (end - sta));
                    }
                }
                sta = it + 1;  // first tick in ROI
            }
            else {
                end = it;  // last tick in ROI
            }
        }
    }
    return ret;
}
