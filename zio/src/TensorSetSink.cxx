#include "WireCellZio/TensorSetSink.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellZio/TensUtil.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(ZioTensorSetSink, WireCell::Zio::TensorSetSink,
                 WireCell::ITensorSetSink, WireCell::IConfigurable)

using namespace WireCell;

Zio::TensorSetSink::TensorSetSink()
    : FlowConfigurable("extract"),
      l(Log::logger("zio")),
      m_had_eos(false)
{
}

Zio::TensorSetSink::~TensorSetSink() {}

void Zio::TensorSetSink::post_configure()
{
    if (!pre_flow()) {
        throw std::runtime_error("Failed to set up flow.  Is server alive?");
    }
}

bool Zio::TensorSetSink::operator()(const ITensorSet::pointer &in)
{
    pre_flow();
    if (!m_flow) {
        return false;
    }

    if (!in) {  // eos
        if (m_had_eos) {
            finalize();
            return false;
        }
        m_had_eos = true;
        zio::Message msg("FLOW"); // send empty as EOS
        bool ok = m_flow->put(msg);
        if (!ok) {
            zio::Message eot;
            m_flow->send_eot(eot);
            m_flow = nullptr;
            return false;
        }
        return true;
    }

    m_had_eos = false;
    zio::Message msg = Zio::pack(in);

    bool ok = m_flow->put(msg);
    if (!ok) {                  // got eot from remote
        zio::Message eot;
        m_flow->send_eot(eot);
        m_flow = nullptr;
        return false;
    }
    return true;
}

