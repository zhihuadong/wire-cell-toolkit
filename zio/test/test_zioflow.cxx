/// Test tensor -> flow -> tensor 

#include "WireCellZio/TensorSetSink.h"
#include "WireCellZio/TensorSetSource.h"
#include "WireCellZio/TensUtil.h"
#include "WireCellUtil/Testing.h"

#include "zio/actor.hpp"
#include "zio/main.hpp"
#include "zio/tens.hpp"
#include "zio/logging.hpp"

static zio::message_t msg_ready("READY", 5);
static zio::message_t msg_done("DONE", 4);
static zio::message_t msg_term("TERM", 4);

using namespace WireCell;

template<typename TYPE>
ITensorSet::pointer make_tensor_set()
{
    zio::info ("by_type {}{}", zio::tens::type_name(typeid(TYPE)), sizeof(TYPE));

    TYPE tensor[2][3][4] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    const TYPE* tensor1 = (TYPE*) tensor;
    std::vector<size_t> shape={2,3,4};

    zio::Message msg1(zio::tens::form);

    // Add an initial, unrelated message part just to make sure tens
    // respects the rules that tens parts aren't all parts.
    msg1.add(zio::message_t((char*)nullptr,0));
    assert(msg1.payload().size() == 1);
    zio::tens::append(msg1, tensor1, shape);
    assert(msg1.payload().size() == 2);

    {
        auto lo1 = msg1.label_object();
        lo1["TENS"]["metadata"]["key"] = "value";
        lo1["TENS"]["metadata"]["answer"] = 42;
        lo1["TENS"]["tensors"][0]["metadata"]["extra"] = "hello world";
        msg1.set_label_object(lo1);
    }

    return Zio::unpack(msg1);
}


void middleman(zio::socket_t& link)
{
    link.send(msg_ready, zio::send_flags::none); // ready

    zio::Node node("middleman");
    auto iport = node.port("take", ZMQ_SERVER);
    auto oport = node.port("give", ZMQ_SERVER);
    iport->bind("inproc://test_zioflow_take");
    oport->bind("inproc://test_zioflow_give");
    zio::flow::Flow iflow(iport), oflow(oport);
    node.online();

    zio::poller_t<> poller;
    poller.add(iport->socket(), zio::event_flags::pollin);
    poller.add(oport->socket(), zio::event_flags::pollin);
    poller.add(link, zio::event_flags::pollin);


    
    {   // say hi to input
        zio::Message bot;
        iflow.recv_bot(bot);
        auto fobj = bot.label_object();
        fobj["direction"] = "inject";
        iflow.send_bot(bot);
    }

    {   // say hi to output
        zio::Message bot;
        oflow.recv_bot(bot);
        auto fobj = bot.label_object();
        fobj["direction"] = "extract";
        oflow.send_bot(bot);
    }

    const auto wait = std::chrono::milliseconds{1000};

    std::deque<zio::Message> queued;
    while (true) {
        std::vector<zio::poller_event<>> events(3);
        int nevents = poller.wait_all(events, wait);
        for (int iev=0; iev<nevents; ++iev) {
            if (events[iev].socket == iport->socket()) {
                zio::Message dat;
                bool ok = iflow.get(dat);
                if (!ok) return;
                queued.emplace_back(std::move(dat));
                zio::info("middleman: queued take");
                continue;
            }
            if (events[iev].socket == oport->socket()) {
                while (queued.size()) {
                    zio::Message dat(std::move(queued.front()));
                    queued.pop_front();
                    bool ok = oflow.put(dat);
                    zio::info("middleman: give");
                    if (!ok) return;
                }
                continue;
            }
            // it's the link
            link.send(msg_done, zio::send_flags::none);
            return;
        }
    }
    link.send(msg_done, zio::send_flags::none);    
}

void giver(zio::socket_t& link)
{
    link.send(msg_ready, zio::send_flags::none);

    Zio::TensorSetSink sink;
    auto cfg = sink.default_configuration();
    cfg["verbose"] = 0;
    cfg["nodename"] = "giver";
    cfg["portname"] = "give";
    cfg["connects"][0]["nodename"] = "middleman";
    cfg["connects"][0]["portname"] = "take";
    sink.configure(cfg);

    for (int ind=0; ind<10; ++ind) {
        auto ts = make_tensor_set<int>();
        bool ok = sink(ts);
        if (!ok) {
            zio::info("giver: failed to give");
            break;
        }

        zio::message_t msg;     // check for term
        auto res = link.recv(msg, zio::recv_flags::dontwait);
        if (res) {
            zio::info("giver: link hit");
            break;
        }
    }

    link.send(msg_done, zio::send_flags::none);    
}

void taker(zio::socket_t& link)
{
    link.send(msg_ready, zio::send_flags::none);

    Zio::TensorSetSource src;
    auto cfg = src.default_configuration();
    cfg["verbose"] = 0;
    cfg["nodename"] = "taker";
    cfg["portname"] = "take";
    cfg["connects"][0]["nodename"] = "middleman";
    cfg["connects"][0]["portname"] = "give";
    src.configure(cfg);

    while (true) {
        ITensorSet::pointer ts;
        bool ok = src(ts);
        if (!ok) {
            zio::info("taker: failed to take");
            break;
        }
        if (!ts) {
            zio::info("taker: EOS");
            break;
        }

        zio::message_t msg;     // check for term
        auto res = link.recv(msg, zio::recv_flags::dontwait);
        if (res) {
            zio::info("taker: link hit");
            break;
        }
    }

    link.send(msg_done, zio::send_flags::none);    
}

int main()
{
    zio::init_all();
    zio::context_t ctx;
    zio::zactor_t mm(ctx, middleman);
    zio::zactor_t g(ctx, giver);
    zio::zactor_t t(ctx, taker);

    zio::message_t msg;
    {
        auto res = g.link().recv(msg, zio::recv_flags::none);
        Assert(res);
    }
    {
        auto res = t.link().recv(msg, zio::recv_flags::none);
        Assert(res);
    }
    {
        auto sres = mm.link().send(msg_term, zio::send_flags::none);
        Assert(sres);
        auto rres = mm.link().recv(msg_term, zio::recv_flags::none);
        Assert(rres);
    }

    return 0;
}
      
