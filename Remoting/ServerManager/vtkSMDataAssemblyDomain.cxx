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
#include "vtkSMOutputPort.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

vtkStandardNewMacro(vtkSMDataAssemblyDomain);
//----------------------------------------------------------------------------
vtkSMDataAssemblyDomain::vtkSMDataAssemblyDomain()
  : DataAssemblyValid(false)
{
}

//----------------------------------------------------------------------------
vtkSMDataAssemblyDomain::~vtkSMDataAssemblyDomain()
{
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::Update(vtkSMProperty* requestingProperty)
{
  this->DataAssemblyValid = false;
  this->Superclass::Update(requestingProperty);
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkSMDataAssemblyDomain::GetDataAssembly()
{
  if (this->DataAssemblyValid)
  {
    return this->DataAssembly;
  }

  auto inputProperty = this->GetRequiredProperty("Input");
  this->DataAssembly = nullptr;
  vtkSMUncheckedPropertyHelper helper(inputProperty);
  auto proxy = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
  auto port = helper.GetOutputPort(0);
  this->DataAssembly = (proxy ? proxy->GetOutputPort(port)->GetDataAssembly() : nullptr);
  this->DataAssemblyValid = true;
  return this->DataAssembly;
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataAssembly: " << this->DataAssembly << endl;
}
