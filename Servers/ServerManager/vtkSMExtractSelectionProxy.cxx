/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractSelectionProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractSelectionProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSelection.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkSMExtractSelectionProxy);
vtkCxxRevisionMacro(vtkSMExtractSelectionProxy, "1.7");
//-----------------------------------------------------------------------------
vtkSMExtractSelectionProxy::vtkSMExtractSelectionProxy()
{
  this->UseGlobalIDs = 0;
  this->PrevUseGlobalIDs = 0;
  this->SelectionFieldType = vtkSelection::CELL;
}

//-----------------------------------------------------------------------------
vtkSMExtractSelectionProxy::~vtkSMExtractSelectionProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionProxy::CreateVTKObjects()
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

  vtkSMSourceProxy* selectionSourceID = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("SelectionSourceID"));
  if (!selectionSourceID)
    {
    vtkErrorMacro("Missing subproxy: SelectionSourceID");
    return;
    }

  vtkSMSourceProxy* selectionSourceGID = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("SelectionSourceGID"));
  if (!selectionSourceGID)
    {
    vtkErrorMacro("Missing subproxy: SelectionSourceGID");
    return;
    }

  // Set field type.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSourceID->GetProperty("FieldType"));
  ivp->SetElement(0, this->SelectionFieldType);
  selectionSourceID->UpdateVTKObjects();

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSourceGID->GetProperty("FieldType"));
  ivp->SetElement(0, this->SelectionFieldType);
  selectionSourceGID->UpdateVTKObjects();

  if (this->UseGlobalIDs)
    {
    this->AddInput(selectionSourceGID, "SetSelectionConnection");
    }
  else
    {
    this->AddInput(selectionSourceID, "SetSelectionConnection");
    }
  this->PrevUseGlobalIDs = this->UseGlobalIDs;
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  if (this->PrevUseGlobalIDs != this->UseGlobalIDs)
    {
    vtkSMSourceProxy* selectionSourceID = vtkSMSourceProxy::SafeDownCast(
      this->GetSubProxy("SelectionSourceID"));
    if (!selectionSourceID)
      {
      vtkErrorMacro("Missing subproxy: SelectionSourceID");
      return;
      }

    vtkSMSourceProxy* selectionSourceGID = vtkSMSourceProxy::SafeDownCast(
      this->GetSubProxy("SelectionSourceGID"));
    if (!selectionSourceGID)
      {
      vtkErrorMacro("Missing subproxy: SelectionSourceGID");
      return;
      }

    if (this->UseGlobalIDs)
      {
      this->AddInput(selectionSourceGID, "SetSelectionConnection");
      }
    else
      {
      this->AddInput(selectionSourceID, "SetSelectionConnection");
      }

    this->PrevUseGlobalIDs = this->UseGlobalIDs;
    }
}

//-----------------------------------------------------------------------------
int vtkSMExtractSelectionProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }
  const char* type = element->GetAttribute("selection_field_type");
  if (type && strcmp(type,"POINT") == 0)
    {
    this->SelectionFieldType  = vtkSelection::POINT;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseGlobalIDs: " << this->UseGlobalIDs << endl;
  os << indent << "SelectionFieldType: " << this->SelectionFieldType << endl;
}
