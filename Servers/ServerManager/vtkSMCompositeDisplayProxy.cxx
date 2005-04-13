/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositeDisplayProxy.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkPVProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkPVOptions.h"
vtkStandardNewMacro(vtkSMCompositeDisplayProxy);
vtkCxxRevisionMacro(vtkSMCompositeDisplayProxy, "1.1.2.2");
//-----------------------------------------------------------------------------
vtkSMCompositeDisplayProxy::vtkSMCompositeDisplayProxy()
{
  this->CollectProxy = 0;
  this->LODCollectProxy = 0;

  // When created, collection is off.
  // I set these to -1 to ensure the decision is propagated.
  this->CollectionDecision = -1;
  this->LODCollectionDecision = -1;
}

//-----------------------------------------------------------------------------
vtkSMCompositeDisplayProxy::~vtkSMCompositeDisplayProxy()
{
  this->CollectProxy = 0;
  this->LODCollectProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->CollectProxy = this->GetSubProxy("Collect");
  this->LODCollectProxy = this->GetSubProxy("LODCollect");

  if (!this->CollectProxy)
    {
    vtkErrorMacro("Failed to find SubProxy Collect.");
    return;
    }

  if (!this->LODCollectProxy)
    {
    vtkErrorMacro("Failed to find SubProxy LODCollect.");
    return;
    }
  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->LODCollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  
  this->Superclass::CreateVTKObjects(numObjects);

  if (!this->ObjectsCreated)
    {
    return;
    }


}
//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupDefaults()
{
  this->Superclass::SetupDefaults();
  this->SetupCollectionFilter(this->CollectProxy);
  this->SetupCollectionFilter(this->LODCollectProxy);

  for (unsigned int i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream cmd;
    vtkClientServerStream stream;
    vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
      vtkProcessModule::GetProcessModule());

    cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Collect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "AddObserver" << "StartEvent" << cmd
      << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Collect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "AddObserver" << "EndEvent" << cmd
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent" << "Execute LODCollect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID(i) << "AddObserver" << "StartEvent" << cmd
      << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent" << "Execute LODCollect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID(i) << "AddObserver" << "EndEvent" << cmd
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    // Handle collection setup with client server.
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    // Special condition to signal the client.
    // Because both processes of the Socket controller think they are 0!!!!
    if (pm->GetClientMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "SetController" << 0
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->LODCollectProxy->GetID(i) << "SetController" << 0
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT, stream);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupPipeline()
{
  this->Superclass::SetupPipeline();
  vtkSMInputProperty* ip = 0;
  vtkClientServerStream stream;
  
  for (unsigned int i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    if (this->CollectProxy)
      {
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "GetPolyDataOutput"
        << vtkClientServerStream::End
        << vtkClientServerStream::Invoke
        << this->UpdateSuppressorProxy->GetID(i) << "SetInput"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      }

    if (this->LODCollectProxy)
      {
      stream
        << vtkClientServerStream::Invoke
        << this->LODCollectProxy->GetID(i) << "GetPolyDataOutput"
        << vtkClientServerStream::End
        << vtkClientServerStream::Invoke
        << this->LODUpdateSuppressorProxy->GetID(i) << "SetInput"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
    }
  
  ip = vtkSMInputProperty::SafeDownCast(
    this->LODCollectProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->LODDecimatorProxy);

  ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->GeometryFilterProxy);

  this->LODCollectProxy->UpdateVTKObjects();
  this->CollectProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupCollectionFilter(vtkSMProxy* collectProxy)
{ 
  vtkPVProcessModule* pm = 
    vtkPVProcessModule::SafeDownCast(vtkProcessModule::GetProcessModule());

  int i, num;
  
  vtkClientServerStream stream;

  num = collectProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    // Default is pass through because it executes fastest.  
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMoveModeToPassThrough"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMPIMToNSocketConnection" 
      << pm->GetMPIMToNSocketConnectionID()
      << vtkClientServerStream::End;
    // create, SetPassThrough, and set the mToN connection
    // object on all servers and client
    pm->SendStream(
      vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);
    // always set client mode
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT, stream);
    // if running in client mode
    // then set the server to be servermode
    if(pm->GetClientMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << collectProxy->GetID(i) << "SetServerToDataServer"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      }
    // if running in render server mode
    if(pm->GetOptions()->GetRenderServerMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << collectProxy->GetID(i) << "SetServerToRenderServer"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
      }
    }
}

//-----------------------------------------------------------------------------
vtkPVLODPartDisplayInformation* vtkSMCompositeDisplayProxy::GetLODInformation()
{
  if (!this->ObjectsCreated)
    {
    return 0;
    }
  if ( ! this->GeometryIsValid)
    { // Update but with collection filter off.
    this->CollectionDecision = 0;
    this->LODCollectionDecision = 0;
    this->LODInformationIsValid = 0;

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->CollectProxy->GetProperty("MoveMode"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property MoveMode on CollectProxy.");
      return 0;
      }
    ivp->SetElement(0, 0); // Pass Through.
    vtkSMProperty *p = this->UpdateSuppressorProxy->GetProperty("ForceUpdate");
    if (!p)
      {
      vtkErrorMacro("Failed to find property ForceUpdate on UpdateSuppressorProxy.");
      return 0;
      }
    p->Modified();
    this->UpdateVTKObjects();
    }
  return this->Superclass::GetLODInformation();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetCollectionDecision(int v)
{
  if (v == this->CollectionDecision)
    {
    return;
    }
  this->CollectionDecision = v;
  // TODO: old codes only supports mode PassThru and Collect. Why?
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("MoveMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property MoveMode on CollectProxy.");
    return;
    }
  ivp->SetElement(0, this->CollectionDecision);
  this->InvalidateGeometryInternal();
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetLODCollectionDecision(int v)
{
  if (!this->ObjectsCreated || v == this->LODCollectionDecision)
    {
    return;
    }
  this->LODCollectionDecision = v;
  // TODO: old codes only supports mode PassThru and Clone. Why?
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LODCollectProxy->GetProperty("MoveMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property MoveMode on LODCollectProxy.");
    return;
    }
  if (!this->LODCollectionDecision)
    {
    ivp->SetElement(0, this->LODCollectionDecision);
    }
  else
    {
    ivp->SetElement(0, 2);
    }
  //ivp->SetElement(0, this->LODCollectionDecision);
  this->InvalidateLODGeometry();
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CollectionDecision: " << this->CollectionDecision << endl;
  os << indent << "LODCollectionDecision: " << this->LODCollectionDecision 
    << endl;
}
