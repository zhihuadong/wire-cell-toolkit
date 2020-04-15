#include "WireCellZio/TensorSetSource.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellZio/TensUtil.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(ZioTensorSetSource, WireCell::Zio::TensorSetSource,
                 WireCell::ITensorSetSource, WireCell::IConfigurable)

using namespace WireCell;

Zio::TensorSetSource::TensorSetSource()
    : FlowConfigurable("inject")
    , l(Log::logger("zio")){}

Zio::TensorSetSource::~TensorSetSource() {}

bool Zio::TensorSetSource::operator()(ITensorSet::pointer &out)
{
    out = nullptr;

    pre_flow();
    if (!m_flow) {
        return false;
    }

    zio::Message msg;
    bool noto;
    try {
        noto = m_flow->get(msg);
    }
    catch (zio::flow::end_of_transmission) {
        l->debug("[TensorSetSource {}:{}] got EOT on flow get DAT",
                 m_node.nick(), m_portname);
        m_flow->eotack();
        m_flow = nullptr;
        return true;            // this counts as EOS
    }
    if (!noto) {                // fixme: maybe loop a few times before giving up?
        l->warn("[TensorSetSource {}:{}] timeout on flow get",
                m_node.nick(), m_portname);                
        finalize();
        return false;
    }

    const zio::multipart_t& pls = msg.payload();
    if (!pls.size()) {           // EOS
        return true;
    }

    out = Zio::unpack(msg);
    return true;
}
