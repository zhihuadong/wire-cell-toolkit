#include "WireCellSio/ClusterFileSink.h"

#include "ClusterJsonify.h"

#include "custard/boost_custard.hpp"

#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(ClusterFileSink, WireCell::Sio::ClusterFileSink,
                 WireCell::IClusterSink, WireCell::IConfigurable)

using namespace WireCell;

Sio::ClusterFileSink::ClusterFileSink()
  : log(Log::logger("io"))
{
}

Sio::ClusterFileSink::~ClusterFileSink()
{
}

WireCell::Configuration Sio::ClusterFileSink::default_configuration() const
{
    Configuration cfg;
    // output json file.  A "%d" type format code may be included to be resolved by a cluster identifier.
    cfg["outname"] = m_outname;
    // whether to also output any referenced frames
    cfg["output_frame"] = m_output_frame;
    return cfg;
}

void Sio::ClusterFileSink::configure(const WireCell::Configuration& cfg)
{
    m_outname = get(cfg, "outname", m_filename);
    m_output_frame = get(cfg, "output_frame", m_output_frame);

    custard::complete_filter(m_out, m_outname);
    if (out.empty()) {
        THROW(ValueError() << errmsg{"unsupported outname: " + m_outname});
    }
}

bool Sio::ClusterFileSink::operator()(const input_pointer& in, output_pointer& out)
{
    out = in;
    if (!in) {
        return true;
    }

    auto top = cluster_jsonify(in);
    fstr << top;
    if (m_output_frame) {
        log.warning("ClusterFileSink: frame saving not yet implemented");
    }
    return true;
}
