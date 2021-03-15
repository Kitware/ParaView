/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataAssemblyListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataAssemblyListDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"

vtkStandardNewMacro(vtkSMDataAssemblyListDomain);
//----------------------------------------------------------------------------
vtkSMDataAssemblyListDomain::vtkSMDataAssemblyListDomain() = default;
//----------------------------------------------------------------------------
vtkSMDataAssemblyListDomain::~vtkSMDataAssemblyListDomain() = default;

//----------------------------------------------------------------------------
void vtkSMDataAssemblyListDomain::Update(vtkSMProperty*)
{
  auto dinfo = this->GetInputDataInformation("Input");
  if (!dinfo)
  {
    this->SetStrings({});
    return;
  }

  std::vector<std::string> strings;
  // when we start supporting multiple named assemblies, this will need to be
  // updated.
  if (dinfo->GetHierarchy())
  {
    strings.emplace_back("Hierarchy");
  }
  if (dinfo->GetDataAssembly())
  {
    strings.emplace_back("Assembly");
  }
  this->SetStrings(strings);
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
