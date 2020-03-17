

#include "WireCellUtil/String.h"

#include "WireCellAux/SimpleTensorSet.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellIface/ITensorSet.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellUtil/Exceptions.h"

#include "generaldomo/client.hpp"
#include "zio/tens.hpp"

#include <torch/script.h> // One-stop header.

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace WireCell;

namespace
{
ITensorSet::pointer to_itensor(const std::vector<torch::IValue> &inputs)
{
    ITensor::vector *itv = new ITensor::vector;

    int ind = 0;
    for (auto ival : inputs)
    {
        auto ten = ival.toTensor().cpu();
        std::cout << "ten.shape: {"
                  << ten.size(0) << ", "
                  << ten.size(1) << ", "
                  << ten.size(2) << ", "
                  << ten.size(3) << "} "
                  << std::endl;
        std::vector<size_t> shape = {(size_t)ten.size(1), (size_t)ten.size(2), (size_t)ten.size(3)};
        // TODO need to figure out type from dtyp
        Aux::SimpleTensor<float> *st = new Aux::SimpleTensor<float>(shape);
        std::cout << "st.shape: {"
                  << st->shape()[0] << ", "
                  << st->shape()[1] << ", "
                  << st->shape()[2] << "} \n";
        size_t nbyte = 4;
        for (auto n : shape)
            nbyte *= n;
        std::cout << "nbyte: " << nbyte << std::endl;
        auto data = (float *)st->data();
        std::cout
            << "ten[0][0].data<float>(): " << ten[0][0].data<float>() << "\n"
            << "ten[0].data<float>(): " << ten[0].data<float>() << "\n"
            << "(float*)ten[0].data<float>(): " << (float *)ten[0].data<float>() << "\n";
        auto src = (float *)ten[0].data<float>();
        std::cout
            << "src[0]: " << src[0] << "\n"
            << "src[nbyte/4-1]: " << src[nbyte / 4 - 1] << "\n";
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

    std::cout << "ten.shape: {"
              << ten.size(0) << ", "
              << ten.size(1) << ", "
              << ten.size(2) << ", "
              << ten.size(3) << "} "
              << std::endl;
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
zio::Message zio_tens_msg()
{
    std::vector<size_t> shape = {2, 2, 30};
    float tensor[2][2][30] = {0};
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
} // namespace

int main()
{
    // client
    std::shared_ptr<generaldomo::Client> m_client;
    generaldomo::console_log log;
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, ZMQ_CLIENT);
    try
    {
        m_client = std::make_shared<generaldomo::Client>(sock, "tcp://localhost:5555", log);
    }
    catch (...)
    {
        THROW(RuntimeError() << errmsg{"Client init faileds!"});
    }
    
    // complete msg
    // std::vector<torch::jit::IValue> inputs;
    // inputs.push_back(torch::ones({1, 2, 2, 600}, torch::dtype(torch::kFloat32).device(torch::kCUDA, 0)));

    // auto iitens = to_itensor(inputs);
    // std::cout << "to_itensor ... OK\n";
    // std::cout << "iitens->tensors()->size(): " << iitens->tensors()->size() << "\n";
    // auto iten = iitens->tensors()->front();
    // std::cout << "iten->shape: {"
    //           << iten->shape()[0] << ", "
    //           << iten->shape()[1] << ", "
    //           << iten->shape()[2] << "} \n";

    // auto msg = Zio::FlowConfigurable::pack(iitens);
    // std::cout << dump(msg) << "\n";

    // simple msg
    auto msg = zio_tens_msg();
    std::cout << dump(msg) << "\n";
    
    zmq::multipart_t mmsg(msg.toparts());
    m_client->send("echo", mmsg);
    std::cout << "m_client->send ... OK\n";

    mmsg.clear();
    m_client->recv(mmsg);
    std::cout << "m_client->recv ... OK\n";

    msg.fromparts(mmsg);
    auto oiten = Zio::FlowConfigurable::unpack(msg);
    return 0;
}