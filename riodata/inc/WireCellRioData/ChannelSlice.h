#ifndef WIRECELLRIODATA_CHANNELSLICE
#define WIRECELLRIODATA_CHANNELSLICE

#include "WireCellRioData/ChannelCharge.h"

#include "Rtypes.h"

#include <vector>

namespace WireCellRio {

    struct ChannelSlice {
	ChannelSlice();
	~ChannelSlice();

	/// The time bin measured from the start of the current frame.
	int tbin;

	std::vector<WireCellRio::ChannelCharge> channels;
	    

	ClassDef(ChannelSlice, 1);
    };

}

#endif
