/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTMultiDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMIceTMultiDisplayProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"

vtkCxxRevisionMacro(vtkSMIceTMultiDisplayProxy, "1.1");
vtkStandardNewMacro(vtkSMIceTMultiDisplayProxy);

//-----------------------------------------------------------------------------

vtkSMIceTMultiDisplayProxy::vtkSMIceTMultiDisplayProxy()
{
  this->OutlineFilterProxy = NULL;
  this->OutlineCollectProxy = NULL;
  this->OutlineUpdateSuppressorProxy = NULL;

  // Turn on suppression to start with to avoid unnecessary collections.
  // Turning it on later is no problem.
  this->SuppressGeometryCollection = 1;
}

//-----------------------------------------------------------------------------

vtkSMIceTMultiDisplayProxy::~vtkSMIceTMultiDisplayProxy()
{
  this->OutlineFilterProxy = NULL;
  this->OutlineCollectProxy = NULL;
  this->OutlineUpdateSuppressorProxy = NULL;
}

//-----------------------------------------------------------------------------

void vtkSMIceTMultiDisplayProxy::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutlineFilterProxy: " << this->OutlineFilterProxy << endl;
  os << indent << "OutlineCollectProxy: " << this->OutlineCollectProxy << endl;
  os << indent << "OutlineUpdateSuppressorProxy: "
     << this->OutlineUpdateSuppressorProxy << endl;
  os << indent << "SuppressGeometryCollection: "
     << this->SuppressGeometryCollection << endl;
}

//-----------------------------------------------------------------------------

void vtkSMIceTMultiDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated || !this->CanCreateProxy)
    {
    return;
    }

  this->OutlineFilterProxy = this->GetSubProxy("OutlineFilter");
  this->OutlineCollectProxy = this->GetSubProxy("OutlineCollect");
  this->OutlineUpdateSuppressorProxy
    = this->GetSubProxy("OutlineUpdateSuppressor");

  this->OutlineFilterProxy->SetServers(vtkProcessModule::DATA_SERVER);
  this->OutlineCollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->OutlineUpdateSuppressorProxy->SetServers(
                                          vtkProcessModule::CLIENT_AND_SERVERS);

  this->Superclass::CreateVTKObjects(numObjects);
}

//-----------------------------------------------------------------------------

void vtkSMIceTMultiDisplayProxy::SetupPipeline()
{
  this->Superclass::SetupPipeline();
  vtkSMProxyProperty *pp;

  pp = vtkSMProxyProperty::SafeDownCast(
                                this->OutlineFilterProxy->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(this->GeometryFilterProxy);
  this->OutlineFilterProxy->UpdateVTKObjects();

  pp = vtkSMProxyProperty::SafeDownCast(
                               this->OutlineCollectProxy->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(this->OutlineFilterProxy);
  this->OutlineCollectProxy->UpdateVTKObjects();

  for (unsigned int i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->OutlineCollectProxy->GetID(i) << "GetPolyDataOutput"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->OutlineUpdateSuppressorProxy->GetID(i) << "SetInput"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
                                           vtkProcessModule::CLIENT_AND_SERVERS,
                                           stream);
    }
  this->OutlineUpdateSuppressorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------

void vtkSMIceTMultiDisplayProxy::SetupDefaults()
{
  unsigned int i;

  this->Superclass::SetupDefaults();

  this->SetupCollectionFilter(this->OutlineCollectProxy);

  for (i = 0; i < this->OutlineCollectProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream cmd;
    vtkClientServerStream stream;
    vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
                                          vtkProcessModule::GetProcessModule());

    cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent" << "Execute OutlineCollect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->OutlineCollectProxy->GetID(i) << "AddObserver" << "StartEvent"
      << cmd << vtkClientServerStream::End;
    cmd.Reset();
    cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent" << "Execute OutlineCollect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->OutlineCollectProxy->GetID(i) << "AddObserver" << "EndEvent"
      << cmd << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    // Handle collection setup with client server.
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetSocketController"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->OutlineCollectProxy->GetID(i) << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);

    // Special condition to signal the client.
    // Because both processes of the Socket controller think they are 0!!!!
    if (pm->GetClientMode())
      {
      stream
        << vtkClientServerStream::Invoke
        << this->OutlineCollectProxy->GetID(i) << "SetController" << 0
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT, stream);
      }
    }

  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
                            this->OutlineCollectProxy->GetProperty("MoveMode"));
  ivp->SetElement(0, 2);
  this->OutlineCollectProxy->UpdateVTKObjects();

  // This is how the superclasses setup their update suppressors.  I'm just
  // following the herd.
  for (i = 0; i < this->OutlineUpdateSuppressorProxy->GetNumberOfIDs(); i++)
    {
    vtkClientServerStream stream;
    vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->OutlineUpdateSuppressorProxy->GetID(i)
      << "SetUpdateNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->OutlineUpdateSuppressorProxy->GetID(i) << "SetUpdatePiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
    }
}

