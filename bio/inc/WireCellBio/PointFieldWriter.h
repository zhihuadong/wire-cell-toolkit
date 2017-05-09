#ifndef WIRECELLBIO_POINTFIELDWRITER
#define WIRECELLBIO_POINTFIELDWRITER

#include "WireCellIface/IPointFieldSink.h"

#include <boost/filesystem.hpp>
#include <string>

namespace WireCellBio {

    /** Write point fields to Bee files.
     *
     * It writes to given output directory following the patterns at
     *
     * http://bnlif.github.io/wire-cell-docs/viz/uploads/
     *
     * By default it writes to ./data/<N>/<N>-<dataset_name>.json with
     * <N> incremented on each insert().
     */
    class PointFieldWriter : public WireCell::IPointFieldSink
    {
    public:
	PointFieldWriter(const char* dataset_name,
		   const char* geomset_name,
		   int run = 0, int subrun = 0, 
		   const char* data_directory_name = "./data");
	virtual ~PointFieldWriter();
	virtual bool operator()(const input_pointer& points);

    private:

	std::string m_name;
	boost::filesystem::path m_dir;
	int m_count;
	std::string m_boiler;
    };
}

#endif
