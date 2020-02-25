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
    const int nchans = shape[0];
    const int nticks = shape[1];
    
    auto md = out->metadata();
    cout << "metadata: " << md << endl;
    Assert(get(md,"time",0.0) == signal_start_time);
    Assert(get(md,"tick",0.0) == tick);
    auto jtens = md["tensors"];
    Assert(jtens.size() == 1);
    auto jten = jtens[0];
    int tbin = get(jten, "tbin", -1);
    Assert(tbin == 10);
    auto jch = jten["channels"];
    Assert(nchans == (int)jch.size()); // JsonCPP type unsafety is annoying!
    Assert(jch[0].asInt() == 1);       // arrays must be int, not size_t, but size is size_t!
    Assert(jch[1].asInt() == 2);

    Eigen::Map<Eigen::ArrayXXf> arr((float*)ten->data(), nchans, nticks);
    std::cout << "channels:\ntick\t"; 
    for (int ichan=0; ichan<nchans; ++ichan) {
        std::cout << jch[ichan].asInt() << "\t";
    }
    std::cout << endl;

    for (int itick=0; itick<nticks; ++itick) {
        std::cout << tbin+itick;
        for (int ichan=0; ichan<nchans; ++ichan) {
            std::cout << "\t" << arr(ichan, itick);
        }
        std::cout << endl;
    }


    return 0;
}
