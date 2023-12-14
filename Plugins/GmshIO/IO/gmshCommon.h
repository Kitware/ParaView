#ifndef gmshCommon_h
#define gmshCommon_h

#include <typeindex>

//-----------------------------------------------------------------------------
enum class GmshPrimitive : signed char
{
  Unsupported = -1,
  POINT = 15,
  LINE = 1,
  TRIANGLE = 2,
  QUAD = 3,
  TETRA = 4,
  HEXA = 5,
  PRISM = 6,
  PYRAMID = 7
};

namespace std
{
template <>
struct hash<GmshPrimitive>
{
  size_t operator()(GmshPrimitive x) const
  {
    return hash<signed char>()(static_cast<signed char>(x));
  }
};
}

#endif // gmshCommon_h
