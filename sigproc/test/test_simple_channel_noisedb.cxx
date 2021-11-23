#include "WireCellUtil/Testing.h"

#include "WireCellSigProc/SimpleChannelNoiseDB.h"

#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"

#include <iostream>

using namespace std;
using namespace WireCell;
using namespace WireCell::SigProc;

int main()
{
    PluginManager& pm = PluginManager::instance();
    pm.add("WireCellAux");
    Factory::lookup_tn<IDFT>("FftwDFT");

    const int nsamples = 5432;
    const double tick = 1.0 * units::ms;

    SimpleChannelNoiseDB cndb(tick, nsamples);
    cndb.configure(cndb.default_configuration());

    Assert(cndb.sample_time() == tick);
    Assert(cndb.nominal_baseline(0) == 0.0);
    Assert(cndb.gain_correction(0) == 1.0);

    auto rcrc0 = cndb.rcrc(0);
    auto config0 = cndb.config(0);
    auto noise0 = cndb.noise(0);

    cerr << rcrc0.size() << endl;

    Assert(rcrc0.size() == nsamples);
    Assert(config0.size() == nsamples);
    Assert(noise0.size() == nsamples);

    cndb.set_gains_shapings({0, 1, 2, 3});

    auto config1 = cndb.config(1);
    cerr << config1.size() << endl;
    Assert(config1.size() == nsamples);

    return 0;
}
