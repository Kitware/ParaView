/*=========================================================================

  Program:   ParaView
  Module:    vtkApplyFilterCommandInternal.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkApplyFilterCommandInternal_h
#define __vtkApplyFilterCommandInternal_h

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

class vtkApplyFilterCommandInternal
{
public:
  typedef vtkstd::vector<vtkstd::string> FilterTypesVector;
  typedef vtkstd::map<vtkstd::string, FilterTypesVector> FilterTypesMap;

  FilterTypesMap FilterTypes;
};


#endif
