#include "WireCellBio/CellWriter.h"

#include <boost/filesystem.hpp>

#include <sstream>
#include <fstream>

using namespace WireCellBio;
using namespace WireCell;
using namespace std;
using namespace boost::filesystem;

CellWriter::CellWriter(const char* dataset_name,
		       const char* geomset_name,
		       int run, int subrun, 
		       const char* data_directory_name)
    : m_name(dataset_name)
    , m_dir(data_directory_name)
    , m_count(0)
{
    if ( '/' != m_dir[m_dir.size()-1]) {
	m_dir += '/';
    }


    stringstream ss;
    ss << "\"runNo\":\""<<run<<"\",\n"
       << "\"subRunNo\":\""<<subrun<<"\",\n"
       << "\"geom\":\""<<geomset_name<<"\",\n"
       << "\"type\":\""<<dataset_name<<"\",\n"; // is this last one the same as in the file name?
    m_boiler = ss.str();
}
CellWriter::~CellWriter()
{
}

bool CellWriter::insert(const ICell::shared_vector& cells)
{
    stringstream ss;
    ss << m_dir << m_count;
    create_directories(ss.str().c_str());

    ss << "/" << m_count << "-" << m_name << ".json";
    string fname = ss.str();

    std::ofstream fstr(fname.c_str());
    fstr << "{\n" << m_boiler << "\"eventNo\":\"" << m_count << "\",\n";

    string xyz = "xyz";
    for (int ind = 0; ind<3; ++ind) {
	string comma = "";
	fstr << "\""<< xyz[ind] << "\":[";
	for (auto cell : *cells) {
	    fstr << comma << cell->center()[ind];
	    comma = ",";
	}
	fstr << "],\n";
    }
    

}
