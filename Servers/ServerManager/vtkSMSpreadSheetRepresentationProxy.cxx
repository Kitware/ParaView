/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSpreadSheetRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSpreadSheetRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIdTypeVectorProperty.h"

vtkStandardNewMacro(vtkSMSpreadSheetRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSpreadSheetRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy::vtkSMSpreadSheetRepresentationProxy()
{
  this->SelectionRepresentation = 0;
}

//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy::~vtkSMSpreadSheetRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMSpreadSheetRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->SelectionRepresentation = 
    vtkSMBlockDeliveryRepresentationProxy::SafeDownCast(
      this->GetSubProxy("SelectionRepresentation"));
  if (!this->SelectionRepresentation)
    {
    vtkErrorMacro("SelectionRepresentation must be defined in the xml configuration.");
    return false;
    }
  return true;

}

//----------------------------------------------------------------------------
bool vtkSMSpreadSheetRepresentationProxy::EndCreateVTKObjects()
{
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMSpreadSheetRepresentationProxy::CreatePipeline(
  vtkSMSourceProxy* input, int outputport)
{
  if (!this->Superclass::CreatePipeline(input, outputport))
    {
    return false;
    }

  // Connect the selection output from the input to the SelectionRepresentation.
  
  // Ensure that the source proxy has created extract selection filters.
  input->CreateSelectionProxies();

  vtkSMSourceProxy* esProxy = input->GetSelectionOutput(outputport);
  if (!esProxy)
    {
    vtkErrorMacro("Input proxy does not support selection extraction.");
    return false;
    }

  // esProxy port:1 is a index based vtkSelection. That's the one we are
  // interested in.
  this->Connect(input, this->SelectionRepresentation, "DataInput", 0);
  this->Connect(esProxy, this->SelectionRepresentation, "Input", 1);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::PassEssentialAttributes()
{
  // FIXME: this linking between "this" and "this->SelectionRepresentation"
  // properties has to be managed a bit more gracefully.

  // Pass essential properties to the selection representation
  // such as "BlockSize", "CacheSize", "FieldType".
  const char* pnames[] = {"BlockSize", "CacheSize", "FieldType", 0};
  for (int cc=0; pnames[cc]; cc++)
    {
    vtkSMProperty* src = this->GetProperty(pnames[cc]);
    vtkSMProperty* dest = this->SelectionRepresentation->GetProperty(pnames[cc]);
    dest->Copy(src);
    this->SelectionRepresentation->UpdateProperty(pnames[cc]);
    }

}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
  if (this->SelectionRepresentation->GetVisibility())
    {
    this->PassEssentialAttributes();
    this->SelectionRepresentation->Update(view);
    }
}

//----------------------------------------------------------------------------
vtkSelection* vtkSMSpreadSheetRepresentationProxy::GetSelectionOutput(vtkIdType block)
{
  return vtkSelection::SafeDownCast(this->SelectionRepresentation->GetOutput(block));
}

//----------------------------------------------------------------------------
bool vtkSMSpreadSheetRepresentationProxy::IsSelectionAvailable(vtkIdType blockid)
{
  return this->SelectionRepresentation->IsAvailable(blockid);
}
//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


