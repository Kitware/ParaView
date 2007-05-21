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
#include "vtkPVXMLElement.h"
#include "vtkSelection.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkSMExtractLocationsProxy);
vtkCxxRevisionMacro(vtkSMExtractLocationsProxy, "1.2.2.1");
//-----------------------------------------------------------------------------
vtkSMExtractLocationsProxy::vtkSMExtractLocationsProxy()
{
  this->Locations = NULL;
}

//-----------------------------------------------------------------------------
vtkSMExtractLocationsProxy::~vtkSMExtractLocationsProxy()
{
  if (this->Locations)
    {
    this->Locations->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::AddLocation(double x, double y, double z)
{
  if (this->Locations == NULL)
    {
    this->Locations = vtkDoubleArray::New();
    this->Locations->SetNumberOfComponents(3);
    this->Locations->SetNumberOfTuples(0);
    }
  this->Locations->InsertNextTuple3(x,y,z);
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::RemoveAllLocations()
{
  if (this->Locations)
    {
    this->Locations->Reset();
    }
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::CreateVTKObjects(int num)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects(num);

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
  
  this->AddInput(selectionSource, "SetSelectionConnection", false);
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkSMProxy* selectionSource = this->GetSubProxy("SelectionSource");
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    selectionSource->GetProperty("Locations"));
  int nlocations = 0;
  if (this->Locations)
    {
    nlocations = this->Locations->GetNumberOfTuples();
    }
  dvp->SetNumberOfElements(nlocations*3);
  if (nlocations)
    {
    dvp->SetElements((double*)this->Locations->GetVoidPointer(0));
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("FieldType"));
  ivp->SetElement(0, vtkSelection::CELL);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  ivp->SetElement(0, vtkSelection::LOCATIONS);

  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
