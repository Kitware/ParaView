/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractLocationsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractLocationsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMExtractLocationsProxy);
vtkCxxRevisionMacro(vtkSMExtractLocationsProxy, "1.5");
//-----------------------------------------------------------------------------
vtkSMExtractLocationsProxy::vtkSMExtractLocationsProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMExtractLocationsProxy::~vtkSMExtractLocationsProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::CreateVTKObjects()
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

  // Set default property values for the Selection Source.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("FieldType"));
  ivp->SetElement(0, vtkSelection::CELL);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  ivp->SetElement(0, vtkSelection::LOCATIONS);

  // No need to call UpdateVTKObjects() since it will get called when
  // UpdateVTKObjects() on this proxy is called.
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
