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

#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSelection.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSpreadSheetRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy::vtkSMSpreadSheetRepresentationProxy()
{
  this->SelectionRepresentation = 0;
  this->SelectionOnly = 0;
}

//----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy::~vtkSMSpreadSheetRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::InvokeStartEvent()
{
  this->InvokeEvent(vtkCommand::StartEvent);
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::InvokeEndEvent()
{
  this->InvokeEvent(vtkCommand::EndEvent);
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

  // Relay StartEvent|EndEvent fired by the internal selection representation.
  vtkCommand* adapter = vtkMakeMemberFunctionCommand(*this,
    &vtkSMSpreadSheetRepresentationProxy::InvokeStartEvent);
  this->SelectionRepresentation->AddObserver(vtkCommand::StartEvent, adapter);
  adapter->Delete();

  adapter = vtkMakeMemberFunctionCommand(*this,
    &vtkSMSpreadSheetRepresentationProxy::InvokeEndEvent);
  this->SelectionRepresentation->AddObserver(vtkCommand::EndEvent, adapter);
  adapter->Delete();

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
  // FIXME switch from this->PreProcesser to this->Streamer
  this->Connect(this->Streamer, this->SelectionRepresentation, "DataInput", 0);
  this->Connect(esProxy, this->SelectionRepresentation, "Input", 1);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::PassEssentialAttributes()
{
  // FIXME: this linking between "this" and "this->SelectionRepresentation"
  // properties has to be managed a bit more gracefully.

  // Pass essential properties to the selection representation
  // such as "BlockSize", "CacheSize", "FieldAssociation"
  const char* pnames[] =
    {"CacheSize", "FieldAssociation", 0}; // removed: "BlockSize",
  for (int cc=0; pnames[cc]; cc++)
    {
    vtkSMProperty* src = this->GetProperty(pnames[cc]);
    vtkSMProperty* dest = this->SelectionRepresentation->GetProperty(pnames[cc]);
    if (src->GetMTime() > dest->GetMTime())
      {
      // Otherwise Copy() call Modified() every time and the spreadsheet view
      // then needs to update everything since is thinks a property on the
      // repsentation has changed.
      dest->Copy(src);
      this->SelectionRepresentation->UpdateProperty(pnames[cc]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::SetSelectionOnly(int newvalue)
{
  if (this->SelectionOnly == newvalue)
    {
    return;
    }
  this->SelectionOnly = newvalue;
  if (this->SelectionOnly)
    {
    this->Connect(
      this->GetInputProxy()->GetSelectionOutput(this->OutputPort),
      this->PreProcessor);
    vtkSMPropertyHelper(this->PreProcessor, "GenerateOriginalIds").Set(0);
    }
  else
    {
    this->Connect(this->GetInputProxy(),
      this->PreProcessor, "Input", this->OutputPort);
    vtkSMPropertyHelper(this->PreProcessor, "GenerateOriginalIds").Set(1);
    }
  this->PreProcessor->UpdateVTKObjects();
  this->Modified();
  this->MarkModified(NULL);
}

//----------------------------------------------------------------------------
void vtkSMSpreadSheetRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->UpdateRequired())
    {
    // this is essential since this proxy's real input is sometimes the extract
    // selection proxy, in which case since the representation is not a true
    // consumer of the extract selection proxy, it does not realize that the
    // pipeline has changed when selection changes. BUG #10479.
    this->NeedsUpdate = true;
    }
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
  os << indent << "SelectionOnly: " << (this->SelectionOnly? "On" : "Off")
    << endl;
}


