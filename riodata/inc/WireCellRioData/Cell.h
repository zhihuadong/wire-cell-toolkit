#ifndef WIRECELLRIODATA_CELL
#define WIRECELLRIODATA_CELL

#include "Rtypes.h"

namespace WireCellRio {

    struct Cell {
	Cell();
	~Cell();

	/// The global identity of the cell.  This is an opaque value.
	int ident;

	/// The ID of the U, V and W wires associated with this
	/// cell.  The IDs are indices into the WireCellRio::WireStore.
	int uid, vid, wid;

	/// The IDs of the points at the corners of the cell.
	/// Each ID is an index into the
	/// WireCellRio::TilingStore.  The IDs are ordered
	/// as one goes around the cell boundary.
	std::vector<int> corners;	   

	ClassDef(Cell, 1);
    };

}

#endif
