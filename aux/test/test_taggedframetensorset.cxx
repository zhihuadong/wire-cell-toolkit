#include "WireCellAux/TaggedFrameTensorSet.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellUtil/Testing.h"

using namespace std;
using namespace WireCell;

int main()
{
    const double tick = 0.5*units::us;
    // "Signal"
    const int signal_frame_ident = 100;
    const double signal_start_time = 6*units::ms;
    vector<float> signal{0.0,1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,0.0};

    ITrace::vector signal_traces{
        make_shared<SimpleTrace>(1, 10, signal),
        make_shared<SimpleTrace>(1, 20, signal),
        make_shared<SimpleTrace>(2, 30, signal),
        make_shared<SimpleTrace>(2, 33, signal)};
    auto f1 = make_shared<SimpleFrame>(signal_frame_ident,
                                       signal_start_time,
                                       signal_traces,
                                       tick);

    ITensorSet::pointer out=nullptr;
    Aux::TaggedFrameTensorSet tfts;
    auto cfg = tfts.default_configuration();
    cfg["tensors"][0]["tag"] = "";
    tfts.configure(cfg);
    bool ok = tfts(f1, out);
    Assert(ok);
    Assert(out);

    Assert(out->ident() == signal_frame_ident);
    auto tens = out->tensors();
    Assert(tens);
    Assert(tens->size() == 1);
    auto ten = tens->at(0);
    Assert(ten);
    auto shape = ten->shape();
    Assert(shape.size() == 2);
    cout << shape[0] <<  " " << shape[1] << endl;
    Assert(shape[0] == 2);
    
    auto md = out->metadata();
    cout << "metadata: " << md << endl;
    Assert(get(md,"time",0.0) == signal_start_time);
    auto jtens = md["tensors"];
    Assert(jtens.size() == 1);
    auto jten = jtens[0];
    Assert(get(jten, "tbin", -1) == 10);
    auto jch = jten["channels"];
    Assert(jch.size() == 2);
    Assert(jch[0].asInt() == 1);
    Assert(jch[1].asInt() == 2);

    return 0;
}
