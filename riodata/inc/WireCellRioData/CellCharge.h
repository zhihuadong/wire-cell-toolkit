#ifndef WIRECELLRIODATA_CELLCHARGE
#define WIRECELLRIODATA_CELLCHARGE

#include "Rtypes.h"

namespace WireCellRio {

    struct CellCharge {
	CellCharge();
	~CellCharge();

	/// The ID of the associated Cell.  This is an index into
	/// the GeometryStore.cells.
	int cellid;

	/// The charge in the cell
	float charge;

	ClassDef(CellCharge, 1);
    };

}

#endif
