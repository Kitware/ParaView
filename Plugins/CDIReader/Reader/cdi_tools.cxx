#include "cdi_tools.h"
#include "vtk_netcdf.h"

#include "cdi.h"
#include "helpers.h"

#include <string.h>

using namespace std;

string GuessGridFileFromUri(const string FileName)
{
  string guess = "";
  // NETCDF-CALL:
  string uri_filename = GetBasenameFromUri(get_att_str(FileName, "NC_GLOBAL", "grid_file_uri"));
  if (uri_filename != "")
  {
    guess = GetPathName(FileName) + "/" + uri_filename;
  }
  return guess;
}

string get_att_str(const string filename, const string var, const string att)
{
  string result = "";

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

string ParseTimeUnits(const int vdate, const int vtime)
{

  string date = ::ConvertInt(vdate);
  string time = ::ConvertInt(vtime);
  string time_units = "";
  if (strlen(date.c_str()) < 8)
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
  if (strlen(time.c_str()) < 6)
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

string ParseCalendar(const int calendar)
{
  string calendar_name = "";
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

// These pass on to CDI functions where function overloading is broken due to F interface stuff, or
// so.
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
