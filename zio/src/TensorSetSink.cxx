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
        l->debug("[TensorSetSink {}:{}] no flow",
                 m_node.nick(), m_portname);                
        return false;
    }

    if (!in) {  // eos
        l->debug("[TensorSetSink {}:{}] got EOS",
                 m_node.nick(), m_portname);                
        if (m_had_eos) {
            finalize();
            return false;
        }
        m_had_eos = true;
        l->debug("[TensorSetSink {}:{}] send empty message as EOS",
                 m_node.nick(), m_portname);                
        zio::Message eos("FLOW"); // send empty DAT as WCT EOS
        bool noto;
        try {
            noto = m_flow->put(eos);
        }
        catch (zio::flow::end_of_transmission) {
            l->debug("[TensorSetSink {}:{}] receive EOT on flow put EOS",
                     m_node.nick(), m_portname);

            m_flow->eotack();
            m_flow = nullptr;
            return false;
        }
        if (!noto) { // timeout
            l->warn("[TensorSetSink {}:{}] timeout on flow put EOS",
                    m_node.nick(), m_portname);
            finalize();
            return false;
        }

        return true;
    }

    m_had_eos = false;
    zio::Message msg = Zio::pack(in);

    l->debug("[TensorSetSink {}:{}] putting DAT",
             m_node.nick(), m_portname);

    bool noto;
    try {
        noto = m_flow->put(msg);
    }
    catch (zio::flow::end_of_transmission) {
        l->debug("[TensorSetSink {}:{}] got EOT on flow put DAT",
                 m_node.nick(), m_portname);
        m_flow->eotack();
        m_flow = nullptr;
        return false;
    }
    if (!noto) { // timeout
        l->warn("[TensorSetSink {}:{}] timeout on flow put",
                m_node.nick(), m_portname);
        finalize();
        return false;
    }
    l->debug("[TensorSetSink {}:{}] sent tensor as DAT",
             m_node.nick(), m_portname);                

    return true;
}

