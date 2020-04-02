#include "WireCellAux/Util.h"

using namespace WireCell;


std::string Aux::dump(const ITensor::pointer iten)
{
    std::stringstream ss;
    ss << "ITensor: ";
    Json::FastWriter jwriter;
    ss << "shape: [";
    for (auto l : iten->shape())
    {
        ss << l << " ";
    }
    ss << "]\n";

    return ss.str();
}