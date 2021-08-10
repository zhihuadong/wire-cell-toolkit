#include "WireCellSio/ClusterFileSink.h"

#include "WireCellAux/ClusterHelpers.h"

#include "custard/boost_custard.hpp"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellAux/FrameTools.h"


WIRECELL_FACTORY(ClusterFileSink, WireCell::Sio::ClusterFileSink,
                 WireCell::IClusterSink, WireCell::IConfigurable)

using namespace WireCell;

Sio::ClusterFileSink::ClusterFileSink()
  : m_drift_speed(1.6 * units::mm / units::us)
  , log(Log::logger("io"))
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
    // for conversion between time and "x" coordinate
    cfg["drift_speed"] = m_drift_speed;
    return cfg;
}

void Sio::ClusterFileSink::configure(const WireCell::Configuration& cfg)
{
    m_outname = get(cfg, "outname", m_outname);
    m_output_frame = get(cfg, "output_frame", m_output_frame);
    m_drift_speed = get(cfg, "drift_speed", m_drift_speed);

    m_out.clear();
    custard::output_filters(m_out, m_outname);
    if (m_out.empty()) {
        THROW(ValueError() << errmsg{"unsupported outname: " + m_outname});
    }
}

bool Sio::ClusterFileSink::operator()(const ICluster::pointer& cluster)
{
    auto top = Aux::cluster::jsonify(cluster, m_drift_speed);
    std::stringstream topss;
    topss << top;
    auto tops = topss.str();
    std::stringstream sizess;
    sizess << tops.size();
    auto sizes = sizess.str();
    auto cname = Aux::cluster::name(cluster);
    m_out << cname << "\n" << sizes << "\n" << tops;

    if (m_output_frame) {
        log->warn("ClsuterFileSink: frame output not yet implemented");
    }
    return true;
}
