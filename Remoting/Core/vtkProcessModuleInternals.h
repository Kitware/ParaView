/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkProcessModuleInternals_h
#define vtkProcessModuleInternals_h

#include "vtkSession.h"      // for vtkSession
#include "vtkSmartPointer.h" // for vtkSmartPointer
#include "vtkWeakPointer.h"  // for vtkWeakPointer

#include <map>    // for std::map
#include <vector> // for std::vector

class vtkProcessModuleInternals
{
public:
  typedef std::map<vtkIdType, vtkSmartPointer<vtkSession>> MapOfSessions;
  MapOfSessions Sessions;

  typedef std::vector<vtkWeakPointer<vtkSession>> ActiveSessionStackType;
  ActiveSessionStackType ActiveSessionStack;
};

#endif

// VTK-HeaderTest-Exclude: vtkProcessModuleInternals.h
