// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    cgio_helpers.h

  Copyright (c) 2013-2014 Mickael Philit
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .NAME cgio_helpers -- function used by vtkCGNSReader
//                       and vtkCGNSReaderInternal
// .SECTION Description
//     provide function to simplify "CGNS" reading through cgio
//
// .SECTION Caveats
//
//
// .SECTION Thanks
// Thanks to .

#ifndef __CGIO_HELPERS_INTERNAL__
#define __CGIO_HELPERS_INTERNAL__

#include <vector>
#include <map>
#include <string>
#include <string.h> // for inline strcmp

#include <cgns_io.h> // Low level IO for fast parsing
#include <cgnslib.h> // DataType, and other definition

#include "vtkCGNSReaderInternal.h"


namespace CGNSRead
{

//------------------------------------------------------------------------------
template <typename T>
inline int readNodeData(int cgioNum, double nodeId, std::vector<T>& data)
{
  int n;
  cgsize_t size = 1;
  cgsize_t dimVals[12];
  int ndim;

  if (cgio_get_dimensions(cgioNum, nodeId, &ndim, dimVals) != CG_OK)
    {
    cgio_error_exit("cgio_get_dimensions");
    return 1;
    }

  // allocate data
  for (n = 0; n < ndim; n++)
    {
    size *= dimVals[n];
    }
  if (size <= 0)
    {
    return 1;
    }
  data.resize(size);

  // read data
  if (cgio_read_all_data(cgioNum, nodeId, &data[0]) != CG_OK)
    {
    return 1;
    }

  return 0;
}

//------------------------------------------------------------------------------
// Specialize char array
template <>
int readNodeData<char>(int cgioNum, double nodeId, std::vector<char>& data);

//------------------------------------------------------------------------------
int readNodeStringData(int cgioNum, double nodeId, std::string& data);

//------------------------------------------------------------------------------
int getNodeChildrenId(int cgioNum, double fatherId, std::vector<double>& childrenIds);

//------------------------------------------------------------------------------
int readBaseIds(int cgioNum, double rootId, std::vector<double>& baseIds);

//------------------------------------------------------------------------------
int readBaseCoreInfo(int cgioNum, double baseId, CGNSRead::BaseInformation &baseInfo);

//------------------------------------------------------------------------------
int readBaseIteration(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readZoneIterInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readSolInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readBaseFamily(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readBaseReferenceState(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

//------------------------------------------------------------------------------
int readZoneInfo(int cgioNum, double nodeId, CGNSRead::BaseInformation& baseInfo);

}
#endif //__CGIO_HELPERS_INTERNAL__
