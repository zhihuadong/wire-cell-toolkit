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

    std::string filename;
    std::vector<std::string> load_path, extvars, extcode, tlavars, tlacode;

    app.add_option("-P,--path", load_path,
                   "Search paths to consider in addition to those in WIRECELL_PATH")->type_size(1)->allow_extra_args(false);
    app.add_option("-V,--ext-str", extvars,
                   "Jsonnet external variable as <name>=<string>")->type_size(1)->allow_extra_args(false);
    app.add_option("-C,--ext-code", extcode,
                   "Jsonnet external code as <name>=<string>")->type_size(1)->allow_extra_args(false);
    app.add_option("-A,--tla-str", tlavars,
                   "Jsonnet level argument value <name>=<string>")->type_size(1)->allow_extra_args(false);
    app.add_option(",--tla-code", tlacode,
                   "Jsonnet level argument code <name>=<string>")->type_size(1)->allow_extra_args(false);
    app.add_option("file", filename, "Jsonnet file to compile");
                    
    // app.set_help_flag();
    // auto help = app.add_flag("-h,--help", "Print help message");

    CLI11_PARSE(app, argc, argv);

    // try {
    //     app.parse(argc, argv);
    //     if (*help) {
    //         throw CLI::CallForHelp();
    //     }
    // }
    // catch(const CLI::Error &e) {
    //     return app.exit(e);
    // }


    std::map<std::string, std::string> m_extvars, m_extcode, m_tlavars, m_tlacode;

    for (auto path : load_path) {
        std::cerr << "path: " << path << std::endl;
    }
    std::cerr << "input: " << filename << std::endl;

    for (auto vev : extvars) {
        std::cerr << "extvar: " << vev << std::endl;
        auto vv = String::split(vev, "=");
        m_extvars[vv[0]] = vv[1];
    }
    for (auto vev : extcode) {
        std::cerr << "extcode: " << vev << std::endl;
        auto vv = String::split(vev, "=");
        m_extcode[vv[0]] = vv[1];
    }
    for (auto vev : tlavars) {
        std::cerr << "tlavar: " << vev << std::endl;
        auto vv = String::split(vev, "=");
        m_tlavars[vv[0]] = vv[1];
    }
    for (auto vev : tlacode) {
        std::cerr << "tlacode: " << vev << std::endl;
        auto vv = String::split(vev, "=");
        m_tlacode[vv[0]] = vv[1];
    }

    try {
        if (filename.empty()) {
            throw CLI::CallForHelp();
        }
    }
    catch(const CLI::Error &e) {
        std::cerr << "Must give at least one Jsonnet file to compile" << std::endl;
        return app.exit(e);
    }
    Persist::Parser parser(load_path, m_extvars, m_extcode,
                           m_tlavars, m_tlacode);

    auto jdat = parser.load(filename);
    std::cout << jdat << std::endl;

    return 0;
}
