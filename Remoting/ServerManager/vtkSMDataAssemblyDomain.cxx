/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataAssemblyDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataAssemblyDomain.h"

#include "vtkDataAssembly.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMUncheckedPropertyHelper.h"

vtkStandardNewMacro(vtkSMDataAssemblyDomain);
//----------------------------------------------------------------------------
vtkSMDataAssemblyDomain::vtkSMDataAssemblyDomain()
{
}

//----------------------------------------------------------------------------
vtkSMDataAssemblyDomain::~vtkSMDataAssemblyDomain() = default;

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::Update(vtkSMProperty*)
{
  auto dinfo = this->GetInputDataInformation("Input");
  if (!dinfo)
  {
    this->ChooseAssembly({}, nullptr);
  }
  else
  {
    auto activeAssembly = this->GetRequiredProperty("ActiveAssembly");
    if (!activeAssembly)
    {
      this->ChooseAssembly("Hierarchy", dinfo->GetHierarchy());
    }
    else
    {
      const std::string name{ vtkSMUncheckedPropertyHelper(activeAssembly).GetAsString(0) };
      if (name == "Hierarchy")
      {
        this->ChooseAssembly(name, dinfo->GetHierarchy());
      }
      else if (name == "Assembly")
      {
        this->ChooseAssembly(name, dinfo->GetDataAssembly());
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkSMDataAssemblyDomain::GetDataAssembly() const
{
  return this->Assembly;
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::ChooseAssembly(const std::string& name, vtkDataAssembly* assembly)
{
  if (this->Name != name || assembly != this->Assembly ||
    (assembly != nullptr && assembly->GetMTime() > this->GetMTime()))
  {
    this->Name = name;
    this->Assembly = assembly;
    this->DomainModified();
  }
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
