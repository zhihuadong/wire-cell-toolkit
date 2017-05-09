#ifndef WIRECELLRIODATA_CHANNELCHARGE
#define WIRECELLRIODATA_CHANNELCHARGE

#include "Rtypes.h"

namespace WireCellRio {

    struct ChannelCharge {
	ChannelCharge();
	~ChannelCharge();

	/// The channel ID (an opaque number)
	int chid;

	/// The charge on this channel (in this tbin)
	float charge;

	/// A measurement of the uncertainty of the charge
	float uncertainty;

	ClassDef(ChannelCharge, 1);
    };

}

#endif
