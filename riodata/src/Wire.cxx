#include "WireCellRioData/Wire.h"


WireCellRio::Wire::Wire()
    : ident(-1), plane(-1), index(-1), channel(-1), point1(-1), point2(-1)
{
}
WireCellRio::Wire::~Wire()
{
}

ClassImp(WireCellRio::Wire);
