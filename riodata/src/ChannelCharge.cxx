#include "WireCellRioData/ChannelCharge.h"


WireCellRio::ChannelCharge::ChannelCharge()
    : chid(-1), charge(-1.0), uncertainty(0)
{
}
WireCellRio::ChannelCharge::~ChannelCharge()
{
}

ClassImp(WireCellRio::ChannelCharge);
