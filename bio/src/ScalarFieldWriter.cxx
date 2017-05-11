#include "WireCellBio/ScalarFieldWriter.h"
#include "WireCellUtil/Units.h"


#include <sstream>
#include <fstream>

using namespace WireCellBio;
using namespace WireCell;
using namespace std;
using namespace boost::filesystem;

ScalarFieldWriter::ScalarFieldWriter(const char* dataset_name,
				     const char* geomset_name,
				     int run, int subrun, 
				     const char* data_directory_name)
    : m_name(dataset_name)
    , m_dir(data_directory_name)
    , m_count(0)
{
    stringstream ss;
    ss << "\"runNo\":\""<<run<<"\",\n"
       << "\"subRunNo\":\""<<subrun<<"\",\n"
       << "\"geom\":\""<<geomset_name<<"\",\n"
       << "\"type\":\""<<dataset_name<<"\",\n"; // is this last one the same as in the file name?
    m_boiler = ss.str();
}
ScalarFieldWriter::~ScalarFieldWriter()
{
}

bool ScalarFieldWriter::operator()(const input_pointer& field)
{
    path dpath = m_dir / std::to_string(m_count);

    if (exists(dpath)) {
	cerr << "PointFieldWriter: output path already exists: " << dpath << endl;
	return false;
    }
    create_directories(dpath);

    string fname = std::to_string(m_count) + "-" + m_name + ".json";
    path fpath = dpath / fname;

    std::ofstream fstr(fpath.c_str());
    fstr << "{\n" << m_boiler;
    string xyz = "xyz";
    for (int ind = 0; ind<3; ++ind) {
	string comma = "";
	fstr << "\""<< xyz[ind] << "\":[";
	for (auto pt : *field) {
	    fstr << comma << pt.second[ind]/units::cm;
	    comma = ",";
	}
	fstr << "],\n";
    }
    string comma = "";
    fstr << "\"q\":[";
    for (auto pt : *field) {
	fstr << comma << pt.first;
	comma = ",";
    }
    fstr << "],\n";

    fstr << "\"eventNo\":\"" << m_count << "\"\n";
    fstr << "}\n";

    ++m_count;
    return true;

}
