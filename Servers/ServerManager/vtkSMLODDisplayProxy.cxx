/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLODDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMLODDisplayProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMLODDisplayProxy);
vtkCxxRevisionMacro(vtkSMLODDisplayProxy, "1.20");
//-----------------------------------------------------------------------------
vtkSMLODDisplayProxy::vtkSMLODDisplayProxy()
{
  this->LODDecimatorProxy = 0;
  this->LODUpdateSuppressorProxy = 0;
  this->LODMapperProxy = 0;
  this->LODResolution = 50;
  this->LODGeometryIsValid = 0;
  this->LODInformation = vtkPVLODPartDisplayInformation::New();
  this->LODInformationIsValid = 0;
}

//-----------------------------------------------------------------------------
vtkSMLODDisplayProxy::~vtkSMLODDisplayProxy()
{
  this->LODDecimatorProxy = 0;
  this->LODUpdateSuppressorProxy = 0;
  this->LODMapperProxy = 0;
  this->LODInformation->Delete();
}
//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::SetLODResolution(int res)
{
  if (res == this->LODResolution)
    {
    return;
    }
  this->LODResolution = res;
  if (!this->LODDecimatorProxy)
    {
    return;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LODDecimatorProxy->GetProperty("NumberOfDivisions"));
  ivp->SetElement(0, this->LODResolution);
  ivp->SetElement(1, this->LODResolution);
  ivp->SetElement(2, this->LODResolution);
  this->UpdateVTKObjects();
  this->InvalidateGeometry();
}

