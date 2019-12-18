/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextScenePrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkSpyPlotHistoryReaderPrivate
 * @brief   Private implementation for
 * spy plot history file reader.
 *
 *
 * \internal
*/

#ifndef vtkSpyPlotHistoryReaderPrivate_h
#define vtkSpyPlotHistoryReaderPrivate_h

#include <vtksys/SystemTools.hxx>

#include <map>     // Needed for STL map.
#include <set>     // Needed for STL set.
#include <sstream> // Needed for STL sstream.
#include <string>  // Needed for STL string.
#include <vector>  // Needed for STL vector.

//-----------------------------------------------------------------------------
namespace SpyPlotHistoryReaderPrivate
{

//========================================================================
class TimeStep
{
public:
  double time;
  std::streampos file_pos;
};

//========================================================================
template <class T>
bool convert(const std::string& num, T& t)
{
  std::istringstream i(num);
  i >> t;
  return !i.fail();
}

//========================================================================
void trim(std::string& string, const std::string& whitespace = " \t\"")
{
  const size_t begin = string.find_first_not_of(whitespace);
  if (begin == std::string::npos)
  {
    // no content
    return;
  }
  const size_t end = string.find_last_not_of(whitespace);
  const size_t range = end - begin + 1;
  string = string.substr(begin, range);
  return;
}

//========================================================================
int rowFromHeaderCol(const std::string& str)
{
  const size_t begin = str.rfind(".");
  if (begin == std::string::npos)
  {
    // no content, so invalid row Id
    return -1;
  }
  int row = -1;
  bool valid = convert(str.substr(begin + 1), row);
  return (valid) ? row : -1;
}

//========================================================================
std::string nameFromHeaderCol(const std::string& str)
{
  const size_t begin = str.rfind(".");
  if (begin == std::string::npos)
  {
    // no content
    return str;
  }
  std::string t(str.substr(0, begin));
  trim(t);
  return t;
}

//========================================================================
void split(const std::string& s, const char& delim, std::vector<std::string>& elems)
{
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim))
  {
    trim(item);
    elems.push_back(item);
  }
  return;
}

//========================================================================
void getMetaHeaderInfo(const std::string& s, const char& delim, std::map<std::string, int>& fields,
  std::map<int, std::string>& lookup)
{
  std::stringstream ss(s);
  std::string item;
  size_t count = 0;
  int index = 0;
  while (std::getline(ss, item, delim))
  {
    trim(item);

    // some hscth files have "time" with different case, so we change the
    // case to a consistent value i.e. all lowercase (BUG #12983).
    if (vtksys::SystemTools::LowerCase(item) == "time")
    {
      item = "time";
    }
    if (fields.find(item) != fields.end())
    {
      ++count;
      fields[item] = index;
      lookup[index] = item;
    }
    if (count == fields.size())
    {
      return;
    }
    ++index;
  }
  return;
}

//========================================================================
void getTimeStepInfo(const std::string& s, const char& delim, std::map<int, std::string>& lookup,
  std::map<std::string, std::string>& info)
{
  std::stringstream ss(s);
  std::string item;
  int index = 0;
  size_t count = 0;
  while (std::getline(ss, item, delim))
  {
    trim(item);
    if (lookup.find(index) != lookup.end())
    {
      // map the header name to this rows value
      // ie time is col 3, so map info[time] to this rows 3rd col
      info[lookup[index]] = item;
      ++count;
    }
    if (count == lookup.size())
    {
      break;
    }
    ++index;
  }
  return;
}

//========================================================================
std::vector<std::string> createTableLayoutFromHeader(std::string& header, const char& delim,
  std::map<int, int>& columnIndexToRowId, std::map<int, std::string>& fieldCols)
{
  // the single presumption we have is that all the properties points
  // are continuous in the header
  std::vector<std::string> cols;
  cols.reserve(header.size());
  split(header, delim, cols);
  std::vector<std::string>::const_iterator it;

  // setup the size of the new header
  std::vector<std::string> newHeader;
  newHeader.reserve(cols.size());

  // find the first "." variable
  bool foundStart = false;
  int rowNumber = -1;
  int index = 0;

  for (it = cols.begin(); it != cols.end(); ++it)
  {
    if ((*it).find(".") != std::string::npos)
    {
      foundStart = true;
      rowNumber = rowFromHeaderCol(*it);
      newHeader.push_back(nameFromHeaderCol(*it));
      columnIndexToRowId.insert(std::pair<int, int>(index, rowNumber));
      break;
    }
    else
    {
      // this is a field property
      fieldCols[index] = nameFromHeaderCol(*it);
    }
    ++index;
  }
  if (!foundStart)
  {
    return newHeader;
  }

  // now track the number of variables we have,
  // and the names of each variable. This way we know how
  // many rows to have in our new table
  ++index;
  ++it;
  int numberOfCols = 1;
  while (rowFromHeaderCol(*it) == rowNumber)
  {
    newHeader.push_back(nameFromHeaderCol(*it));
    ++index;
    ++it;
    ++numberOfCols;
  }
  while (it != cols.end() && (rowNumber = rowFromHeaderCol(*it)) != -1)
  {
    columnIndexToRowId.insert(std::pair<int, int>(index, rowNumber));
    index += numberOfCols;
    it += numberOfCols;
  }
  while (it != cols.end())
  {
    // this is a field property
    fieldCols[index] = nameFromHeaderCol(*it);
    ;
    ++it;
    ++index;
  }
  return newHeader;
}
}

#endif
// VTK-HeaderTest-Exclude: vtkSpyPlotHistoryReaderPrivate.h
