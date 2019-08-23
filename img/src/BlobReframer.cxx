/*
  This starts by finding the set of all frames accessible via slices
  in a cluster that have at least one blob.



 */

#include "WireCellImg/BlobReframer.h"
#include "WireCellUtil/NamedFactory.h"


WIRECELL_FACTORY(BlobReframer, WireCell::Img::BlobReframer,
                 WireCell::IClusterFramer, WireCell::IConfigurable)


using namespace WireCell;

Img::BlobReframer::BlobReframer()
{
}

Img::BlobReframer::~BlobReframer()
{
}

void Img::BlobReframer::configure(const WireCell::Configuration& cfg)
{
}

WireCell::Configuration Img::BlobReframer::default_configuration() const
{
    Configuration cfg;
    return cfg;
}

// cluster goes in, frame goes out.
bool Img::BlobReframer::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        return true;            // EOS
    }

    cluster_indexed_graph_t grind(in->graph());

    for (auto iblob : oftype<IBlob::pointer>(grind)) {
        // to be continued.
    }

    
    return true;
}

