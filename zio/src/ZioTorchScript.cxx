#include "WireCellZio/ZioTorchScript.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"

#include "WireCellAux/SimpleTensorSet.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellIface/ITensorSet.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellUtil/Exceptions.h"

#include "zio/domo/client.hpp"
#include "zio/tens.hpp"

WIRECELL_FACTORY(ZioTorchScript, WireCell::Pytorch::ZioTorchScript,
                 WireCell::Pytorch::ITorchScript, WireCell::IConfigurable)

using namespace WireCell;

Pytorch::ZioTorchScript::ZioTorchScript() : m_ident(0), l(Log::logger("pytorch")) {}

Configuration Pytorch::ZioTorchScript::default_configuration() const
{
    Configuration cfg;

    // zio
    cfg["address"] = "tcp://localhost:5555";
    cfg["service"] = "torch:dnnroi";

    // TorchScript model
    cfg["model"] = "model.ts";
    cfg["gpu"] = true;

    // if failed, wait this time and try again
    cfg["wait_time"] = 500; // ms

    // for debug
    cfg["nloop"] = 10;

    return cfg;
}

void Pytorch::ZioTorchScript::configure(const WireCell::Configuration &cfg)
{
    m_cfg = cfg;
}

namespace
{
ITensorSet::pointer to_itensor(const std::vector<torch::IValue> &inputs)
{
    ITensor::vector *itv = new ITensor::vector;

    int ind = 0;
    for (auto ival : inputs)
    {
        auto ten = ival.toTensor().cpu();
        std::vector<size_t> shape = {(size_t)ten.size(1), (size_t)ten.size(2), (size_t)ten.size(3)};
        // TODO need to figure out type from dtyp
        Aux::SimpleTensor<float> *st = new Aux::SimpleTensor<float>(shape);
        size_t nbyte = 4;
        for (auto n : shape)
            nbyte *= n;
        auto data = (float *)st->data();
        memcpy(data, (float *)ten[0][0].data<float>(), nbyte);
        itv->push_back(ITensor::pointer(st));
        ++ind;
    }

    int seqno = 0;
    Configuration md;

    return std::make_shared<Aux::SimpleTensorSet>(seqno, md, ITensor::shared_vector(itv));
}
torch::IValue from_itensor(const ITensorSet::pointer &inputs)
{
    if (inputs->tensors()->size() != 1)
    {
        THROW(ValueError() << errmsg{"inputs->tensors()->size()!=1"});
    }
    auto iten = inputs->tensors()->front();
    if (iten->shape().size() != 4)
    {
        THROW(ValueError() << errmsg{"iten->shape().size()!=4"});
    }
    //TODO determine data type from metadata
    auto ten = torch::from_blob((float *)iten->data(), {(long)iten->shape()[0],
                                                        (long)iten->shape()[1],
                                                        (long)iten->shape()[2],
                                                        (long)iten->shape()[3]});
    return ten;
}
std::string dump(const zio::Message &msg) {
    std::stringstream ss;
    ss << "zio.Message: ";
    ss << "ZIO" << msg.level() << msg.form() << msg.label();
    ss << " + [0x" << msg.origin() << "," << msg.granule() << "," << msg.seqno() << "]";
    ss << " + [" << msg.payload().str() << "]";
    return ss.str();
}
std::string dump(const ITensorSet::pointer &itens) {
    std::stringstream ss;
    ss << "ITensorSet: ";
    Json::FastWriter jwriter;
    ss << itens->ident() << ", " << jwriter.write(itens->metadata());
    for (auto iten : *itens->tensors()) {
        ss << "shape: [";
        for(auto l : iten->shape()) {
            ss << l << " ";
        }
        ss << "]\n";
    }
    return ss.str();
}
} // namespace

torch::IValue
Pytorch::ZioTorchScript::forward(const std::vector<torch::IValue> &inputs)
{
    l->debug("ZioTorchScript::forward");
    torch::IValue ret;
    int wait_time = m_cfg["wait_time"].asInt();
    int thread_wait_time = 0;

    zmq::context_t ctx;
    zmq::socket_t sock(ctx, ZMQ_CLIENT);
    zio::domo::Client m_client(sock, get<std::string>(m_cfg, "address", "tcp://localhost:5555"));

    // for(int iloop=0; iloop<m_cfg["nloop"].asInt();++iloop) {
    bool success = false;
    while (!success)
    {
        try
        {
            auto iitens = to_itensor(inputs);
            auto msg = Zio::FlowConfigurable::pack(iitens);
            zmq::multipart_t mmsg(msg.toparts());
            m_client.send(get<std::string>(m_cfg, "service", "torch:dnnroi"), mmsg);
            mmsg.clear();
            m_client.recv(mmsg);
            msg.fromparts(mmsg);
            auto oitens = Zio::FlowConfigurable::unpack(msg);
            ret = from_itensor(oitens);
            
            success = true;
        }
        catch (...)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
            thread_wait_time += wait_time;
        }
    }
    // }

    l->debug("thread_wait_time: {} sec", thread_wait_time / 1000.);

    return ret;
}