#include "WireCellZio/ZioTensorSetSource.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(ZioTensorSetSource, WireCell::Zio::ZioTensorSetSource,
                 WireCell::ITensorSetSource, WireCell::IConfigurable)

using namespace WireCell;

Zio::ZioTensorSetSource::ZioTensorSetSource() : l(Log::logger("zio")) {}

Zio::ZioTensorSetSource::~ZioTensorSetSource() {}

Configuration Zio::ZioTensorSetSource::default_configuration() const
{
    Configuration cfg;
    return cfg;
}

void Zio::ZioTensorSetSource::configure(const WireCell::Configuration &cfg)
{
    m_cfg = cfg;
}

bool Zio::ZioTensorSetSource::operator()(ITensorSet::pointer &out)
{
    // if buffer not empty
    if (m_tensors.size() > 0)
    {
        out = m_tensors.back();
        m_tensors.pop_back();
        return true;
    }

    // fill m_tensors

    // issue quit if buffer empty after filling
    if (m_tensors.empty())
    {
        return false;
    }

    // pop one from buffer
    out = m_tensors.back();
    m_tensors.pop_back();
    return true;
}