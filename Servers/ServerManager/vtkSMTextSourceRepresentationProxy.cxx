/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextSourceRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTextSourceRepresentationProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMTextWidgetRepresentationProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <vtkstd/string>

vtkStandardNewMacro(vtkSMTextSourceRepresentationProxy);
vtkCxxRevisionMacro(vtkSMTextSourceRepresentationProxy, "1.1");
vtkCxxSetObjectMacro(vtkSMTextSourceRepresentationProxy, Input, vtkSMSourceProxy);
//----------------------------------------------------------------------------
vtkSMTextSourceRepresentationProxy::vtkSMTextSourceRepresentationProxy()
{
  this->Input = 0;
  this->Dirty = true;
  this->UpdateSuppressorProxy = 0;
  this->TextWidgetProxy = 0;
  this->CollectProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMTextSourceRepresentationProxy::~vtkSMTextSourceRepresentationProxy()
{
  this->SetInput(0);
  this->UpdateSuppressorProxy = 0;
  this->TextWidgetProxy = 0;
  this->CollectProxy = 0;
}

//----------------------------------------------------------------------------
bool vtkSMTextSourceRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  this->CreateVTKObjects();

  if(!this->TextWidgetProxy->AddToView(view))
    {
    return false;
    }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSMTextSourceRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  if(!this->TextWidgetProxy->RemoveFromView(view))
    {
    return false;
    }
  
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->UpdateSuppressorProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("UpdateSuppressor"));
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::DATA_SERVER
    | vtkProcessModule::CLIENT);

  this->TextWidgetProxy = vtkSMTextWidgetRepresentationProxy::SafeDownCast(
    this->GetSubProxy("TextWidgetRepresentation"));

  if(!this->TextWidgetProxy)
    {
    return;
    }

  this->CollectProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Collect"));
  this->CollectProxy->SetServers(
    vtkProcessModule::DATA_SERVER|vtkProcessModule::CLIENT);

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return;
    }

  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetUpdateNumberOfPieces"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetPartitionId"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetUpdatePiece"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    this->UpdateSuppressorProxy->GetServers(), stream);

  stream << vtkClientServerStream::Invoke
         << this->CollectProxy->GetID() << "SetProcessModuleConnection"
         << pm->GetConnectionClientServerID(this->GetConnectionID())
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
                 this->CollectProxy->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::AddInput(vtkSMSourceProxy* input, 
  const char* vtkNotUsed(method), int vtkNotUsed(hasMultipleInputs))
{
  this->SetInput(input);
  if (!input)
    {
    return;
    }

  input->CreateParts();
  this->CreateVTKObjects();

  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(input);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(this->CollectProxy);
  this->UpdateSuppressorProxy->UpdateVTKObjects();

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_TABLE);
  this->CollectProxy->UpdateVTKObjects();

  this->Dirty = true;
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return;
    }

  if (!this->Dirty)
    {
    return;
    }

  this->Dirty = false;
  this->UpdateSuppressorProxy->InvokeCommand("ForceUpdate");
  this->Superclass::Update(view);

  vtkProcessModule* pm  = vtkProcessModule::GetProcessModule();
  vtkAlgorithm* dp = vtkAlgorithm::SafeDownCast(
    pm->GetObjectFromID(this->CollectProxy->GetID()));

  vtkTable* data = vtkTable::SafeDownCast(dp->GetOutputDataObject(0));
  vtkstd::string text = "";
  if (data->GetNumberOfRows() > 0 && data->GetNumberOfColumns() > 0)
    {
    text = data->GetValue(0, 0).ToString();
    }

  // Now get the text from the Input and set it on the text widget display.
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->TextWidgetProxy->GetProperty("Text"));
  svp->SetElement(0, text.c_str());
  this->TextWidgetProxy->UpdateProperty("Text");

 // this->InvokeEvent(vtkSMViewProxy::ForceUpdateEvent);
}

//-----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::SetUpdateTime(double time)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created!");
    return;
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("UpdateTime"));
  dvp->SetElement(0, time);
  // UpdateTime is immediate update, so no need to update.

  // Go upstream to the reader and mark it modified.
  this->MarkUpstreamModified();
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


