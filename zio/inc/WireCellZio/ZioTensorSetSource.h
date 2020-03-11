/**
 * zio::Message -> WireCell::ITensorSet
 */

#ifndef WIRECELLZIO_ZIOTENSORSETSOURCE
#define WIRECELLZIO_ZIOTENSORSETSOURCE

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/ITensorSetSource.h"
#include "WireCellUtil/Logging.h"

namespace WireCell
{
namespace Zio
{

class ZioTensorSetSource : public ITensorSetSource, public IConfigurable
{
public:
    ZioTensorSetSource();
    virtual ~ZioTensorSetSource();

    /// IFrameSource
    virtual bool operator()(ITensorSet::pointer &out);

    /// IConfigurable
    virtual WireCell::Configuration default_configuration() const;
    virtual void configure(const WireCell::Configuration &config);

private:
    Configuration m_cfg; /// copy of configuration
    Log::logptr_t l;

    ITensorSet::vector m_tensors; // current set of depos
};
} // namespace Zio
} // namespace WireCell
#endif // WIRECELLZIO_ZIOTENSORSETSOURCE