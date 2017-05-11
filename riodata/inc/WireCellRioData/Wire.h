#ifndef WIRECELLRIODATA_WIRE
#define WIRECELLRIODATA_WIRE

#include "Rtypes.h"

namespace WireCellRio {

struct Wire {
    Wire();
    ~Wire();

    /// The global identity of the wire.  This is an opaque value.
    int ident;

    /// The plane
    int plane;

    /// A monotonically increasing index for this wire in its plane
    int index;

    /// The identity of the associated channel.  This is an opaque value.
    int channel;

    /// The ID of the starting point of the wire.  This is an index into the WireStore
    int point1;

    /// The ID of the ending point of the wire.  This is an index into the WireStore
    int point2;

    ClassDef(Wire, 1);
};

}

#endif
