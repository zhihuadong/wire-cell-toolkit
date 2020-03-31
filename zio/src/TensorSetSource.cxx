#include "WireCellZio/TensorSetSource.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellZio/TensUtil.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(ZioTensorSetSource, WireCell::Zio::TensorSetSource,
                 WireCell::ITensorSetSource, WireCell::IConfigurable)

using namespace WireCell;

Zio::TensorSetSource::TensorSetSource() : FlowConfigurable("inject"), l(Log::logger("zio")), m_had_eos(false) {}

Zio::TensorSetSource::~TensorSetSource() {}

bool Zio::TensorSetSource::operator()(ITensorSet::pointer &out)
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

    out = Zio::unpack(msg);

    return true;
}
