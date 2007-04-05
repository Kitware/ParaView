/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAbstractDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAbstractDisplayProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkCxxRevisionMacro(vtkSMAbstractDisplayProxy, "1.2");

//-----------------------------------------------------------------------------
vtkSMAbstractDisplayProxy::vtkSMAbstractDisplayProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMAbstractDisplayProxy::~vtkSMAbstractDisplayProxy()
{
}
 
//-----------------------------------------------------------------------------
void vtkSMAbstractDisplayProxy::SetVisibilityCM(int v)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, v);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMAbstractDisplayProxy::GetVisibilityCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMAbstractDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

