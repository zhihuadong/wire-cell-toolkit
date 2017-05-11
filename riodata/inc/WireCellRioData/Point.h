#ifndef WIRECELLRIODATA_POINT
#define WIRECELLRIODATA_POINT

#include "Rtypes.h"

namespace WireCellRio {

    struct Point {
	Point(float x=0, float y=0, float z=0);
	~Point();

	/// A location in Cartesian 3-space.
	float x, y, z;

	ClassDef(Point, 1);
    };

}

#endif
