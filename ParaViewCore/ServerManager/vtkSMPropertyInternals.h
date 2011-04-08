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

#include <vtkstd/map>
#include <vtkstd/vector>
#include "vtkStdString.h"

struct vtkSMPropertyInternals
{
  typedef vtkstd::map<vtkStdString, vtkSmartPointer<vtkSMDomain> > DomainMap;
  DomainMap Domains;

  typedef vtkstd::vector<vtkSmartPointer<vtkSMDomain> > DependentsVector;
  DependentsVector Dependents;
};

#endif

