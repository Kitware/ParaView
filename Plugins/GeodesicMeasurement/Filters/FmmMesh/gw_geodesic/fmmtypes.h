#pragma once

#include <map>

namespace GW
{
class GW_GeodesicVertex;
};

typedef std::multimap<float,GW::GW_GeodesicVertex*> NarrowBand;
