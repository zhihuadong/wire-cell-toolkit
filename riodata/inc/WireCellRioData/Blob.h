#ifndef WIRECELLRIODATA_BLOB
#define WIRECELLRIODATA_BLOB

#include "WireCellRioData/CellCharge.h"

#include "Rtypes.h"

#include <string>
#include <vector>

namespace WireCellRio {

    struct Blob {
	Blob();
	~Blob();

	/// A free-form, user-defined string describing how this
	/// blob was created.
	std::string label;

	/// The time bin w.r.t. the start of the frame in which
	/// this bin exists.
	int tbin;
	    
	/// Total charge considered to be in this blob.  This may
	/// not be the sum of the charge in the individual cells.
	/// A negative charge value is undefined.
	float qtot;

	/// A user-defined collection of quality values, such as
	/// chi-squared, likelihood or other goodness-of-fits
	std::vector<float> qualities;

	/// The cell-charge values associated with this blob
	std::vector<CellCharge> cells;

	ClassDef(Blob, 1);
    };

}

#endif
