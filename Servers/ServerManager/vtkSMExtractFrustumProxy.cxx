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
vtkCxxRevisionMacro(vtkSMExtractFrustumProxy, "1.1");
//-----------------------------------------------------------------------------
vtkSMExtractFrustumProxy::vtkSMExtractFrustumProxy()
{
  this->Frustum = NULL;
}

//-----------------------------------------------------------------------------
vtkSMExtractFrustumProxy::~vtkSMExtractFrustumProxy()
{
  if (this->Frustum)
    {
    this->Frustum->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMExtractFrustumProxy::SetFrustum(double *vertices)
{
  if (this->Frustum == NULL)
    {
    this->Frustum = vtkDoubleArray::New();
    this->Frustum->SetNumberOfComponents(4);
    this->Frustum->SetNumberOfTuples(8);
    }
  double *data = this->Frustum->GetPointer(0);
  memcpy(data, vertices, 32*sizeof(double));
}

//-----------------------------------------------------------------------------
void vtkSMExtractFrustumProxy::RemoveAllValues()
{
  if (this->Frustum)
    {
    this->Frustum->Reset();
    }
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

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    selectionSource->GetProperty("Frustum"));
  if (this->Frustum)
    {
    dvp->SetNumberOfElements(32);
    dvp->SetElements((double*)this->Frustum->GetVoidPointer(0));
    }  
  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMExtractFrustumProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
