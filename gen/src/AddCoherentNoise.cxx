#include "WireCellGen/AddCoherentNoise.h"

#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"

#include "Noise.h"

#include <iostream>

WIRECELL_FACTORY(AddCoherentNoise, WireCell::Gen::AddCoherentNoise,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

Gen::AddCoherentNoise::AddCoherentNoise()
    : m_nsamples(9600)
    , log(Log::logger("sim"))
{
}


Gen::AddCoherentNoise::~AddCoherentNoise()
{
}

WireCell::Configuration Gen::AddCoherentNoise::default_configuration() const
{
    Configuration cfg;

    cfg["nsamples"] = m_nsamples;
    return cfg;
}

void Gen::AddCoherentNoise::configure(const WireCell::Configuration& cfg)
{
    m_nsamples = get<int>(cfg,"nsamples",m_nsamples);

    log->debug("AddCoherentNoise: samples: \"{}\"",
               m_nsamples);
}



bool Gen::AddCoherentNoise::operator()(const input_pointer& inframe, output_pointer& outframe)
{
    if (!inframe) {
        outframe = nullptr;
        return true;
    }

    // Does nothing for test purposes
    ITrace::vector outtraces;
    for (const auto& intrace : *inframe->traces()) {
        int chid = intrace->channel();
        Waveform::realseq_t wave = {0};

	       wave.resize(m_nsamples,0);
	       Waveform::increase(wave, intrace->charge());
        auto trace = make_shared<SimpleTrace>(chid, intrace->tbin(), wave);
        outtraces.push_back(trace);
    }
    outframe = make_shared<SimpleFrame>(inframe->ident(), inframe->time(),
                                        outtraces, inframe->tick());
    return true;
}
