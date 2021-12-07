/* Comentary:

   This class is one element in a 2 by 2 matrix of classes which start
   very symmetric.

   (Depo, Impact) outer-product (Zipper, Transform)

   The "Depo" class is high level and uses the low level "Impact" class.

   The "Transform" version is newer than the "Zipper" version and
   while it started with a symmetric interface, it must grow out of
   it.

   The "Transform" version is faster than the "Zipper" for most
   realistically complicated event topology.  It will be made faster
   yet but this will break the symmetry:

   1) The "Tranform" version builds a dense array so sending its
   output through the narrow waveform() is silly.

   2) It can be optimized if the array has a channel basis instead of
   a wire one so the "zipper" interface is not fitting.

   3) It may be further optimized by performing its transforms on two
   planes from each face (for two-faced APAs) simultaneously.  This
   requires a different intrerface, possibly by (ab)using
   BinnedDiffusion in a different way than "zipper".

   4) It may be optimized yet further by breaking up the FFTs to do
   smaller ones (in time) on charge (X) FR and then doing the larger
   ones over ER(X)RC(X)RC.  This work also requires extension of PIR.
   This improvement may also be applicable to zipper.

   All of this long winded explantion, which nobody will ever read, is
   "justification" for the otherwise disgusting copy-paste which is
   going on with these two pairs of classes.

 */

#include "WireCellGen/DepoTransform.h"
#include "WireCellGen/ImpactTransform.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"
#include "WireCellGen/BinnedDiffusion_transform.h"
#include "WireCellUtil/Units.h"
#include "WireCellUtil/Point.h"

#include <omp.h>

WIRECELL_FACTORY(DepoTransform, WireCell::Gen::DepoTransform, WireCell::IDepoFramer, WireCell::IConfigurable)

using namespace WireCell;
using namespace std;

Gen::DepoTransform::DepoTransform()
  : m_start_time(0.0 * units::ns)
  , m_readout_time(5.0 * units::ms)
  , m_tick(0.5 * units::us)
  , m_drift_speed(1.0 * units::mm / units::us)
  , m_nsigma(3.0)
  , m_frame_count(0)
  , l(Log::logger("sim"))
{
}

Gen::DepoTransform::~DepoTransform() {}

void Gen::DepoTransform::configure(const WireCell::Configuration& cfg)
{
    auto anode_tn = get<string>(cfg, "anode", "");
    m_anode = Factory::find_tn<IAnodePlane>(anode_tn);

    m_nsigma = get<double>(cfg, "nsigma", m_nsigma);
    bool fluctuate = get<bool>(cfg, "fluctuate", false);
    m_rng = nullptr;
    if (fluctuate) {
        auto rng_tn = get<string>(cfg, "rng", "");
        m_rng = Factory::find_tn<IRandom>(rng_tn);
    }

    m_readout_time = get<double>(cfg, "readout_time", m_readout_time);
    m_tick = get<double>(cfg, "tick", m_tick);
    m_start_time = get<double>(cfg, "start_time", m_start_time);
    m_drift_speed = get<double>(cfg, "drift_speed", m_drift_speed);
    m_frame_count = get<int>(cfg, "first_frame_number", m_frame_count);

    auto jpirs = cfg["pirs"];
    if (jpirs.isNull() or jpirs.empty()) {
        std::string msg = "must configure with some plane impact response components";
        l->error(msg);
        THROW(ValueError() << errmsg{"Gen::Ductor: " + msg});
    }
    m_pirs.clear();
    for (auto jpir : jpirs) {
        auto tn = jpir.asString();
        auto pir = Factory::find_tn<IPlaneImpactResponse>(tn);
        m_pirs.push_back(pir);
    }
}
WireCell::Configuration Gen::DepoTransform::default_configuration() const
{
    Configuration cfg;

    /// How many Gaussian sigma due to diffusion to keep before truncating.
    put(cfg, "nsigma", m_nsigma);

    /// Whether to fluctuate the final Gaussian deposition.
    put(cfg, "fluctuate", false);

    /// The open a gate.  This is actually a "readin" time measured at
    /// the input ("reference") plane.
    put(cfg, "start_time", m_start_time);

    /// The time span for each readout.  This is actually a "readin"
    /// time span measured at the input ("reference") plane.
    put(cfg, "readout_time", m_readout_time);

    /// The sample period
    put(cfg, "tick", m_tick);

    /// The nominal speed of drifting electrons
    put(cfg, "drift_speed", m_drift_speed);

    /// Allow for a custom starting frame number
    put(cfg, "first_frame_number", m_frame_count);

    /// Name of component providing the anode plane.
    put(cfg, "anode", "");
    /// Name of component providing the anode pseudo random number generator.
    put(cfg, "rng", "");

    /// Plane impact responses
    cfg["pirs"] = Json::arrayValue;

    return cfg;
}

