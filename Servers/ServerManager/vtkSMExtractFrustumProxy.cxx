/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractFrustumProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractFrustumProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSelection.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkSMExtractFrustumProxy);
vtkCxxRevisionMacro(vtkSMExtractFrustumProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMExtractFrustumProxy::vtkSMExtractFrustumProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMExtractFrustumProxy::~vtkSMExtractFrustumProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMExtractFrustumProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkSMSourceProxy* selectionSource = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("SelectionSource"));
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }
  
  this->AddInput(selectionSource, "SetSelectionConnection");
}

//-----------------------------------------------------------------------------
void vtkSMExtractFrustumProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkSMProxy* selectionSource = this->GetSubProxy("SelectionSource");
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }

  vtkSMIntVectorProperty* ivp = NULL;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  ivp->SetElement(0, vtkSelection::FRUSTUM);
  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMExtractFrustumProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
