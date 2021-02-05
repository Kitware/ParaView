
#ifndef CDI_TOOLS
#define CDI_TOOLS

#include "cdi.h"
#include <string>

struct CDIVar
{
  int StreamID;
  int VarID;
  int GridID;
  int ZAxisID;
  int GridSize;
  int NLevel;
  int Type;
  int ConstTime;
  int Timestep;
  int LevelID;
  char Name[CDI_MAX_NAME];
};

std::string GuessGridFileFromUri(const std::string FileName);
std::string get_att_str(const std::string filename, const std::string var, const std::string att);
std::string ParseTimeUnits(const int vdate, const int vtime);
std::string ParseCalendar(const int calendar);

void cdi_set_cur(CDIVar* cdiVar, int Timestep, int level);

// These pass on to CDI functions where function overloading is broken due to F interface stuff, or
// so.
void readslice(int streamID, int varID, int levelID, double data[], SizeType* nmiss);

void readslice(int streamID, int varID, int levelID, float data[], SizeType* nmiss);

void readvar(int streamID, int varID, double data[], SizeType* nmiss);

void readvar(int streamID, int varID, float data[], SizeType* nmiss);

template <class T>
void cdi_get_part_unstruct(CDIVar* cdiVar, int start, size_t size, T* buffer, int nlevels);
template <class T>
void cdi_get_part_struct(CDIVar* cdiVar, int start, size_t size, T* buffer, int nlevels);
template <class T>
void cdi_get_full(CDIVar* cdiVar, T* buffer, int nlevels);

template <class T>
void cdi_get_part(CDIVar* cdiVar, int start, size_t size, T* buffer, int nlevels, bool grib)
{
  bool full = cdiVar->GridSize == size;
  if (full)
  {
    cdi_get_full(cdiVar, buffer, nlevels);
  }
  else // Only a part of a layer is requested. We need to distinguish between 2d and 1d datasets.
  {
    const size_t xdim_size = gridInqXsize(cdiVar->GridID), ydim_size = gridInqYsize(cdiVar->GridID),
                 grid_size = cdiVar->GridSize;

    if ((xdim_size == grid_size || ydim_size == grid_size) && !grib)
      cdi_get_part_unstruct(cdiVar, start, size, buffer, nlevels);
    else
      cdi_get_part_struct(cdiVar, start, size, buffer, nlevels);
  }
}

template <class T>
void cdi_get_full(CDIVar* cdiVar, T* buffer, int nlevels)
{
  SizeType nmiss;
  int nrecs = streamInqTimestep(cdiVar->StreamID, cdiVar->Timestep);
  if (nrecs > 0)
  {
    if (nlevels == 1)
    {
      readslice(cdiVar->StreamID, cdiVar->VarID, cdiVar->LevelID, buffer, &nmiss);
    }
    else
      readvar(cdiVar->StreamID, cdiVar->VarID, buffer, &nmiss);
  }
}

template <class T>
void cdi_get_part_struct(CDIVar* cdiVar, int start, size_t size, T* buffer, int nlevels)
{
  const size_t gridsize = cdiVar->GridSize;
  T fullbuff[gridsize * nlevels];
  cdi_get_full(cdiVar, fullbuff, nlevels);
  for (size_t lev = 0; lev < nlevels; lev++)
  {
    for (size_t i = 0; i < size; i++)
    {
      buffer[i + lev * size] = fullbuff[(lev * gridsize) + start + i];
    }
  }
}

template <class T>
void cdi_get_part_unstruct(CDIVar* cdiVar, int start, size_t size, T* buffer, int nlevels)
{
  SizeType nmiss;
  int memtype = 0;
  int nrecs = streamInqTimestep(cdiVar->StreamID, cdiVar->Timestep);
  if (nrecs > 0)
  {
    if (std::is_same<T, double>::value)
      memtype = 1; // this is CDI memtype double
    else if (std::is_same<T, float>::value)
      memtype = 2; // this is CDI memtype float
  }

  if (nlevels == 1)
    streamReadVarSlicePart(cdiVar->StreamID, cdiVar->VarID, cdiVar->LevelID, cdiVar->Type, start,
      size, buffer, &nmiss, memtype);
  else
    streamReadVarPart(
      cdiVar->StreamID, cdiVar->VarID, cdiVar->Type, start, size, buffer, &nmiss, memtype);
}

#endif /* CDI_TOOLS */
