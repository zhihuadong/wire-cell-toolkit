#include "WireCellRioData/Blob.h"

WireCellRio::Blob::Blob()
    : label(""), tbin(-1), qtot(-1), qualities(), cells()
{
}
      
WireCellRio::Blob::~Blob()
{
}

ClassImp(WireCellRio::Blob);
