/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMPropertyInternals_h
#define vtkSMPropertyInternals_h

#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>
#include <vector>

struct vtkSMPropertyInternals
{
  typedef std::map<std::string, vtkSmartPointer<vtkSMDomain> > DomainMap;
  DomainMap Domains;

  typedef std::vector<vtkSmartPointer<vtkSMDomain> > DependentsVector;
  DependentsVector Dependents;

  // Only one source allowed for links
  vtkWeakPointer<vtkSMProperty> LinkSourceProperty;
};

#endif

// VTK-HeaderTest-Exclude: vtkSMPropertyInternals.h
