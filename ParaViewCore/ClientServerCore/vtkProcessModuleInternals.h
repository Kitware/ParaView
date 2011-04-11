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
#ifndef __vtkProcessModuleInternals_h
#define __vtkProcessModuleInternals_h

#include "vtkWeakPointer.h"
#include "vtkSmartPointer.h"
#include "vtkSession.h"

#include <vtkstd/map>
#include <vtkstd/vector>

class vtkProcessModule::vtkInternals
{
public:
  typedef vtkstd::map<vtkIdType, vtkSmartPointer<vtkSession> > MapOfSessions;
  MapOfSessions Sessions;

  typedef vtkstd::vector<vtkWeakPointer<vtkSession> > ActiveSessionStackType;
  ActiveSessionStackType ActiveSessionStack;
};

#endif