//-----------------------------------------------------------------------------
vtkPVLODPartDisplayInformation* vtkSMLODDisplayProxy::GetLODInformation()
{
  if (this->LODInformationIsValid)
    {
    return this->LODInformation;
    }
  if ( ! vtkProcessModule::GetProcessModule() || !this->ObjectsCreated)
    {
    return 0;
    }

  // Update the GeometryFilter so that we don't get obsolete
  // data information.
  vtkSMSourceProxy::SafeDownCast(this->GeometryFilterProxy)->UpdatePipeline(
    this->UpdateTime);
  
  this->LODInformation->CopyFromObject(0); // Clear information.
  if (!this->LODDecimatorProxy->GetID().IsNull())
    {
    vtkProcessModule::GetProcessModule()->GatherInformation(
      this->ConnectionID, this->LODDecimatorProxy->GetServers(),
      this->LODInformation, this->LODDecimatorProxy->GetID());
    }
  this->LODInformationIsValid = 1;

  return this->LODInformation;  
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::SetupVolumePipeline()
{
  if (this->VolumePipelineType == NONE)
    {
    return;
    }
  this->Superclass::SetupVolumePipeline();
  vtkSMProxyProperty* pp;
  
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("LODMapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->LODMapperProxy);
  // I am reusing the regular LOD mapper.
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::SetupPipeline()
{
  this->Superclass::SetupPipeline();
  
  this->Connect(this->LODDecimatorProxy, this->GeometryFilterProxy, 0);
  this->Connect(this->LODUpdateSuppressorProxy, this->LODDecimatorProxy, 0);
  this->Connect(this->LODMapperProxy, this->LODUpdateSuppressorProxy, 0);
  
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->ActorProxy->GetProperty("LODMapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property LODMapper on ActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->LODMapperProxy);
  this->ActorProxy->UpdateVTKObjects();
  
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::SetupDefaults()
{
  this->Superclass::SetupDefaults();
  vtkSMIntVectorProperty *ip;

  // Initialize LODDecimatorProxy.
  ip = vtkSMIntVectorProperty::SafeDownCast(
    this->LODDecimatorProxy->GetProperty("CopyCellData"));
  ip->SetElement(0, 1);

  ip = vtkSMIntVectorProperty::SafeDownCast(
    this->LODDecimatorProxy->GetProperty("UseInputPoints"));
  ip->SetElement(0, 1);

  ip = vtkSMIntVectorProperty::SafeDownCast(
    this->LODDecimatorProxy->GetProperty("UseInternalTriangles"));
  ip->SetElement(0, 0);
  this->LODDecimatorProxy->UpdateVTKObjects();

  // Initialize LODMapperProxy
  // Will get intialzied with this->MapperProxy in Superclass::SetupDefaults();

  // Broadcast for subclasses.
  vtkClientServerStream stream;
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorProxy->GetID() << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODUpdateSuppressorProxy->GetID() << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::CLIENT_AND_SERVERS, stream);

  // This is here just for streaming (can be removed if streaming is removed).
  vtkClientServerStream stream2;
  stream2
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODMapperProxy->GetID() << "SetNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream2
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODMapperProxy->GetID() << "SetPiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  // Do we need to client too?
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::RENDER_SERVER, stream2);
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated || !this->CanCreateProxy)
    {
    return;
    }
  this->LODDecimatorProxy = this->GetSubProxy("LODDecimator");
  this->LODUpdateSuppressorProxy = this->GetSubProxy("LODUpdateSuppressor");
  this->LODMapperProxy = this->GetSubProxy("LODMapper");

  this->LODDecimatorProxy->SetServers(vtkProcessModule::DATA_SERVER);
  this->LODUpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->LODMapperProxy->SetServers(vtkProcessModule::CLIENT | 
    vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::CacheUpdate(int idx, int total)
{
  if (!this->LODMapperProxy)
    {
    vtkErrorMacro("Objects not created yet!");
    return;
    }
  this->Superclass::CacheUpdate(idx, total);

  // Is this even necessary? LOD mode will not be active when
  // using Cache.
  vtkClientServerStream stream;
  stream
    << vtkClientServerStream::Invoke
    << this->LODMapperProxy->GetID() << "Modified"
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream); 

  // Since displayed geometry changed, the LOD geometry is no longer valid.
  this->LODGeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
int vtkSMLODDisplayProxy::GetLODFlag()
{
  if (!this->ActorProxy)
    {
    vtkErrorMacro("ActorProxy not created.");
    return 1;
    }
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->ActorProxy->GetProperty("RenderModuleHelper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property RenderModuleHelper.");
    return 1;
    }
  if (pp->GetNumberOfProxies() < 1)
    {
    vtkErrorMacro("RenderModuleHelper not set.");
    return 1;
    }
  vtkPVRenderModuleHelper* helper = vtkPVRenderModuleHelper::SafeDownCast(
    vtkProcessModule::GetProcessModule()->
    GetObjectFromID(pp->GetProxy(0)->GetID()));
  if (!helper)
    {
    vtkErrorMacro("RenderModuleHelper object not found.");
    return 1;
    }
  return helper->GetLODFlag();
}

//-----------------------------------------------------------------------------
int vtkSMLODDisplayProxy::UpdateRequired()
{
  if (!this->LODGeometryIsValid && this->GetLODFlag() && 
    this->LODUpdateSuppressorProxy)
    {
    return 1;
    } 
  return this->Superclass::UpdateRequired();
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::Update(vtkSMAbstractViewModuleProxy* view)
{
  this->Superclass::Update(view);

  if (!this->LODGeometryIsValid && this->GetLODFlag() && 
    this->LODUpdateSuppressorProxy)
    {
    this->UpdateLODPipeline();
    }
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::UpdateLODPipeline()
{
  if (!this->LODGeometryIsValid)
    {
    this->LODInformationIsValid = 0;
    vtkSMProperty* p = this->LODUpdateSuppressorProxy->GetProperty(
      "ForceUpdate");
    if (!p)
      {
      vtkErrorMacro("Failed to find property ForceUpdate on "
        "LODUpdateSuppressorProxy.");
      return;
      }
    p->Modified();
    this->LODUpdateSuppressorProxy->UpdateVTKObjects();
    this->LODGeometryIsValid = 1;
    this->InvokeEvent(vtkSMLODDisplayProxy::InformationInvalidatedEvent);
    }
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  this->Superclass::MarkModified(modifiedProxy);

  // Do not invalidate geometry if MarkModified() was called by self.
  // A lot of the changes to the display proxy do not require
  // invalidating geometry. Those that do should call InvalidateGeometry()
  // explicitly.
  if (modifiedProxy != this)
    {
    this->InvalidateLODGeometry(this->UseCache);
    }
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::InvalidateLODGeometry(int useCache)
{
  this->LODGeometryIsValid = 0;
  this->LODInformationIsValid = 0;
  this->InvokeEvent(vtkSMLODDisplayProxy::InformationInvalidatedEvent);
  
  if (!useCache && this->LODUpdateSuppressorProxy)
    {
    vtkSMProperty* p = this->LODUpdateSuppressorProxy->GetProperty("RemoveAllCaches");
    if (!p)
      {
      vtkErrorMacro("Failed to find property RemoveAllCaches on LODUpdateSuppressorProxy.");
      return;
      }
    p->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::InvalidateGeometry()
{
  this->Superclass::InvalidateGeometry();
  this->InvalidateLODGeometry(0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LODDecimatorProxy: " << this->LODDecimatorProxy << endl;
  os << indent << "LODUpdateSuppressorProxy: " <<
    this->LODUpdateSuppressorProxy << endl;
  os << indent << "LODMapperProxy: " << this->LODMapperProxy << endl;
  os << indent << "LODInformation: " << this->LODInformation << endl;
  os << indent << "LODResolution: " << this->LODResolution << endl;
  os << indent << "LODGeometryIsValid: " << this->LODGeometryIsValid << endl;
  os << indent << "LODInformationIsValid: " << this->LODInformationIsValid 
    << endl;
}
