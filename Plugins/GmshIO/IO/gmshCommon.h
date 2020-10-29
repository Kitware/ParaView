#ifndef gmshCommon_h
#define gmshCommon_h

//-----------------------------------------------------------------------------
enum class GmshPrimitive : char
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

#endif // gmshCommon_h
