/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTextDisplayProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMTextWidgetDisplayProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTable.h"
#include "vtkVariant.h"

#include <vtkstd/string>

vtkStandardNewMacro(vtkSMTextDisplayProxy);
vtkCxxRevisionMacro(vtkSMTextDisplayProxy, "1.7");
//----------------------------------------------------------------------------
vtkSMTextDisplayProxy::vtkSMTextDisplayProxy()
{
  this->Dirty = true;
  this->UpdateSuppressorProxy = 0;
  this->TextWidgetProxy = 0;
  this->CollectProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMTextDisplayProxy::~vtkSMTextDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->TextWidgetProxy = 0;
  this->CollectProxy = 0;
}

//----------------------------------------------------------------------------
void vtkSMTextDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->CreateVTKObjects();

  this->TextWidgetProxy->AddToRenderModule(rm);
  this->Superclass::AddToRenderModule(rm);
}

//----------------------------------------------------------------------------
void vtkSMTextDisplayProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->TextWidgetProxy->RemoveFromRenderModule(rm);
  this->Superclass::RemoveFromRenderModule(rm);
}

//----------------------------------------------------------------------------
void vtkSMTextDisplayProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->UpdateSuppressorProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("UpdateSuppressor"));
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::DATA_SERVER
    | vtkProcessModule::CLIENT);

  this->TextWidgetProxy = vtkSMTextWidgetDisplayProxy::SafeDownCast(
    this->GetSubProxy("TextWidget"));

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
void vtkSMTextDisplayProxy::AddInput(unsigned int,
                                     vtkSMSourceProxy* input,
                                     unsigned int outputPort,
                                     const char*)
{
  if (!input)
    {
    return;
    }

  input->CreateParts();
  this->CreateVTKObjects();

  vtkSMInputProperty* ip;
  ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddInputConnection(input, outputPort);

  ip = vtkSMInputProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->CollectProxy);
  this->UpdateSuppressorProxy->UpdateVTKObjects();

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_TABLE);
  this->CollectProxy->UpdateVTKObjects();

  this->Dirty = true;
}

//----------------------------------------------------------------------------
void vtkSMTextDisplayProxy::Update(vtkSMAbstractViewModuleProxy* view)
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

  this->InvokeEvent(vtkSMAbstractDisplayProxy::ForceUpdateEvent);
}

//-----------------------------------------------------------------------------
void vtkSMTextDisplayProxy::SetUpdateTime(double time)
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
void vtkSMTextDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


