/*! test TaggedFrameTensorSet
 *
 * Also provides an example of how to "hook" an Eigen array to 2D
 * tensor data provided by an ITensor.
 */

#include "WireCellAux/TaggedFrameTensorSet.h"
#include "WireCellAux/TaggedTensorSetFrame.h"
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
    auto iframe1 = make_shared<SimpleFrame>(signal_frame_ident,
                                            signal_start_time,
                                            signal_traces,
                                            tick);

    ITensorSet::pointer itensorset=nullptr;
    Aux::TaggedFrameTensorSet tfts;
    auto cfg = tfts.default_configuration();
    cfg["tensors"][0]["tag"] = "";
    tfts.configure(cfg);
    bool ok = tfts(iframe1, itensorset);
    Assert(ok);
    Assert(itensorset);

    Assert(itensorset->ident() == signal_frame_ident);
    auto tens = itensorset->tensors();
    Assert(tens);

    auto md = itensorset->metadata();
    Assert(tens->size() == 2);  // waveform+channels

    cout << "metadata: " << md << endl;
    Assert(get(md,"time",0.0) == signal_start_time);
    Assert(get(md,"tick",0.0) == tick);
    auto jtens = md["tensors"];
    Assert(jtens.size() == 1);

    for (auto jten : jtens) {
        int iwf = jten["waveform"].asInt();
        auto wft = tens->at(iwf);
        Assert(wft);
        int ich = jten["channels"].asInt();
        auto cht = tens->at(ich);
        Assert(cht);
        Assert(jten["summary"].isNull());
        
        auto wf_shape = wft->shape();
        Assert(wf_shape.size() == 2);
        cout << wf_shape[0] <<  " " << wf_shape[1] << endl;
        const int nchans = wf_shape[0];
        const int nticks = wf_shape[1];
        Assert(nchans == 2);
    
        int tbin = get(jten, "tbin", -1);
        Assert(tbin == 10);

        auto ch_shape = cht->shape();
        Assert(ch_shape.size() == 1);
        Assert(nchans == (int)ch_shape[0]);
        Eigen::Map<Eigen::ArrayXi> ch_arr((int*)cht->data(), nchans);
        Assert(ch_arr[0] == 1);
        Assert(ch_arr[1] == 2);

        Eigen::Map<Eigen::ArrayXXf> wf_arr((float*)wft->data(), nchans, nticks);
        std::cout << "channels:\ntick\t"; 
        for (int ichan=0; ichan<nchans; ++ichan) {
            std::cout << ch_arr[ichan] << "\t";
        }
        std::cout << endl;
        
        for (int itick=0; itick<nticks; ++itick) {
            std::cout << tbin+itick;
            for (int ichan=0; ichan<nchans; ++ichan) {
                std::cout << "\t" << wf_arr(ichan, itick);
            }
            std::cout << endl;
        }

    }

    //
    // Now convert back to an IFrame
    //
    
    Aux::TaggedTensorSetFrame ttsf;
    cfg = ttsf.default_configuration();
    cfg["tensors"][0]["tag"] = "";
    ttsf.configure(cfg);

    IFrame::pointer iframe2;
    ok = ttsf(itensorset, iframe2);
    Assert(ok);
    Assert(iframe2);
    {
        auto tt = iframe2->trace_tags();
        Assert(tt.size() == 1);
        Assert(tt[0] == "");
    }
    size_t nticks = 0;
    const size_t nchans = 2;
    {
        auto traces = iframe2->traces();
        Assert(traces->size() == nchans);
        Assert(traces->at(0)->channel() == 1);
        nticks = traces->at(0)->charge().size();
        Assert(traces->at(1)->channel() == 2);
        Assert(nticks == traces->at(1)->charge().size());
    }
    {
        auto tt = iframe2->tagged_traces("");
        Assert(tt.size() == nchans);
    }

    {
        auto traces = iframe2->traces();

        std::cout << "channels:\ntick\t"; 
        for (size_t ichan=0; ichan<nchans; ++ichan) {
            std::cout << traces->at(ichan)->channel() << "\t";
        }
        std::cout << endl;
        Assert(nticks == 34);
        for (size_t itick=0; itick<nticks; ++itick) {
            for (size_t ich = 0; ich < nchans; ++ich) {
                const auto& tr = traces->at(ich);
                auto tbin = tr->tbin();
                std::cout << tbin+itick
                          << ":" << tr->charge()[itick] << " ";
            }
            std::cout << endl;
        }
    }
    return 0;
}
