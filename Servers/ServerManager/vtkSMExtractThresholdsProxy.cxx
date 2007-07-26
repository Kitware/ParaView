/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractThresholdsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractThresholdsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSelection.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkSMExtractThresholdsProxy);
vtkCxxRevisionMacro(vtkSMExtractThresholdsProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMExtractThresholdsProxy::vtkSMExtractThresholdsProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMExtractThresholdsProxy::~vtkSMExtractThresholdsProxy()
{
}


//-----------------------------------------------------------------------------
void vtkSMExtractThresholdsProxy::CreateVTKObjects()
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
void vtkSMExtractThresholdsProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkSMProxy* selectionSource = this->GetSubProxy("SelectionSource");
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("FieldType"));
  ivp->SetElement(0, vtkSelection::POINT);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  ivp->SetElement(0, vtkSelection::THRESHOLDS);

  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMExtractThresholdsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
