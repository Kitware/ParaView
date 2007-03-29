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

#include <vtkstd/vector>

class vtkSMExtractSelectionProxy::vtkInternal
{
public:

  typedef vtkstd::vector<vtkIdType> IdVectorType;
  IdVectorType Indices;
  IdVectorType GlobalsIDs;
};

vtkStandardNewMacro(vtkSMExtractSelectionProxy);
vtkCxxRevisionMacro(vtkSMExtractSelectionProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMExtractSelectionProxy::vtkSMExtractSelectionProxy()
{
  this->UseGlobalIDs = 0;
  this->SelectionFieldType = vtkSelection::CELL;
  this->Internal = new vtkInternal();
}

//-----------------------------------------------------------------------------
vtkSMExtractSelectionProxy::~vtkSMExtractSelectionProxy()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionProxy::AddIndex(vtkIdType piece, vtkIdType id)
{
  this->Internal->Indices.push_back(piece);
  this->Internal->Indices.push_back(id);
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionProxy::RemoveAllIndices()
{
  this->Internal->Indices.clear();
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionProxy::AddGlobalID(vtkIdType id)
{
  // piece number is not used for global ids.
  this->Internal->GlobalsIDs.push_back(0);
  this->Internal->GlobalsIDs.push_back(id);
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionProxy::RemoveAllGlobalIDs()
{
  this->Internal->GlobalsIDs.clear();
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionProxy::CreateVTKObjects(int num)
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
void vtkSMExtractSelectionProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkSMProxy* selectionSource = this->GetSubProxy("SelectionSource");
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }

  vtkSMIdTypeVectorProperty* idvp = vtkSMIdTypeVectorProperty::SafeDownCast(
    selectionSource->GetProperty("IDs"));
  if (this->UseGlobalIDs)
    {
    idvp->SetNumberOfElements(this->Internal->GlobalsIDs.size());
    if (this->Internal->GlobalsIDs.size() > 0)
      {
      idvp->SetElements(&this->Internal->GlobalsIDs[0]);
      }
    }
  else
    {
    idvp->SetNumberOfElements(this->Internal->Indices.size());
    if (this->Internal->Indices.size() > 0)
      {
      idvp->SetElements(&this->Internal->Indices[0]);
      }
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("FieldType"));
  ivp->SetElement(0, this->SelectionFieldType);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  if (this->UseGlobalIDs)
    {
    ivp->SetElement(0, vtkSelection::GLOBALIDS);
    }
  else
    {
    ivp->SetElement(0, vtkSelection::INDICES);
    }
  selectionSource->UpdateVTKObjects();
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
}