//-----------------------------------------------------------------------------

void vtkSMIceTMultiDisplayProxy::SetCollectionDecision(int v)
{
  if (!this->OutlineUpdateSuppressorProxy)
    {
    vtkErrorMacro(<< "SetCollectionDecision called before "
                  "CreateVTKObjects called.");
    this->Superclass::SetCollectionDecision(v);
    return;
    }

  if (this->SuppressGeometryCollection)
    {
    // Set the mapper's input on the client to the outline.
    vtkClientServerStream stream;
    for (unsigned int i=0;
         i < this->OutlineUpdateSuppressorProxy->GetNumberOfIDs(); i++)
      {
      stream << vtkClientServerStream::Invoke
             << this->OutlineUpdateSuppressorProxy->GetID(i) << "GetOutput"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << this->MapperProxy->GetID(i) << "SetInput"
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      }
    vtkProcessModule::GetProcessModule()->SendStream(vtkProcessModule::CLIENT,
                                                     stream);

    // Turn off collection of real data.  Skip over vtkSMMultiDisplayProxy
    // since it has different logic for setting the collection decision.
    this->vtkSMCompositeDisplayProxy::SetCollectionDecision(0);
    }
  else
    {
    // Set the mapper's input on the client to the geometry.
    vtkClientServerStream stream;
    for (unsigned int i=0; i < this->OutlineUpdateSuppressorProxy->GetNumberOfIDs(); i++)
      {
      stream << vtkClientServerStream::Invoke
             << this->UpdateSuppressorProxy->GetID(i) << "GetOutput"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << this->MapperProxy->GetID(i) << "SetInput"
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      }
    vtkProcessModule::GetProcessModule()->SendStream(vtkProcessModule::CLIENT,
                                                     stream);

    this->Superclass::SetCollectionDecision(v);
    }
}

//-----------------------------------------------------------------------------

void vtkSMIceTMultiDisplayProxy::SetLODCollectionDecision(int v)
{
  if (!this->OutlineUpdateSuppressorProxy)
    {
    vtkErrorMacro(<< "SetLODCollectionDecision called before "
                  "CreateVTKObjects called.");
    this->Superclass::SetCollectionDecision(v);
    return;
    }

  if (this->SuppressGeometryCollection)
    {
    // Set the mapper's input on the client to the outline.
    vtkClientServerStream stream;
    for (unsigned int i=0;
         i < this->OutlineUpdateSuppressorProxy->GetNumberOfIDs(); i++)
      {
      stream << vtkClientServerStream::Invoke
             << this->OutlineUpdateSuppressorProxy->GetID(i) << "GetOutput"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << this->LODMapperProxy->GetID(i) << "SetInput"
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      }
    vtkProcessModule::GetProcessModule()->SendStream(vtkProcessModule::CLIENT,
                                                     stream);

    // Turn off collection of real data.  Skip over vtkSMMultiDisplayProxy
    // since it has different logic for setting the collection decision.
    this->vtkSMCompositeDisplayProxy::SetLODCollectionDecision(0);
    }
  else
    {
    // Set the mapper's input on the client to the geometry.
    vtkClientServerStream stream;
    for (unsigned int i=0;
         i < this->OutlineUpdateSuppressorProxy->GetNumberOfIDs(); i++)
      {
      stream << vtkClientServerStream::Invoke
             << this->LODUpdateSuppressorProxy->GetID(i) << "GetOutput"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << this->LODMapperProxy->GetID(i) << "SetInput"
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      }
    vtkProcessModule::GetProcessModule()->SendStream(vtkProcessModule::CLIENT,
                                                     stream);

    this->Superclass::SetLODCollectionDecision(v);
    }
}
