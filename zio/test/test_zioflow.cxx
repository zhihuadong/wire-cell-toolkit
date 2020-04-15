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
    const int cred = 10;
    zio::timeout_t timeout(5000);

    link.send(msg_ready, zio::send_flags::none); // ready

    zio::Node node("middleman");
    node.set_verbose(false);
    auto iport = node.port("takes", ZMQ_SERVER);
    auto oport = node.port("gives", ZMQ_SERVER);
    // server/client can't to inproc, "@" is "abstract namespace" and Linux-only
    iport->bind();
    oport->bind();
    zio::Flow iflow(iport, zio::flow::direction_e::inject, cred, timeout);
    zio::Flow oflow(oport, zio::flow::direction_e::extract, cred, timeout);
    node.online();

    zio::debug("[middleman]: online");

    zio::poller_t<> poller;
    poller.add(iport->socket(), zio::event_flags::pollin);
    poller.add(oport->socket(), zio::event_flags::pollin);
    poller.add(link, zio::event_flags::pollin);

    bool noto; // no timeout

    zio::debug("[middleman]: bot handshake with {}", iport->name()); 
    noto = iflow.bot();    // let EOT exception past
    if (!noto) {
        zio::debug("[middleman]: bot handshake timeout with {}", iport->name());
        throw(std::runtime_error("unexpected bot handshake timeout with " + iport->name()));
    }

    zio::debug("[middleman]: bot handshake with {}", oport->name()); 
    noto = oflow.bot();    // let EOT exception past
    if (!noto) {
        zio::debug("[middleman]: bot handshake timeout with {}", oport->name());
        throw(std::runtime_error("unexpected bot handshake timeout with " + oport->name()));
    }

    iflow.pay();                // flushes pay to giver client
    oflow.pay();                // income pay from taker client, if any

    // a short max wait on flow ports.  This makes the loop into a
    // slow spin.  Efectively ping-ponging between iport/oport.  A
    // more proper way is to put both in a common poll.
    const auto wait = std::chrono::milliseconds{100};

    zio::debug("[middleman]: starting loop with timeout {}", wait.count()); 

    std::vector<zio::poller_event<>> events(3);

    bool keep_going = true;
    while (keep_going) {

        int nhits = poller.wait_all(events, wait);

        for (int ihit=0; ihit < nhits; ++ihit) {
            if (events[ihit].socket == link) {
                zio::info("[middleman]: link hit");
                iflow.eot();
                oflow.eot();
                keep_going = false;
                break;
            }

            if (events[ihit].socket == oport->socket()) {

                zio::Message msg;
                bool noto = oflow.recv(msg); // doesn't throw on EOT
                assert(noto);                // poll assures
                zio::debug("[middleman]: oport hit {}", msg.label());
                
                const zio::flow::Label lab(msg);
                if (lab.msgtype() == zio::flow::msgtype_e::eot) {
                    oflow.eotack(msg);
                    iflow.eot(msg);
                    keep_going = false;
                    break;
                }
                // otherwise, this must be pay, which should be
                // processed correctly by recv().
                assert(lab.msgtype() == zio::flow::msgtype_e::pay);

            }

            if (events[ihit].socket == iport->socket()) {

                zio::Message msg;
                bool noto = iflow.recv(msg); // doesn't throw on EOT
                assert(noto);                // poll assures
                zio::debug("[middleman]: iport hit {}", msg.label());
                
                const zio::flow::Label lab(msg);
                if (lab.msgtype() == zio::flow::msgtype_e::eot) {
                    zio::debug("[middleman]: EOT from {}", iport->name());
                    iflow.eotack(msg);
                    oflow.eot(msg);
                    keep_going = false;
                    break;
                }

                if (lab.msgtype() == zio::flow::msgtype_e::dat) {
                    bool noto;
                    try {
                        noto = oflow.put(msg);
                    }
                    catch (zio::flow::end_of_transmission) {
                        zio::debug("[middleman]: EOT from PUT with {}", iport->name());
                        oflow.eotack();
                        iflow.eot();
                        keep_going = false;
                        break;
                    }
                    if (!noto) {
                        zio::debug("[middleman]: timeout from PUT with {}", iport->name());
                        iflow.eot();
                        oflow.eot();
                        keep_going = false;
                        break;
                    }
                    oflow.pay(); // flush pay
                }
            }
        }
    }
    link.send(msg_done, zio::send_flags::none);    
    zio::info("[middleman]: going offline");
    node.offline();
}

void giver(zio::socket_t& link)
{
    link.send(msg_ready, zio::send_flags::none);

    Zio::TensorSetSink sink;
    auto cfg = sink.default_configuration();
    cfg["verbose"] = 0;
    cfg["nodename"] = "giver";
    cfg["portname"] = "givec";
    cfg["timeout"] = 5000;
    cfg["connects"][0]["nodename"] = "middleman";
    cfg["connects"][0]["portname"] = "takes";
    sink.configure(cfg);

    for (int ind=0; ind<10; ++ind) {
        zio::info("[giver]: pretending work");
        zio::sleep_ms(zio::time_unit_t{100});
        auto ts = make_tensor_set<int>();
        assert(ts);
        bool ok = sink(ts);
        if (!ok) {
            zio::info("[giver]: failed to give");
            break;
        }
        zio::info("[giver]: sunk ITensor");

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
    cfg["portname"] = "takec";
    cfg["timeout"] = 5000;
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
        log->info("giver returned");
    }
    {
        auto res = t.link().recv(msg, zio::recv_flags::none);
        Assert(res);
        log->info("taker returned");
    }
    {
        log->info("shutdown middleman");
        auto sres = mm.link().send(msg_term, zio::send_flags::none);
        Assert(sres);
        log->info("wait for middleman");
        auto rres = mm.link().recv(msg_term, zio::recv_flags::none);
        Assert(rres);
        log->info("middleman returned");
    }

    return 0;
}
      
