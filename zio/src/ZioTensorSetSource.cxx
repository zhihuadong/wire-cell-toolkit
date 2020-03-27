#include "WireCellZio/ZioTensorSetSource.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(ZioTensorSetSource, WireCell::Zio::ZioTensorSetSource,
                 WireCell::ITensorSetSource, WireCell::IConfigurable)

using namespace WireCell;

Zio::ZioTensorSetSource::ZioTensorSetSource() : FlowConfigurable("inject"), l(Log::logger("zio")), m_had_eos(false) {}

Zio::ZioTensorSetSource::~ZioTensorSetSource() {}

bool Zio::ZioTensorSetSource::operator()(ITensorSet::pointer &out)
{
    out = nullptr;

    pre_flow();
    if (!m_flow) {
        return false;
    }

    zio::Message msg;
    bool ok = m_flow->get(msg, m_timeout);
    if (!ok) {
        zio::Message eot;
        m_flow->send_eot(eot);
        m_flow = nullptr;
        return false;           // timeout, eot or other error
    }

    const zio::multipart_t& pls = msg.payload();
    if (!pls.size()) {           // EOS
        if (m_had_eos) {         // 2nd
            finalize();
            return false;
        }
        m_had_eos = true;
        return true;
    }
    m_had_eos = false;

    out = Zio::FlowConfigurable::unpack(msg);

    return true;
}
