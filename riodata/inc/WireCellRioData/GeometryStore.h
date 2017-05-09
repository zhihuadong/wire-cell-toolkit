#ifndef WIRECELLRIODATA_GEOMETRYSTORE
#define WIRECELLRIODATA_GEOMETRYSTORE

#include "WireCellRioData/Wire.h"
#include "WireCellRioData/Cell.h"
#include "WireCellRioData/Point.h"

#include "Rtypes.h"

#include <vector>

namespace WireCellRio {

    struct GeometryStore {
	GeometryStore();
	~GeometryStore();

	std::vector<WireCellRio::Cell> cells;
	std::vector<WireCellRio::Wire> wires;
	std::vector<WireCellRio::Point> points;

	ClassDef(GeometryStore, 1);
    };

}

#endif
