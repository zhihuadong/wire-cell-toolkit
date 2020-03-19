#include "WireCellZio/ZioTorchScript.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"

#include "WireCellAux/SimpleTensorSet.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellIface/ITensorSet.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellUtil/Exceptions.h"

#include "zio/tens.hpp"

WIRECELL_FACTORY(ZioTorchScript, WireCell::Pytorch::ZioTorchScript,
                 WireCell::Pytorch::ITorchScript, WireCell::IConfigurable)

using namespace WireCell;

Pytorch::ZioTorchScript::ZioTorchScript() : m_ident(0), l(Log::logger("pytorch")) {}

Configuration Pytorch::ZioTorchScript::default_configuration() const
{
    Configuration cfg;

    // TorchScript model
    cfg["model"] = "model.ts";
    cfg["gpu"] = true;

    // if failed, wait this time and try again
    cfg["wait_time"] = 500; // ms

    // for debug
    cfg["nloop"] = 10;

    return cfg;
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
        // std::cout << "ten.shape: {"
        //           << ten.size(0) << ", "
        //           << ten.size(1) << ", "
        //           << ten.size(2) << ", "
        //           << ten.size(3) << "} "
        //           << std::endl;
        std::vector<size_t> shape = {(size_t)ten.size(1), (size_t)ten.size(2), (size_t)ten.size(3)};
        // TODO need to figure out type from dtyp
        Aux::SimpleTensor<float> *st = new Aux::SimpleTensor<float>(shape);
        // std::cout << "st.shape: {"
        //           << st->shape()[0] << ", "
        //           << st->shape()[1] << ", "
        //           << st->shape()[2] << "} \n";
        size_t nbyte = 4;
        for (auto n : shape)
            nbyte *= n;
        // std::cout << "nbyte: " << nbyte << std::endl;
        auto data = (float *)st->data();
        // std::cout
        //     << "ten[0][0].data<float>(): " << ten[0][0].data<float>() << "\n"
        //     << "ten[0].data<float>(): " << ten[0].data<float>() << "\n"
        //     << "(float*)ten[0].data<float>(): " << (float *)ten[0].data<float>() << "\n";
        // auto src = (float *)ten[0].data<float>();
        // std::cout
        //     << "src[0]: " << src[0] << "\n"
        //     << "src[nbyte/4-1]: " << src[nbyte / 4 - 1] << "\n";
        memcpy(data, (float *)ten[0][0].data<float>(), nbyte);
        itv->push_back(ITensor::pointer(st));
        ++ind;
    }

    int seqno = 0;
    Configuration md;
    // Json::Reader reader;
    // reader.parse(label[zio::tens::form]["metadata"].dump(), md);

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

    // std::cout << "ten.shape: {"
    //           << ten.size(0) << ", "
    //           << ten.size(1) << ", "
    //           << ten.size(2) << ", "
    //           << ten.size(3) << "} "
    //           << std::endl;
    return ten;
}

zio::Message zio_tens_msg()
{
    #define N2 20 // <--- breaks broker if change to 30
    std::vector<size_t> shape = {2, 2, N2};
    float tensor[2][2][N2] = {0};
    const float *tensor1 = (float *)tensor;
    zio::Message msg(zio::tens::form);
    // Add an initial, unrelated message part just to make sure tens
    // respects it.
    msg.add(zio::message_t((char *)nullptr, 0));

    Configuration label;
    label[zio::tens::form]["metadata"] = {};
    auto &meta = label[zio::tens::form]["metadata"];
    meta["tick"] = 500;
    Json::FastWriter jwriter;
    msg.set_label(jwriter.write(label));

    zio::tens::append(msg, tensor1, shape);

    return msg;
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

void Pytorch::ZioTorchScript::configure(const WireCell::Configuration &cfg)
{
    m_cfg = cfg;
}

torch::IValue
Pytorch::ZioTorchScript::forward(const std::vector<torch::IValue> &inputs)
{
    l->info("ZioTorchScript::forward");
    torch::IValue ret;
    int wait_time = m_cfg["wait_time"].asInt();
    int thread_wait_time = 0;

    zio::console_log log;
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, ZMQ_CLIENT);
    try {
        m_client = std::make_shared<zio::domo::Client>(sock, "tcp://localhost:5555", log);
    } catch (...) {
        THROW(RuntimeError() << errmsg{"Client init faileds!"});
    }

    // for(int iloop=0; iloop<m_cfg["nloop"].asInt();++iloop) {
    bool success = false;
    while (!success)
    {
        try
        {
            // auto iitens = to_itensor(inputs);
            // auto msg = Zio::FlowConfigurable::pack(iitens);
            // zmq::multipart_t mmsg(msg.toparts());
            // m_client->send("torch", mmsg);
            // mmsg.clear();
            // m_client->recv(mmsg);
            // msg.fromparts(mmsg);
            // auto oitens = Zio::FlowConfigurable::unpack(msg);
            // ret = from_itensor(oitens);

            auto iitens = to_itensor(inputs);
            std::cout << "torch -> itens ... OK\n";
            std::cout << dump(iitens);
            auto msg = Zio::FlowConfigurable::pack(iitens);
            std::cout << "itens -> msg ... OK\n";
            // std::cout << dump(msg) << "\n";
            zmq::multipart_t mmsg(msg.toparts());
            std::cout << mmsg.str() << "\n";
            m_client->send("torch", mmsg);
            std::cout << "Client::send ... OK\n";
            mmsg.clear();
            m_client->recv(mmsg);
            std::cout << "Client::recv ... OK\n";
            // std::cout << mmsg.str() << "\n";
            msg.fromparts(mmsg);
            // std::cout << dump(msg) << "\n";
            auto oitens = Zio::FlowConfigurable::unpack(msg);
            std::cout << "msg -> itens ... OK\n";
            // std::cout << dump(oitens);
            ret = from_itensor(oitens);
            std::cout << "itens -> torch ... OK\n";
            
            success = true;
        }
        catch (...)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
            thread_wait_time += wait_time;
        }
    }
    // }

    l->info("thread_wait_time: {} sec", thread_wait_time / 1000.);

    return ret;
}