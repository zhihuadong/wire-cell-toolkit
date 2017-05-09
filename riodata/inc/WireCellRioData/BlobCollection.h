#ifndef WIRECELLRIODATA_BLOBCOLLECTION
#define WIRECELLRIODATA_BLOBCOLLECTION

#include "Rtypes.h"

#include <vector>

namespace WireCellRio {

    struct BlobCollection {
	BlobCollection();
	~BlobCollection();

	/// The IDs of the blobs associated with this collection.
	/// The ID is an index into the CellStore.blobs vector.
	std::vector<int> blobids;

	ClassDef(BlobCollection, 1);
    };

}

#endif
