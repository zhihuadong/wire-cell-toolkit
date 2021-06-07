// A wire cell version of the jsonnet cli.

#include "CLI11.hpp"

#include "WireCellUtil/Persist.h"
#include "WireCellUtil/String.h"

#include <iostream>
#include <vector>
#include <map>

using namespace WireCell;

int main(int argc, char* argv[])
{
    CLI::App app{"wcsonnet is a Wire-Cell Toolkit aware Jsonnet compiler"};

    std::vector<std::string> cfgfiles, load_path, extvars, extcode, tlavars, tlacode;

    app.add_option("-P,--path", load_path,
                   "Search paths to consider in addition to those in WIRECELL_PATH");

    app.add_option("-V,--ext-str", extvars,
                   "Jsonnet external variable as <name>=<string>");
    app.add_option("-C,--ext-code", extvars,
                   "Jsonnet external code as <name>=<string>");
    app.add_option("-A,--tla-str", tlavars,
                   "Jsonnet level argument value <name>=<string>");
    app.add_option("--tla-code", tlacode,
                   "Jsonnet level argument code <name>=<string>");
    app.add_option("files", cfgfiles,
                   "Jsonnet files to compile");
                    
    app.set_help_flag();
    auto help = app.add_flag("-h,--help", "Print help message");

    //CLI11_PARSE(app, argc, argv);

    try {
        app.parse(argc, argv);
        if (*help or cfgfiles.empty()) {
            throw CLI::CallForHelp();
        }
    }
    catch(const CLI::Error &e) {
        return app.exit(e);
    }


    std::map<std::string, std::string> m_extvars, m_extcode, m_tlavars, m_tlacode;

    for (auto vev : extvars) {
        auto vv = String::split(vev, "=");
        m_extvars[vv[0]] = vv[1];
    }
    for (auto vev : extcode) {
        auto vv = String::split(vev, "=");
        m_extcode[vv[0]] = vv[1];
    }
    for (auto vev : tlavars) {
        auto vv = String::split(vev, "=");
        m_tlavars[vv[0]] = vv[1];
    }
    // Add any top-level argument code
    for (auto vev : tlacode) {
        auto vv = String::split(vev, "=");
        m_tlacode[vv[0]] = vv[1];
    }

    Persist::Parser parser(load_path, m_extvars, m_extcode,
                           m_tlavars, m_tlacode);

    auto jdat = parser.load(argv[1]);
    std::cout << jdat << std::endl;

    return 0;
}
