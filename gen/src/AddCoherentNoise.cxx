#include "WireCellGen/AddCoherentNoise.h"

#include "WireCellIface/SimpleTrace.h"
#include "WireCellIface/SimpleFrame.h"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/FFTBestLength.h"

#include "Noise.h"

#include <iostream>

WIRECELL_FACTORY(AddCoherentNoise, WireCell::Gen::AddCoherentNoise,
                 WireCell::IFrameFilter, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;

Gen::AddCoherentNoise::AddCoherentNoise(const std::string& rng)
    : m_rng_tn(rng)
    , m_nsamples(9600)
    , m_period(0.4)
    , log(Log::logger("sim"))
{

  // Need some cleanig? (might be unnecessary)
   coherent_groups.clear();
   phase_groups.clear();
}

Gen::AddCoherentNoise::~AddCoherentNoise()
{
}

std::complex<double> Gen::AddCoherentNoise::multiply( std::complex<double> a, std::complex<double> b)
{
  std::complex<double> c;
  c.real(a.real()*b.real()-a.imag()*b.imag());
  c.imag(a.real()*b.imag()+a.imag()*b.real());
  return c;
}

void Gen::AddCoherentNoise::gen_elec_resp_default()
{
  // calculate the frequencies ...
  m_elec_resp_freq.resize(m_fft_length,0);
  for (unsigned int i=0;i!=m_elec_resp_freq.size();i++)
  {
    if (i<=m_elec_resp_freq.size()/2.){
      m_elec_resp_freq.at(i) = i / (m_elec_resp_freq.size() *1.0) * 1./m_period ; // the second half is useless ...
    }else{
      m_elec_resp_freq.at(i) = (m_elec_resp_freq.size()-i) / (m_elec_resp_freq.size() *1.0) * 1./m_period ; // the second half is useless ...
    }
  }
}

size_t Gen::AddCoherentNoise::argclosest(std::vector<float> const& vec, float value)
{
    // Returns the location in vec of the entry closest to value
    auto const it = std::lower_bound(vec.begin(), vec.end(), value);
    if (it == vec.end()) { return -1; }

    return std::distance(vec.begin(), it);
}


WireCell::Configuration Gen::AddCoherentNoise::default_configuration() const
{
    Configuration cfg;

    cfg["rng"] = m_rng_tn;
    cfg["nsamples"] = m_nsamples;
    cfg["period"] = m_period;
    cfg["normalization"] = m_normalization;


    return cfg;
}

void Gen::AddCoherentNoise::configure(const WireCell::Configuration& cfg)
{
    m_rng_tn = get(cfg, "rng", m_rng_tn);
    m_rng = Factory::find_tn<IRandom>(m_rng_tn);
    m_nsamples = get<int>(cfg,"nsamples",m_nsamples);
    m_period = get<int>(cfg,"period",m_period);
    m_normalization = get<int>(cfg,"normalization",m_normalization);

    log->debug("AddCoherentNoise: using IRandom: \"{}\"", m_rng_tn);
    log->debug("AddCoherentNoise: using m_nsamples: \"{}\"", m_nsamples);
    log->debug("AddCoherentNoise: using m_period: \"{}\"", m_period);

    m_fft_length = fft_best_length(m_nsamples);

    log->debug("AddCoherentNoise: m_fft_length: {}", m_fft_length);

    gen_elec_resp_default();

    log->debug("AddCoherentNoise: m_elec_resp_freq.size(): {}", m_elec_resp_freq.size());

    // Make a map with frequencies and amplitudes associated to a correlation number
    // These quantities have to be imported from somewhere. Just put them hardcoded for today
    std::vector<int> groups = { 64 };

    for(int group : groups)
    {
      std::vector<float> cohfreqs = { 0.01*1./units::us }; //<< in MHz
      std::vector<float> cohampls = { 400.*units::mV }; //<< in mV

      std::vector<float> temp_amplitudes(m_fft_length,0);

      //Find the position
      for (unsigned int j=0;j!=cohfreqs.size();j++)
      {
        const int pos = argclosest( m_elec_resp_freq, cohfreqs.at(j) );

        log->debug("AddCoherentNoise: pos: {}", pos);

        temp_amplitudes.at(pos) = cohampls.at(j);
        temp_amplitudes.at(m_fft_length-pos) = cohampls.at(j); // << Not very elegant
      }

      // Store the temporary amplitudes in the group
      coherent_groups[group] = temp_amplitudes;
    }

    log->debug("AddCoherentNoise: configuration done");

}

std::vector<float> Gen::AddCoherentNoise::get_phase( std::vector<float> ampls, IRandom::pointer rng)
{

  std::vector<float>  phase_groups;

  for( float ampl : ampls )
  {
    // Don't bother randomizing zero amplitudes
    if(ampl == 0) { phase_groups.push_back(0.0); continue; }

    // Generate one random phase (in radians) uniformy from 0 to pi/2
    float phase = rng->uniform(-3.1415926, 3.1415926);
    phase_groups.push_back(phase);
  }

  return phase_groups;
}

bool Gen::AddCoherentNoise::operator()(const input_pointer& inframe, output_pointer& outframe)
{
    if (!inframe) {
        outframe = nullptr;
        return true;
    }

    //initialize the phase groups: <<< THIS IS WRONG!
    for(auto group : coherent_groups)
    {
      std::vector<float> new_phase;

      for(int i=0; i<m_fft_length; i++ )
      {
        new_phase.push_back(m_rng->uniform(-3.1415926, 3.1415926));
      }
      phase_groups[group.first]=new_phase;
    }

    ITrace::vector outtraces;
    for (const auto& intrace : *inframe->traces())
    {
      int chid = intrace->channel();

      // Loop over the various correlation groups
      // If the channel is the new of the correlation group, make a new set of phases
      // NB: this method assumes that channels are seen in order

      //Create an zero waveform
      Waveform::realseq_t wave(m_nsamples, 0.0);

      for(auto group : coherent_groups)
      {

        int groupid = group.first;
        std::vector<float> ampls = group.second;

        // Get a new phase for that group if the correlation sequence is exausted
        if( chid % groupid == 0 )
        {
          phase_groups[groupid] = get_phase(ampls, m_rng);
        }

        // Create the ampls vector and multiply for the phase
        WireCell::Waveform::compseq_t noise_freq(ampls.size(),0);

        for (int i=0;i<int(ampls.size());i++)
        {
          const double amplitude = ampls.at(i) * sqrt(2./3.1415926);// / units::mV;
          noise_freq.at(i).real( amplitude );
          noise_freq.at(i).imag( amplitude) ;

          double freq = m_elec_resp_freq.at(i);
          double theta = -freq*m_fft_length*phase_groups[groupid].at(i); //complex rotation angle

          // Perform the phase rotation: noise_freq*(cos(theta)+ i*sin(theta))
          std::complex<double> rotation( cos(theta), sin(theta) );
          noise_freq.at(i) = multiply(noise_freq.at(i), rotation );
        }

        log->debug("AddCoherentNoise: noise_freq.size() {}", noise_freq.size());

        Waveform::realseq_t wave_temp = WireCell::Waveform::idft(noise_freq);

        //normalize the waveform ( maybe this is not necessary with appropriate rescaling )
        std::transform(wave_temp.begin(), wave_temp.end(), wave_temp.begin(),
                     [ & ](float x) { return x*m_normalization ; });

        for(auto w : wave_temp)
        {
          log->debug("AddCoherentNoise: ww {}", w);
        }


        log->debug("AddCoherentNoise: wave_temp.size() {}", wave_temp.size());
        log->debug("AddCoherentNoise: m_nsamples {}", m_nsamples);

        wave_temp.resize(m_nsamples,0);


        // Add the correlated waveforms to the original empty signal
        Waveform::increase(wave, wave_temp);

      } // end group

      log->debug("AddCoherentNoise: wave.size() {}", wave.size());

      // Add the signal (be careful to double counting with the incoherent noise)
      Waveform::increase(wave, intrace->charge());
      auto trace = make_shared<SimpleTrace>(chid, intrace->tbin(), wave);
      outtraces.push_back(trace);

    } // end channels


  outframe = make_shared<SimpleFrame>(inframe->ident(), inframe->time(),
                                        outtraces, inframe->tick());
  return true;
}
