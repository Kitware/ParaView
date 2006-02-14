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

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMCompositeDisplayProxy);
vtkCxxRevisionMacro(vtkSMCompositeDisplayProxy, "1.16");
//-----------------------------------------------------------------------------
vtkSMCompositeDisplayProxy::vtkSMCompositeDisplayProxy()
{
  this->CollectProxy = 0;
  this->LODCollectProxy = 0;
  this->VolumeCollectProxy = 0;

  this->DistributorProxy = 0;
  this->LODDistributorProxy = 0;
  this->VolumeDistributorProxy = 0;

  this->DistributorSuppressorProxy = 0;
  this->LODDistributorSuppressorProxy = 0;
  this->VolumeDistributorSuppressorProxy = 0;

  // When created, collection is off.
  // I set these to -1 to ensure the decision is propagated.
  this->CollectionDecision = -1;
  this->LODCollectionDecision = -1;

  this->DistributedGeometryIsValid = 0;
  this->DistributedLODGeometryIsValid = 0;
  this->DistributedVolumeGeometryIsValid = 0;

  this->OrderedCompositing = -1;
  this->OrderedCompositingTree = NULL;
}

//-----------------------------------------------------------------------------
vtkSMCompositeDisplayProxy::~vtkSMCompositeDisplayProxy()
{
  this->SetOrderedCompositingTreeInternal(NULL);

  this->CollectProxy = 0;
  this->LODCollectProxy = 0;
  this->VolumeCollectProxy = 0;

  this->DistributorProxy = 0;
  this->LODDistributorProxy = 0;
  this->VolumeDistributorProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated || !this->CanCreateProxy)
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

  this->DistributorProxy = this->GetSubProxy("Distributor");
  this->LODDistributorProxy = this->GetSubProxy("LODDistributor");

  if (!this->DistributorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy Distributor.");
    return;
    }
  if (!this->LODDistributorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy LODDistributor.");
    return;
    }

  this->DistributorProxy->SetServers(vtkProcessModule::RENDER_SERVER);
  this->LODDistributorProxy->SetServers(vtkProcessModule::RENDER_SERVER);

  this->DistributorSuppressorProxy
    = this->GetSubProxy("DistributorSuppressor");
  this->LODDistributorSuppressorProxy
    = this->GetSubProxy("LODDistributorSuppressor");

  if (!this->DistributorSuppressorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy DistributorSuppressor.");
    return;
    }
  if (!this->LODDistributorSuppressorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy LODDistributorSuppressor.");
    return;
    }

  this->DistributorSuppressorProxy->SetServers(
                                          vtkProcessModule::CLIENT_AND_SERVERS);
  this->LODDistributorSuppressorProxy->SetServers(
                                          vtkProcessModule::CLIENT_AND_SERVERS);
  
  if (this->HasVolumePipeline)
    {
    this->VolumeCollectProxy = this->GetSubProxy("VolumeCollect");

    if (!this->VolumeCollectProxy)
      {
      vtkErrorMacro("Failed to find SubProxy VolumeCollect.");
      return;
      }

    this->VolumeCollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

    this->VolumeDistributorProxy = this->GetSubProxy("VolumeDistributor");

    if (!this->VolumeDistributorProxy)
      {
      vtkErrorMacro("Failed to find SubProxy VolumeDistributor.");
      return;
      }

    this->VolumeDistributorProxy->SetServers(vtkProcessModule::RENDER_SERVER);

    this->VolumeDistributorSuppressorProxy
      = this->GetSubProxy("VolumeDistributorSuppressor");

    if (!this->VolumeDistributorSuppressorProxy)
      {
      vtkErrorMacro("Failed to find SubProxy VolumeDistributorSuppressor.");
      return;
      }

    this->VolumeDistributorSuppressorProxy->SetServers(
                                          vtkProcessModule::CLIENT_AND_SERVERS);
    }
  else
    {
    this->RemoveSubProxy("VolumeCollect");
    this->RemoveSubProxy("VolumeDistributor");
    this->RemoveSubProxy("VolumeDistributorSuppressor");
    }

  this->Superclass::CreateVTKObjects(numObjects);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupDefaults()
{
  unsigned int i;

  this->Superclass::SetupDefaults();
  this->SetupCollectionFilter(this->CollectProxy);
  this->SetupCollectionFilter(this->LODCollectProxy);

  for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream cmd;
    vtkClientServerStream stream;
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

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
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT_AND_SERVERS, stream);

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
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT_AND_SERVERS, stream);

    // Handle collection setup with client server.
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << pm->GetConnectionClientServerID(this->ConnectionID)
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << pm->GetConnectionClientServerID(this->ConnectionID)
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT_AND_SERVERS, stream);

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
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::CLIENT, stream);
      }
    }

  this->SetOrderedCompositing(0);

  for (i=0; i < this->DistributorProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream cmd;
    vtkClientServerStream stream;
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent"
        << "Execute OrderedCompositeDistribute"
        << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->DistributorProxy->GetID(i) << "AddObserver"
           << "StartEvent" << cmd
           << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent"
        << "Execute OrderedCompositeDistribute"
        << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->DistributorProxy->GetID(i) << "AddObserver"
           << "EndEvent" << cmd
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);

    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent"
        << "Execute LODOrderedCompositeDistribute"
        << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODDistributorProxy->GetID(i) << "AddObserver"
           << "StartEvent" << cmd
           << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent"
        << "Execute LODOrderedCompositeDistribute"
        << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODDistributorProxy->GetID(i) << "AddObserver"
           << "EndEvent" << cmd
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      vtkProcessModule::RENDER_SERVER, stream);

    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetController"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->DistributorProxy->GetID(i) << "SetController"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetController"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODDistributorProxy->GetID(i) << "SetController"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, 
      stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupPipeline()
{
  unsigned int i;

  this->Superclass::SetupPipeline();
  vtkSMInputProperty* ip = 0;
  vtkSMStringVectorProperty *svp = 0;
  vtkClientServerStream stream;

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
  
  for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
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
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::CLIENT_AND_SERVERS, stream);
    }

  // On the render server, insert a distributor.
  ip = vtkSMInputProperty::SafeDownCast(
                                  this->DistributorProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->UpdateSuppressorProxy);
  this->DistributorProxy->UpdateVTKObjects();

  ip = vtkSMInputProperty::SafeDownCast(
                               this->LODDistributorProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->LODUpdateSuppressorProxy);
  this->LODDistributorProxy->UpdateVTKObjects();

  // On the render server, attach an update suppressor to the distributor.  On
  // the client side (since the distributor is not there) attach it to the other
  // update suppressor.  We cannot do this through the server manager interface.
  for (i = 0; i < this->DistributorProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke
           << this->UpdateSuppressorProxy->GetID(i) << "GetOutputPort" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->DistributorSuppressorProxy->GetID(i)
           << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke
           << this->LODUpdateSuppressorProxy->GetID(i) << "GetOutputPort" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODDistributorSuppressorProxy->GetID(i)
           << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER, stream);
    }

  for (i = 0; i < this->DistributorProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke
           << this->DistributorProxy->GetID(i) << "GetOutputPort" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->DistributorSuppressorProxy->GetID(i)
           << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;

    stream << vtkClientServerStream::Invoke
           << this->LODDistributorProxy->GetID(i) << "GetOutputPort" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODDistributorSuppressorProxy->GetID(i)
           << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);
    }

  ip = vtkSMInputProperty::SafeDownCast(
                                       this->MapperProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->DistributorSuppressorProxy);
  this->MapperProxy->UpdateVTKObjects();

  ip = vtkSMInputProperty::SafeDownCast(
                                    this->LODMapperProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->LODDistributorSuppressorProxy);
  this->LODMapperProxy->UpdateVTKObjects();

  svp = vtkSMStringVectorProperty::SafeDownCast(
                             this->DistributorProxy->GetProperty("OutputType"));
  svp->SetElement(0, "vtkPolyData");

  svp = vtkSMStringVectorProperty::SafeDownCast(
                          this->LODDistributorProxy->GetProperty("OutputType"));
  svp->SetElement(0, "vtkPolyData");

  this->DistributorProxy->UpdateVTKObjects();
  this->LODDistributorProxy->UpdateVTKObjects();

  svp = vtkSMStringVectorProperty::SafeDownCast(
                   this->DistributorSuppressorProxy->GetProperty("OutputType"));
  svp->SetElement(0, "vtkPolyData");
  svp = vtkSMStringVectorProperty::SafeDownCast(
                this->LODDistributorSuppressorProxy->GetProperty("OutputType"));
  svp->SetElement(0, "vtkPolyData");
  this->DistributorSuppressorProxy->UpdateVTKObjects();
  this->LODDistributorSuppressorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupVolumePipeline()
{
  if (!this->HasVolumePipeline)
    {
    return;
    }

  this->Superclass::SetupVolumePipeline();

  vtkSMInputProperty *ip;
  vtkSMStringVectorProperty *svp;
  vtkClientServerStream stream;

  ip = vtkSMInputProperty::SafeDownCast(
                                this->VolumeCollectProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeFilterProxy);

  this->VolumeCollectProxy->UpdateVTKObjects();

  unsigned int i;
  for (i = 0; i < this->VolumeCollectProxy->GetNumberOfIDs(); i++)
    {
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID(i) << "GetUnstructuredGridOutput"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeUpdateSuppressorProxy->GetID(i) << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT_AND_SERVERS, stream);
    }

  // On the render server, insert a distributor.
  ip = vtkSMInputProperty::SafeDownCast(
                            this->VolumeDistributorProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeUpdateSuppressorProxy);
  this->VolumeDistributorProxy->UpdateVTKObjects();

  // On the render server, attach an update suppressor to the distributor.  On
  // the client side (since the distributor is not there) attach it to the other
  // update suppressor.  We cannot do this through the server manager interface.
  for (i = 0; i < this->DistributorProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke
           << this->VolumeUpdateSuppressorProxy->GetID(i)<< "GetOutputPort" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->VolumeDistributorSuppressorProxy->GetID(i)
           << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER, stream);
    }

  for (i = 0; i < this->DistributorProxy->GetNumberOfIDs(); i++)
    {
    stream << vtkClientServerStream::Invoke
           << this->VolumeDistributorProxy->GetID(i) << "GetOutputPort" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->VolumeDistributorSuppressorProxy->GetID(i)
           << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);
    }

  ip = vtkSMInputProperty::SafeDownCast(
                               this->VolumePTMapperProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeDistributorSuppressorProxy);
  this->VolumePTMapperProxy->UpdateVTKObjects();

  ip = vtkSMInputProperty::SafeDownCast(
                            this->VolumeBunykMapperProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeDistributorSuppressorProxy);
  this->VolumeBunykMapperProxy->UpdateVTKObjects();

  ip = vtkSMInputProperty::SafeDownCast(
                           this->VolumeZSweepMapperProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeDistributorSuppressorProxy);
  this->VolumeZSweepMapperProxy->UpdateVTKObjects();

  svp = vtkSMStringVectorProperty::SafeDownCast(
                       this->VolumeDistributorProxy->GetProperty("OutputType"));
  svp->SetElement(0, "vtkUnstructuredGrid");

  this->VolumeDistributorProxy->UpdateVTKObjects();

  svp = vtkSMStringVectorProperty::SafeDownCast(
             this->VolumeDistributorSuppressorProxy->GetProperty("OutputType"));
  svp->SetElement(0, "vtkUnstructuredGrid");
  this->VolumeDistributorSuppressorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupVolumeDefaults()
{
  if (!this->HasVolumePipeline)
    {
    return;
    }
  this->Superclass::SetupVolumeDefaults();

  this->SetupCollectionFilter(this->VolumeCollectProxy);

  unsigned int i;
  for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream cmd;
    vtkClientServerStream stream;
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent"
        << "Execute VolumeCollect"
        << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID(i) << "AddObserver" << "StartEvent"
      << cmd << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent"
        << "Execute VolumeCollect"
        << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID(i) << "AddObserver" << "EndEvent"
      << cmd << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT_AND_SERVERS, stream);

    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << pm->GetConnectionClientServerID(this->ConnectionID)
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER_ROOT|
      vtkProcessModule::RENDER_SERVER_ROOT, stream);


    // Special condition to signal the client.
    // Because both processes of the Socket controller think they are 0!!!!
    if (pm->GetClientMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << this->VolumeCollectProxy->GetID(i) << "SetController" << 0
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::CLIENT, stream);
      }
    }

  for (i=0; i < this->DistributorProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream cmd;
    vtkClientServerStream stream;
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent"
        << "Execute LODOrderedCompositeDistribute"
        << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->VolumeDistributorProxy->GetID(i) << "AddObserver"
           << "StartEvent" << cmd
           << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent"
        << "Execute LODOrderedCompositeDistribute"
        << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->VolumeDistributorProxy->GetID(i) << "AddObserver"
           << "EndEvent" << cmd
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);

    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetController"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->VolumeDistributorProxy->GetID(i) << "SetController"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupCollectionFilter(vtkSMProxy* collectProxy)
{ 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

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
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT_AND_SERVERS, stream);
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMPIMToNSocketConnection" 
      << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
      << vtkClientServerStream::End;
    // create, SetPassThrough, and set the mToN connection
    // object on all servers and client
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);
    // always set client mode
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
      vtkProcessModule::CLIENT, stream);
    // if running in client mode
    // then set the server to be servermode
    if(pm->GetClientMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << collectProxy->GetID(i) << "SetServerToDataServer"
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::DATA_SERVER, stream);
      }
    // if running in render server mode
    if(pm->GetOptions()->GetRenderServerMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << collectProxy->GetID(i) << "SetServerToRenderServer"
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID,
        vtkProcessModule::RENDER_SERVER, stream);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetInputInternal(vtkSMSourceProxy *input)
{
  this->Superclass::SetInputInternal(input);

  if (this->HasVolumePipeline)
    {
    // Find out how many processes are in the render server.  This should really
    // be done by the process module itself.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetNumberOfPartitions"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
    int numRenderServerProcs;
    if (!pm->GetLastResult(this->ConnectionID,
                           vtkProcessModule::RENDER_SERVER)
        .GetArgument(0, 0, &numRenderServerProcs))
      {
      vtkErrorMacro("Could not get the size of the render server.");
      }

    vtkIdType numCells = input->GetDataInformation()->GetNumberOfCells();
    if (numCells/numRenderServerProcs < 1000000)
      {
      this->SupportsZSweepMapper = 1;
      }
    if (numCells/numRenderServerProcs < 500000)
      {
      this->SupportsBunykMapper = 1;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::InvalidateDistributedGeometry()
{
  this->DistributedGeometryIsValid = 0;
  this->DistributedLODGeometryIsValid = 0;
  this->DistributedVolumeGeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
int vtkSMCompositeDisplayProxy::IsDistributedGeometryValid()
{
  if (this->VolumeRenderMode)
    {
    return (   this->DistributedVolumeGeometryIsValid
            && this->VolumeGeometryIsValid );
    }
  else
    {
    return (this->DistributedGeometryIsValid && this->GeometryIsValid);
    }
}

//-----------------------------------------------------------------------------
vtkPVLODPartDisplayInformation* vtkSMCompositeDisplayProxy::GetLODInformation()
{
  if (!this->ObjectsCreated)
    {
    return 0;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!this->LODGeometryIsValid &&
      (pm->GetOptions()->GetClientMode() || pm->GetNumberOfPartitions() >= 2))
    { 
    // Update but with collection filter off.
    this->CollectionDecision = 0;
    this->LODCollectionDecision = 0;

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
      vtkErrorMacro("Failed to find property ForceUpdate on "
                    "UpdateSuppressorProxy.");
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
  if (v == this->CollectionDecision || !this->CollectProxy)
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
  this->InvalidateGeometryInternal(0);
  this->UpdateVTKObjects();

  // Changing the collection decision can change the ordered compositing
  // distribution.
  int oc = this->OrderedCompositing;
  this->OrderedCompositing = 0;
  this->SetOrderedCompositing(oc);
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
  this->InvalidateLODGeometry(0);
  this->UpdateVTKObjects();

  // Changing the collection decision can change the ordered compositing
  // distribution.
  int oc = this->OrderedCompositing;
  this->OrderedCompositing = 0;
  this->SetOrderedCompositing(oc);
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::SetOrderedCompositing(int val)
{
  if (!this->ObjectsCreated || (this->OrderedCompositing == val))
    {
    return;
    }

  this->OrderedCompositing = val;

  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
                            this->DistributorProxy->GetProperty("PassThrough"));
  ivp->SetElements1(!this->OrderedCompositing || this->CollectionDecision);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
                         this->LODDistributorProxy->GetProperty("PassThrough"));
  ivp->SetElements1(!this->OrderedCompositing || this->LODCollectionDecision);

  if (this->VolumeDistributorProxy)
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
                      this->VolumeDistributorProxy->GetProperty("PassThrough"));
    ivp->SetElements1(!this->OrderedCompositing);
    }

  this->UpdateVTKObjects();

  this->InvalidateDistributedGeometry();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetOrderedCompositingTreeInternal(
  vtkSMProxy* tree)
{
  vtkSetObjectBodyMacro(OrderedCompositingTree, vtkSMProxy, tree);
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::SetOrderedCompositingTree(vtkSMProxy *tree)
{
  if (this->OrderedCompositingTree == tree)
    {
    return;
    }

  if (this->OrderedCompositingTree)
    {
    this->RemoveGeometryFromCompositingTree();
    }

  this->SetOrderedCompositingTreeInternal(tree);

  if (this->OrderedCompositingTree)
    {
    this->AddGeometryToCompositingTree();
    }

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
                                this->DistributorProxy->GetProperty("PKdTree"));
  pp->RemoveAllProxies();
  pp->AddProxy(this->OrderedCompositingTree);

  this->DistributorProxy->UpdateVTKObjects();
  this->LODDistributorProxy->UpdateVTKObjects();
  if (this->VolumeDistributorProxy)
    {
    this->VolumeDistributorProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::RemoveGeometryFromCompositingTree()
{
  unsigned int i;

  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
                                  this->DistributorProxy->GetProperty("Input"));
  if (ip->GetNumberOfProxies() < 1) return;

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
                         this->OrderedCompositingTree->GetProperty("DataSets"));

  vtkSMSourceProxy *input = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0));
  for (i = 0; i < input->GetNumberOfParts(); i++)
    {
    pp->RemoveProxy(input->GetPart(i));
    }

  if (this->VolumeDistributorProxy)
    {
    ip = vtkSMInputProperty::SafeDownCast(
                            this->VolumeDistributorProxy->GetProperty("Input"));
    input = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0));
    for (i = 0; i < input->GetNumberOfParts(); i++)
      {
      pp->RemoveProxy(input->GetPart(i));
      }
    }

  this->OrderedCompositingTree->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::AddGeometryToCompositingTree()
{
  this->RemoveGeometryFromCompositingTree();

  if (this->Visibility)
    {
    vtkSMInputProperty *ip;
    if (!this->VolumeRenderMode)
      {
      ip = vtkSMInputProperty::SafeDownCast(
                                  this->DistributorProxy->GetProperty("Input"));
      }
    else
      {
      ip = vtkSMInputProperty::SafeDownCast(
                            this->VolumeDistributorProxy->GetProperty("Input"));
      }
    if (ip->GetNumberOfProxies() < 1) return;
    vtkSMSourceProxy *input = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0));

    vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
                         this->OrderedCompositingTree->GetProperty("DataSets"));

    for (unsigned int i = 0; i < input->GetNumberOfParts(); i++)
      {
      pp->AddProxy(input->GetPart(i));
      }

    this->OrderedCompositingTree->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::SetVisibility(int visible)
{
  this->Superclass::SetVisibility(visible);

  if (this->OrderedCompositingTree)
    {
    this->AddGeometryToCompositingTree();
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::CacheUpdate(int idx, int total)
{
  this->Superclass::CacheUpdate(idx, total);

  if (this->VolumeRenderMode)
    {
    this->DistributedVolumeGeometryIsValid = 0;
    }
  else
    {
    this->DistributedGeometryIsValid = 0;
    }

  this->DistributedLODGeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::UpdateDistributedGeometry()
{
  this->Update();

  if (this->VolumeRenderMode)
    {
    if (!this->DistributedVolumeGeometryIsValid && this->VolumeGeometryIsValid)
      {
      vtkSMProperty *p
        = this->VolumeDistributorSuppressorProxy->GetProperty("ForceUpdate");
      p->Modified();
      this->DistributedVolumeGeometryIsValid = 1;
      // Make sure any ForceUpates are called in the correct order.  That is,
      // the superclasses' suppressors should be called before ours.
      this->VolumeUpdateSuppressorProxy->UpdateVTKObjects();
      this->VolumeDistributorSuppressorProxy->UpdateVTKObjects();
      }
    }
  else
    {
    if (!this->DistributedGeometryIsValid && this->GeometryIsValid)
      {
      vtkSMProperty *p
        = this->DistributorSuppressorProxy->GetProperty("ForceUpdate");
      p->Modified();
      this->DistributedGeometryIsValid = 1;
      // Make sure any ForceUpates are called in the correct order.  That is,
      // the superclasses' suppressors should be called before ours.
      this->UpdateSuppressorProxy->UpdateVTKObjects();
      this->DistributorSuppressorProxy->UpdateVTKObjects();
      }
    }

  if (!this->DistributedLODGeometryIsValid && this->LODGeometryIsValid)
    {
    vtkSMProperty *p
      = this->LODDistributorSuppressorProxy->GetProperty("ForceUpdate");
    p->Modified();
    this->DistributedLODGeometryIsValid = 1;
    // Make sure any ForceUpates are called in the correct order.  That is,
    // the superclasses' suppressors should be called before ours.
    this->LODUpdateSuppressorProxy->UpdateVTKObjects();
    this->LODDistributorSuppressorProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::Update()
{
  // If any geometry is going to be updated, make sure we invalidate the
  // distributed geometry.
  this->DistributedGeometryIsValid
    = this->DistributedGeometryIsValid && this->GeometryIsValid;
  this->DistributedLODGeometryIsValid =
    this->DistributedLODGeometryIsValid && this->LODGeometryIsValid;
  this->DistributedVolumeGeometryIsValid
    = this->DistributedVolumeGeometryIsValid && this->VolumeGeometryIsValid;

  this->Superclass::Update();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::MarkModified(vtkSMProxy *modifiedProxy)
{
  if (modifiedProxy == this->OrderedCompositingTree)
    {
    this->InvalidateDistributedGeometry();
    this->vtkSMConsumerDisplayProxy::MarkModified(modifiedProxy);
    return;
    }

  this->Superclass::MarkModified(modifiedProxy);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CollectionDecision: " << this->CollectionDecision << endl;
  os << indent << "LODCollectionDecision: " << this->LODCollectionDecision 
     << endl;
  os << indent << "OrderedCompositing: " << this->OrderedCompositing << endl;
  os << indent << "OrderedCompositingTree: "
     << this->OrderedCompositingTree << endl;
}
