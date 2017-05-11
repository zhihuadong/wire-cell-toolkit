#ifdef __ROOTCLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclass;

#pragma link C++ namespace WireCell;
#pragma link C++ namespace WireCellRio;

// Geometry
#pragma link C++ class WireCellRio::Point;
#pragma link C++ class WireCellRio::Cell;
#pragma link C++ class WireCellRio::Wire;

// Cell
#pragma link C++ class WireCellRio::CellCharge;
#pragma link C++ class WireCellRio::Blob;
#pragma link C++ class WireCellRio::BlobCollection;

// Channel
#pragma link C++ class WireCellRio::ChannelCharge;
#pragma link C++ class WireCellRio::ChannelSlice;

// Stores
#pragma link C++ class WireCellRio::GeometryStore;
#pragma link C++ class WireCellRio::FrameStore;

#endif
