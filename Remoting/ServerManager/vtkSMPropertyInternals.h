// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMPropertyInternals_h
#define vtkSMPropertyInternals_h

#include "vtkSMDomain.h"     // for vtkSMDomain
#include "vtkSMProperty.h"   // for vtkSMProperty
#include "vtkSmartPointer.h" // for vtkSmartPointer
#include "vtkWeakPointer.h"  // for vtkWeakPointer

#include <map>    // for std::map
#include <vector> // for std::vector

struct vtkSMPropertyInternals
{
  typedef std::map<std::string, vtkSmartPointer<vtkSMDomain>> DomainMap;
  DomainMap Domains;

  typedef std::vector<vtkSmartPointer<vtkSMDomain>> DependentsVector;
  DependentsVector Dependents;

  // Only one source allowed for links
  vtkWeakPointer<vtkSMProperty> LinkSourceProperty;
};

#endif

// VTK-HeaderTest-Exclude: vtkSMPropertyInternals.h
