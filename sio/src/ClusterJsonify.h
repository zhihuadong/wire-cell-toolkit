#ifndef WIRECELL_SIO_CLUSTERJSONIFY
#define WIRECELL_SIO_CLUSTERJSONIFY

#include "WireCellUtil/Configuration.h"
#include "WireCellIface/ICluster.h"

namespace WireCell::Sio {
     Json::Value cluster_jsonify(const ICluster::pointer& in);
}

#endif