bool Gen::DepoTransform::operator()(const input_pointer& in, output_pointer& out)
{
    if (!in) {
        out = nullptr;
        return true;
    }

    auto t00 = omp_get_wtime() ;
    auto depos = in->depos();

    Binning tbins(m_readout_time / m_tick, m_start_time, m_start_time + m_readout_time);
    ITrace::vector traces;
    for (auto face : m_anode->faces()) {
    auto t0 = omp_get_wtime() ;
        // Select the depos which are in this face's sensitive volume
        IDepo::vector face_depos, dropped_depos;
        auto bb = face->sensitive();
        if (bb.empty()) {
            l->debug("anode {} face {} is marked insensitive, skipping", m_anode->ident(), face->ident());
            continue;
        }

        for (auto depo : (*depos)) {
            if (bb.inside(depo->pos())) {
                face_depos.push_back(depo);
            }
            else {
                dropped_depos.push_back(depo);
            }
        }

        if (face_depos.size()) {
            auto ray = bb.bounds();
            l->debug(
                "anode: {}, face: {}, processing {} depos spanning "
                "t:[{},{}]ms, bb:[{}-->{}]cm",
                m_anode->ident(), face->ident(), face_depos.size(), face_depos.front()->time() / units::ms,
                face_depos.back()->time() / units::ms, ray.first / units::cm, ray.second / units::cm);
        }
        if (dropped_depos.size()) {
            auto ray = bb.bounds();
            l->debug(
                "anode: {}, face: {}, dropped {} depos spanning "
                "t:[{},{}]ms, outside bb:[{}-->{}]cm",
                m_anode->ident(), face->ident(), dropped_depos.size(), dropped_depos.front()->time() / units::ms,
                dropped_depos.back()->time() / units::ms, ray.first / units::cm, ray.second / units::cm);
        }

    auto t1 =omp_get_wtime();
        int iplane = -1;
        for (auto plane : face->planes()) {
    auto t2 =omp_get_wtime();
            ++iplane;

            const Pimpos* pimpos = plane->pimpos();

            Binning tbins(m_readout_time / m_tick, m_start_time, m_start_time + m_readout_time);

            Gen::BinnedDiffusion_transform bindiff(*pimpos, tbins, m_nsigma, m_rng);
            for (auto depo : face_depos) {
                depo = modify_depo(plane->planeid(), depo);
                bindiff.add(depo, depo->extent_long() / m_drift_speed, depo->extent_tran());
            }

            auto& wires = plane->wires();

            auto pir = m_pirs.at(iplane);
    auto t3 =omp_get_wtime();
            Gen::ImpactTransform transform(pir, bindiff);

            const int nwires = pimpos->region_binning().nbins();
    auto t4 =omp_get_wtime();
            for (int iwire = 0; iwire < nwires; ++iwire) {
                auto wave = transform.waveform(iwire);

                auto mm = Waveform::edge(wave);
                if (mm.first == (int) wave.size()) {  // all zero
                    continue;
                }

                int chid = wires[iwire]->channel();
                int tbin = mm.first;

                ITrace::ChargeSequence charge(wave.begin() + mm.first, wave.begin() + mm.second);
                auto trace = make_shared<SimpleTrace>(chid, tbin, charge);
                traces.push_back(trace);
            }
    auto t5 =omp_get_wtime();
    std::cout<<"Timing-DepoTransForm_p1 "<< t1-t0 <<std::endl ;
    std::cout<<"Timing-DepoTransForm_BD "<< t3-t2 <<std::endl ;
    std::cout<<"Timing-DepoTransForm_Transform_Matrix "<< t4-t3 <<std::endl ;
    std::cout<<"Timing-DepoTransForm_NwireLoop "<< t5-t4 <<std::endl ;
        }
    }

    auto frame = make_shared<SimpleFrame>(m_frame_count, m_start_time, traces, m_tick);
    ++m_frame_count;
    out = frame;
    auto t6 = omp_get_wtime();
    std::cout<<"Timing-DepoTransForm_Operator_total " <<t6-t00 <<std::endl ;
    return true;
}
