/*=========================================================================

   Program: ParaView
   Module:  cdi_tools.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

/*-------------------------------------------------------------------------
   Copyright (c) 2018 Niklas Roeber, DKRZ Hamburg
  -------------------------------------------------------------------------*/

#ifndef CDI_TOOLS
#define CDI_TOOLS

#include "cdi.h"
#include <string>

namespace cdi_tools
{
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

std::string GuessGridFileFromUri(const std::string& FileName);
std::string ParseTimeUnits(const int& vdate, const int& vtime);
std::string ParseCalendar(const int& calendar);

void cdi_set_cur(CDIVar* cdiVar, int Timestep, int level);

//@{
/**
 * Interface to forward to cdi float or double variant.
 */
void readslice(int streamID, int varID, int levelID, double data[], SizeType* nmiss);
void readslice(int streamID, int varID, int levelID, float data[], SizeType* nmiss);
void readvar(int streamID, int varID, double data[], SizeType* nmiss);
void readvar(int streamID, int varID, float data[], SizeType* nmiss);
//@}

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

}; // end of namespace

#endif /* CDI_TOOLS */
