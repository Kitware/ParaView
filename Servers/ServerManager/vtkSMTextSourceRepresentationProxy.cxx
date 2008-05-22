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
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMTextWidgetRepresentationProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkInformation.h"

#include <vtkstd/string>

vtkStandardNewMacro(vtkSMTextSourceRepresentationProxy);
vtkCxxRevisionMacro(vtkSMTextSourceRepresentationProxy, "1.7");
//----------------------------------------------------------------------------
vtkSMTextSourceRepresentationProxy::vtkSMTextSourceRepresentationProxy()
{
  this->Dirty = true;
  this->UpdateSuppressorProxy = 0;
  this->TextWidgetProxy = 0;
  this->CollectProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMTextSourceRepresentationProxy::~vtkSMTextSourceRepresentationProxy()
{
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
bool vtkSMTextSourceRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->UpdateSuppressorProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("UpdateSuppressor"));
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::DATA_SERVER
    | vtkProcessModule::CLIENT);

  this->TextWidgetProxy = vtkSMTextWidgetRepresentationProxy::SafeDownCast(
    this->GetSubProxy("TextWidgetRepresentation"));

  if(!this->TextWidgetProxy)
    {
    return false;
    }

  this->CollectProxy = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Collect"));
  this->CollectProxy->SetServers(
    vtkProcessModule::DATA_SERVER|vtkProcessModule::CLIENT);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTextSourceRepresentationProxy::EndCreateVTKObjects()
{
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
  
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::AddInput(unsigned int inputPort,
                                                  vtkSMSourceProxy* input,
                                                  unsigned int outputPort,
                                                  const char* method)
{
  this->Superclass::AddInput(inputPort, input, outputPort, method);

  vtkSMInputProperty* ip;
  ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddInputConnection(input, outputPort);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_TABLE);
  this->CollectProxy->UpdateVTKObjects();

  // It is essential to update the CollectProxy before connecting it in the
  // pipeline since we changed the OutputDataType.

  ip = vtkSMInputProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->CollectProxy);
  this->UpdateSuppressorProxy->UpdateVTKObjects();
  

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

  // check if we should UseCache

  if (this->ViewInformation->Has(vtkSMViewProxy::USE_CACHE()))
    {
    if(this->ViewInformation->Get(vtkSMViewProxy::USE_CACHE())>0)
      {
      if (this->ViewInformation->Has(vtkSMViewProxy::CACHE_TIME()))
        {
        vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
          this->UpdateSuppressorProxy->GetProperty("CacheUpdate"));
        dvp->SetElement(0, this->ViewInformation->Get(vtkSMViewProxy::CACHE_TIME()));
        this->UpdateSuppressorProxy->UpdateProperty("CacheUpdate", 1);
        return;
        }
      }
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

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy != this)
    {
    this->Dirty = true;
    }

  this->Superclass::MarkDirty(modifiedProxy);
}

//-----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::SetUpdateTimeInternal(double time)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("UpdateTime"));
  dvp->SetElement(0, time);

  // Calls MarkUpstreamModified().
  this->Superclass::SetUpdateTimeInternal(time);
}

//----------------------------------------------------------------------------
void vtkSMTextSourceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


