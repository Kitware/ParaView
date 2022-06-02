/*=========================================================================

   Program: ParaView
   Module:  cdi_tools.cxx

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

#include "cdi_tools.h"
#include "vtk_netcdf.h"
#include "vtksys/SystemTools.hxx"

#include <cstring>

//----------------------------------------------------------------------------
std::string get_att_str(const std::string& filename, const std::string& var, const std::string& att)
{
  std::string result;

  int ncFD;
  nc_open(filename.c_str(), NC_NOWRITE, &ncFD);
  int status = NC_NOERR, varId;
  size_t len;
  if (var == "NC_GLOBAL")
  {
    varId = NC_GLOBAL;
  }
  else
  {
    status = nc_inq_varid(ncFD, var.c_str(), &varId);
    if (status != NC_NOERR)
      return result;
  }
  status = nc_inq_attlen(ncFD, varId, att.c_str(), &len);
  if (status != NC_NOERR)
    return result;
  char buffer[len + 1];
  status = nc_get_att_text(ncFD, varId, att.c_str(), buffer);
  if (status != NC_NOERR)
    return result;
  buffer[len] = 0;
  result = buffer;
  return result;
}

namespace cdi_tools
{
//----------------------------------------------------------------------------
std::string GuessGridFileFromUri(const std::string& FileName)
{
  std::string guess;
  // NETCDF-CALL:
  std::string uri_filename =
    vtksys::SystemTools::GetFilenameName(::get_att_str(FileName, "NC_GLOBAL", "grid_file_uri"));
  if (!uri_filename.empty())
  {
    std::string dir = vtksys::SystemTools::GetParentDirectory(FileName);
    // guess = vtksys::SystemTools::JoinPath({dir, uri_filename});
    guess = dir + "/" + uri_filename;
  }
  return guess;
}

//----------------------------------------------------------------------------
std::string ParseTimeUnits(const int& vdate, const int& vtime)
{
  std::string date = std::to_string(vdate);
  std::string time = std::to_string(vtime);
  std::string time_units;
  if (date.size() < 8)
  {
    time_units = "days since 0-1-1 ";
  }
  else
  {
    time_units = "days since ";
    time_units += date.substr(0, 4);
    time_units += "-";
    time_units += date.substr(4, 2);
    time_units += "-";
    time_units += date.substr(6, 2);
    time_units += " ";
  }
  if (time.size() < 6)
  {
    time_units += "00:00:00";
  }
  else
  {
    time_units += time.substr(0, 2);
    time_units += ":";
    time_units += time.substr(2, 2);
    time_units += ":";
    time_units += time.substr(4, 2);
  }
  return time_units;
}

//----------------------------------------------------------------------------
std::string ParseCalendar(const int& calendar)
{
  std::string calendar_name;
  switch (calendar)
  {
    case CALENDAR_STANDARD:
      calendar_name = "standard";
      break;
    case CALENDAR_GREGORIAN:
      calendar_name = "gregorian";
      break;
    case CALENDAR_PROLEPTIC:
      calendar_name = "proleptic_gregorian";
      break;
    case CALENDAR_NONE:
      calendar_name = "none";
      break;
    case CALENDAR_360DAYS:
      calendar_name = "360_day";
      break;
    case CALENDAR_365DAYS:
      calendar_name = "365_day";
      break;
    case CALENDAR_366DAYS:
      calendar_name = "366_day";
      break;
  }
  return calendar_name;
}

//----------------------------------------------------------------------------
//  CDI helper functions
//----------------------------------------------------------------------------
void cdi_set_cur(CDIVar* cdiVar, int Timestep, int level)
{
  cdiVar->Timestep = Timestep;
  cdiVar->LevelID = level;
}

void readslice(int streamID, int varID, int levelID, double data[], SizeType* nmiss)
{
  streamReadVarSlice(streamID, varID, levelID, data, nmiss);
}
void readslice(int streamID, int varID, int levelID, float data[], SizeType* nmiss)
{
  streamReadVarSliceF(streamID, varID, levelID, data, nmiss);
}
void readvar(int streamID, int varID, double data[], SizeType* nmiss)
{
  streamReadVar(streamID, varID, data, nmiss);
}
void readvar(int streamID, int varID, float data[], SizeType* nmiss)
{
  streamReadVarF(streamID, varID, data, nmiss);
}

}; // end of namespace cdi_tools
