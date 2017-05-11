#ifndef WIRECELLRIODATA_FRAMESTORE
#define WIRECELLRIODATA_FRAMESTORE

#include "WireCellRioData/ChannelSlice.h"
#include "WireCellRioData/Blob.h"
#include "WireCellRioData/BlobCollection.h"

#include "Rtypes.h"

#include <vector>

namespace WireCellRio {

    struct FrameStore {
	FrameStore();
	~FrameStore();

	std::vector<WireCellRio::Blob> blobs;
	std::vector<WireCellRio::BlobCollection> blobcollections;
	std::vector<WireCellRio::ChannelSlice> chslices;

	ClassDef(FrameStore, 1);
    };

}

#endif
