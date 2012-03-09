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

#ifndef __vtkSMPropertyInternals_h
#define __vtkSMPropertyInternals_h

#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSmartPointer.h"

#include <map>
#include <vector>
#include "vtkStdString.h"

struct vtkSMPropertyInternals
{
  typedef std::map<vtkStdString, vtkSmartPointer<vtkSMDomain> > DomainMap;
  DomainMap Domains;

  typedef std::vector<vtkSmartPointer<vtkSMDomain> > DependentsVector;
  DependentsVector Dependents;
};

#endif

