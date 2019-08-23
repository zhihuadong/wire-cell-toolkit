#include "WireCellGen/DepoSplat.h"
#include "WireCellIface/SimpleTrace.h"

#include "WireCellUtil/Binning.h"
#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(DepoSplat, WireCell::Gen::DepoSplat,
                 WireCell::IDuctor, WireCell::IConfigurable)




using namespace WireCell;

Gen::DepoSplat::DepoSplat()
    : Ductor()
{
}

Gen::DepoSplat::~DepoSplat()
{
}

                                  
ITrace::vector Gen::DepoSplat::process_face(IAnodeFace::pointer face,
                                            const IDepo::vector& depos)

{
    // channel-charge map
    std::unordered_map<int, std::vector<float> > chch;

    // tick-edged bins
    Binning tbins(m_readout_time/m_tick, m_start_time,
                  m_start_time+m_readout_time);

    int iplane = -1;
    for (auto plane : face->planes()) {
        ++iplane;

        const Pimpos* pimpos = plane->pimpos();

        // wire-centered pitch bins
        const Binning& wbins = pimpos->region_binning();
        

        auto& wires = plane->wires();

        // std::cerr << "splat: plane " << plane->planeid() << " "
        //           << "wbins:" << wbins << " "
        //           << "tbins:" << tbins << " "
        //           << "#wires:" << wires.size() << " "
        //           << "#depos:" << depos.size() << "\n";

        int idepo = 0;
        for (auto depo : depos) {

            const double pwid = m_nsigma * depo->extent_tran();
            const double pcen = pimpos->distance(depo->pos());

            const double twid = m_nsigma*depo->extent_long();
            const double tcen = depo->time();

            const int pbeg = std::max(wbins.bin(pcen-pwid), 0);
            const int pend = std::min(wbins.bin(pcen+pwid)+1, (int)wires.size());
            const int tbeg = tbins.bin(tcen-twid); // fixme what limits
            const int tend = tbins.bin(tcen+twid)+1; //  to enforce here?
            
            // if (idepo == 0) {
            //     std::cerr << "splat: depo=" << depo->pos()/units::mm << "mm "
            //               << "@" << depo->time()/units::ms<< " ms "
            //               << "p=(" << pcen << "+-" << pwid << "), t=(" << tcen << "+=" << twid << ") "
            //               << "pi=[" << pbeg << " " << pend << "], ti=[" << tbeg << " " << tend << "]\n";
            // }

            for (int ip = pbeg; ip < pend; ++ip) {
                auto iwire = wires[ip];
                auto& charge = chch[iwire->channel()];
                if ((int)charge.size() < tend) {
                    charge.resize(tend, 0.0);
                }
                for (int it = tbeg; it < tend; ++it) {
                    charge[it] += std::abs(depo->charge());
                }
            }
            ++idepo;
        }
    }

    // make output traces
    ITrace::vector traces;
    for (auto& chchit : chch) {
        const int chid = chchit.first;
        auto& chv = chchit.second;
        auto trace = std::make_shared<SimpleTrace>(chid, 0, chv);
        traces.push_back(trace);
    }
    return traces;
}
