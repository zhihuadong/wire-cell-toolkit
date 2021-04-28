#include "WireCellSio/NumpyDepoLoader.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Array.h"
#include "WireCellUtil/Logging.h"
#include "WireCellUtil/Point.h"
#include "WireCellUtil/cnpy.h"
#include "WireCellIface/SimpleDepo.h"

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <tuple>

WIRECELL_FACTORY(NumpyDepoLoader, WireCell::Sio::NumpyDepoLoader,
                 WireCell::IDepoSource, WireCell::IConfigurable)

using namespace WireCell;

Sio::NumpyDepoLoader::NumpyDepoLoader()
  : m_load_count(0)
  , m_eos(1)
{
}

Sio::NumpyDepoLoader::~NumpyDepoLoader() {}

WireCell::Configuration Sio::NumpyDepoLoader::default_configuration() const
{
    Configuration cfg;

    // The input file name to read. 
    cfg["filename"] = "wct-frame.npz";

    return cfg;
}

void Sio::NumpyDepoLoader::configure(const WireCell::Configuration& config)
{
    m_cfg = config;
}



using DataArray = Eigen::Array<float, Eigen::Dynamic, 7>;
using InfoArray = Eigen::Array<int, Eigen::Dynamic, 4>;


bool Sio::NumpyDepoLoader::next()
{
    auto log = Log::logger("sio");

    ++m_load_count;
        
    // match names used by NDS
    const std::string data_name = String::format("depo_data_%d", m_load_count-1);
    const std::string info_name = String::format("depo_info_%d", m_load_count-1);

    const std::string fname = m_cfg["filename"].asString();
    cnpy::NpyArray data_arr, info_arr;
    try {
        data_arr = cnpy::npz_load(fname, data_name);
        info_arr = cnpy::npz_load(fname, info_name);
    }
    catch (std::runtime_error) {
        log->debug("NumpyDepoLoader: {}:{}: no such array",
                   fname, data_name);
        return false;
    }

    // In [4]: f['depo_data_0'].shape
    // Out[4]: (7, 1785)

    if (data_arr.shape[0] != 7) {
        log->warn("NumpyDepoLoader: {}:{}: depo data is not size 7",
                  fname, data_name);
        return false;
    }
    if (info_arr.shape[0] != 4) {
        log->warn("NumpyDepoLoader: {}:{}: depo info is not size 4",
                  fname, info_name);
        return false;
    }

    const auto ndatas = data_arr.shape[1];
    const auto ninfos = info_arr.shape[1];
    if (ndatas != ninfos) {
        log->warn("NumpyDepoLoader: {}: mismatch ndepo={} ninfo={}",
                  fname, ndatas, ninfos);
        return false;
    }
    const auto ndepos = ndatas;
    log->debug("load {} depos from frame {}", ndepos, data_name);

    // data: time, charge, x, y, z, dlong, dtran
    // info: ID, pdg, gen, child

    // Get as eigen arrays
    auto data = Eigen::Map<DataArray>(data_arr.data<float>(),
                                      ndepos, 7);
    auto info = Eigen::Map<InfoArray>(info_arr.data<int>(),
                                      ndepos, 4);
    assert(data.cols() == 7); // make sure we understand
    assert(info.cols() == 4); // how to call eigen::map!

    std::vector<SimpleDepo*> sdepos;
    for (size_t ind=0; ind < ndepos; ++ind) {

        // log->debug("dump: {}: t={} q={} x={} y={} z={}", ind,
        //            data(ind, 0), data(ind, 1),
        //            data(ind, 2), data(ind, 3), data(ind, 4));

        auto sdepo = new SimpleDepo(
            data(ind, 0),        // t
            Point(data(ind, 2),  // x
                  data(ind, 3),  // y
                  data(ind, 4)), // z
            data(ind, 1),        // q
            nullptr,             // prior
            data(ind, 5),        // extent_long
            data(ind, 6),        // extent_tran
            info(ind, 0),        // id
            info(ind, 1));       // pdg

        const auto gen = info(ind, 2);
        if (gen > 0) {
            // this depo is a prior
            const size_t other = info(ind, 3);
            if (other >= sdepos.size()) {
                log->warn("NumpyDepoLoader: {}: depo ordering corrupt at {}: {} > {}",
                          fname, ind, other, sdepos.size());
                return false;
            }
        
            auto idepo = IDepo::pointer(sdepo);
            sdepo = nullptr;
            sdepos[other]->set_prior(idepo);
        }
        // We save both gen=0 and gen>0 as nullptrs to preserve indexing
        sdepos.push_back(sdepo);
    }
    
    for (auto sdepo: sdepos) {
        if (sdepo) {
            auto idepo = IDepo::pointer(sdepo);
            sdepo = nullptr;
            m_depos.push_back(idepo);
        }
    }
    return true;
}

bool Sio::NumpyDepoLoader::operator()(WireCell::IDepo::pointer& outdepo)
{
    outdepo = nullptr;
    if (m_depos.empty()) {
        if (m_eos == 0) {
            m_eos = 1;          // we just finished a stream
            return true;        // so send null outdepo -> EOS
        }
        // We are outside a stream, try to initiate a new one
        bool ok = next();
        if (!ok or m_depos.empty()) { // failed to initiate
            m_depos.clear();
            return false;       // we've gone off the rails
        }
        m_eos = 0;              // we are in a working stream
    }

    outdepo = m_depos.front();
    m_depos.pop_front();
    auto log = Log::logger("sio");
    if (!outdepo) {
        log->debug("depo loader got no depo");
    }
    // else {
    //     log->debug("depo loader: t={} q={} x={}",
    //                outdepo->time(), outdepo->charge(), outdepo->pos().x());
    // }
    return true;
}
