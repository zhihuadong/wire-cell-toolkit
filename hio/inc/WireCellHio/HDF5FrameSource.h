/** Read in IFrame in hdf5 format
 */

#ifndef WIRECELLHDF5_HDF5FRAMESOURCE
#define WIRECELLHDF5_HDF5FRAMESOURCE

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameSource.h"

namespace WireCell {
namespace Hdf5 {

class HDF5FrameSource : public IFrameSource, public IConfigurable {
public:
  HDF5FrameSource();
  virtual ~HDF5FrameSource();

  /// IFrameSource
  virtual bool operator()(IFrame::pointer &out);

  /// IConfigurable
  virtual WireCell::Configuration default_configuration() const;
  virtual void configure(const WireCell::Configuration &config);

private:
  std::vector<std::string> m_filenames;
  std::string m_policy;
  IFrame::vector m_frames; // current set of depos
};
} // namespace Hdf5
} // namespace WireCell

#endif