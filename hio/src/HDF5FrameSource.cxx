#include "WireCellHio/HDF5FrameSource.h"

#include "WireCellIface/SimpleFrame.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"

WIRECELL_FACTORY(HDF5FrameSource, WireCell::Hdf5::HDF5FrameSource,
                 WireCell::IFrameSource, WireCell::IConfigurable)

using namespace WireCell;

Hdf5::HDF5FrameSource::HDF5FrameSource() : m_policy("") {}

Hdf5::HDF5FrameSource::~HDF5FrameSource() {}

Configuration Hdf5::HDF5FrameSource::default_configuration() const {
  Configuration cfg;
  cfg["filelist"] = Json::arrayValue; // list of input files, empties are skipped
  cfg["policy"] = ""; // set to "stream" to avoid sending EOS after each file's worth of depos.
  return cfg;
}

void Hdf5::HDF5FrameSource::configure(const WireCell::Configuration &cfg) {
  m_filenames = get<std::vector<std::string>>(cfg, "filelist");
  std::reverse(m_filenames.begin(), m_filenames.end()); // to use pop_back().
  m_policy = get<std::string>(cfg, "policy", "");
}

bool Hdf5::HDF5FrameSource::operator()(IFrame::pointer &out) {
  out = nullptr;
  
  if(m_frames.size() > 0) {
    out = m_frames.back();
    m_frames.pop_back();
    return true;
  }

  while (!m_filenames.empty()) {
    auto fname = m_filenames.back();
    m_filenames.pop_back();

  }
}