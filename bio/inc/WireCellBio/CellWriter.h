#ifndef WIRECELLBIO_CELLWRITER
#define WIRECELLBIO_CELLWRITER

#include "WireCellIface/ICellVectorSink.h"

#include <string>

namespace WireCellBio {

    /** Write Cells to Bee files.
     *
     * It writes to given output directory following the patterns at
     *
     * http://bnlif.github.io/wire-cell-docs/viz/uploads/
     *
     * By default it writes to ./data/<N>/<N>-<dataset_name>.json with
     * <N> incremented on each insert().
     */
    class CellWriter : public WireCell::ICellVectorSink
    {
    public:
	CellWriter(const char* dataset_name,
		   const char* geomset_name,
		   int run = 0, int subrun = 0, 
		   const char* data_directory_name = "./data");
	virtual ~CellWriter();
	virtual bool insert(const WireCell::ICell::shared_vector& cells);

    private:

	std::string m_name;
	std::string m_dir;
	std::string m_count;
	std::string m_boiler;
    };
}

#endif
