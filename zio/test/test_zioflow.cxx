/// Test tensor -> flow -> tensor 

#include "WireCellZio/TensorSetSink.h"
#include "WireCellZio/TensorSetSource.h"
#include "WireCellZio/TensUtil.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Logging.h"

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
    zio::debug ("make tensor set {}{}", zio::tens::type_name(typeid(TYPE)), sizeof(TYPE));

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
    node.set_verbose(false);
    auto iport = node.port("takes", ZMQ_SERVER);
    auto oport = node.port("gives", ZMQ_SERVER);
    // server/client can't to inproc, "@" is "abstract namespace" and Linux-only
    iport->bind("ipc://@test_zioflow_take");
    oport->bind("ipc://@test_zioflow_give");
    zio::flow::Flow iflow(iport), oflow(oport);
    node.online();

    zio::debug("[middleman]: online");

    zio::poller_t<> poller;
    poller.add(iport->socket(), zio::event_flags::pollin);
    poller.add(oport->socket(), zio::event_flags::pollin);
    poller.add(link, zio::event_flags::pollin);

    
    zio::debug("[middleman]: recv bot from giver"); 
    {   // say hi to input
        zio::Message bot;
        iflow.recv_bot(bot);
        auto fobj = bot.label_object();
        fobj["direction"] = "inject";
        iflow.send_bot(bot);
    }
    iflow.flush_pay();

    zio::debug("[middleman]: recv bot from taker"); 
    {   // say hi to output
        zio::Message bot;
        oflow.recv_bot(bot);
        auto fobj = bot.label_object();
        fobj["direction"] = "extract";
        oflow.send_bot(bot);
    }

    const auto wait = std::chrono::milliseconds{1000};

    zio::debug("[middleman]: starting loop"); 

    while (true) {
        std::vector<zio::poller_event<>> events(3);
        int nevents = poller.wait_all(events, wait);
        for (int iev=0; iev<nevents; ++iev) {
            if (events[iev].socket == iport->socket()) {
                zio::Message dat;
                if (! iflow.get(dat)) {
                    zio::error("[middleman]: take interupted");
                    iflow.send_eot(dat);
                    goto bail;
                }

                zio::info("[middleman]: #{}:\n{}",
                          dat.seqno(), dat.label());

                if (! oflow.put(dat)) {
                    zio::error("[middleman]: give interupted");
                    oflow.send_eot(dat);
                }
                continue;
            }

            // it's the link telling us to quit, we initiate end of
            // transmission
            {
                zio::Message eot;
                iflow.send_eot(eot);
                oflow.send_eot(eot);
                oflow.recv_eot(eot);
                iflow.recv_eot(eot);
            }
            goto bail;
        }
    }
  bail:
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
    cfg["connects"][0]["portname"] = "takes";
    sink.configure(cfg);

    for (int ind=0; ind<10; ++ind) {
        zio::info("[giver]: pretending work");
        sleep(1);
        auto ts = make_tensor_set<int>();
        bool ok = sink(ts);
        if (!ok) {
            zio::info("[giver]: failed to give");
            break;
        }

        zio::message_t msg;     // check for term
        auto res = link.recv(msg, zio::recv_flags::dontwait);
        if (res) {
            zio::info("[giver]: link hit");
            break;
        }
    }
    zio::info("[giver]: send EOS");
    sink(nullptr);
    zio::info("[giver]: send EOT");
    sink(nullptr);

    zio::info("[giver]: sending actor done message");
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
    cfg["connects"][0]["portname"] = "gives";
    src.configure(cfg);

    while (true) {
        ITensorSet::pointer ts;
        bool ok = src(ts);
        if (!ok) {
            zio::info("[taker]: failed to take");
            break;
        }
        if (!ts) {
            zio::info("[taker]: EOS");
            break;
        }

        zio::message_t msg;     // check for term
        auto res = link.recv(msg, zio::recv_flags::dontwait);
        if (res) {
            zio::info("[taker]: link hit");
            break;
        }
    }

    zio::info("[taker]: sending actor done message");
    link.send(msg_done, zio::send_flags::none);    
}

int main()
{
    Log::add_stdout(true, "debug");
    Log::set_level("debug");
    auto log = Log::logger("test");
    log->info("Started WCT logging");
    zio::init_all();
    zio::info("Started ZIO logging");

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
      
