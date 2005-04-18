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
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkPVLODPartDisplayInformation.h"

vtkStandardNewMacro(vtkSMLODDisplayProxy);
vtkCxxRevisionMacro(vtkSMLODDisplayProxy, "1.3");
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
  
  this->LODInformation->CopyFromObject(0); // Clear information.
  if (this->LODDecimatorProxy->GetNumberOfIDs() > 0)
    {
    vtkProcessModule::GetProcessModule()->GatherInformation(
      this->LODInformation, this->LODDecimatorProxy->GetID(0));
    }
  this->LODInformationIsValid = 1;

  return this->LODInformation;  
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::SetupVolumePipeline()
{
  if (!this->HasVolumePipeline)
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
  // The drawback to this is that now, when the non-LOD Volume properties
  // change, care must be taken to appropriately update LODMapperProxy.
  // The only property we need to be explicitly worried about is 
  // "SelectScalarArray" ("SelectColorArray");

}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::SetupPipeline()
{
  this->Superclass::SetupPipeline();
  vtkSMInputProperty* ip;
  vtkSMProxyProperty* pp;
  
  ip = vtkSMInputProperty::SafeDownCast(
    this->LODDecimatorProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on LODDecimatorProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->GeometryFilterProxy);
  this->LODDecimatorProxy->UpdateVTKObjects();

  ip = vtkSMInputProperty::SafeDownCast(
    this->LODUpdateSuppressorProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on LODUpdateSuppressorProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->LODDecimatorProxy);
  this->LODUpdateSuppressorProxy->UpdateVTKObjects();

  // LODUpdateSuppressorProxy shares OutputType property with UpdateSuppressorProxy,
  // so it will get set accordingly.
  ip = vtkSMInputProperty::SafeDownCast(
    this->LODMapperProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on LODMapperProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->LODUpdateSuppressorProxy);
  this->LODMapperProxy->UpdateVTKObjects();
  
  pp = vtkSMProxyProperty::SafeDownCast(
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
  // Will get intialzied with this->MapperProxy  in Superclass::SetupDefaults();

  // I have no clue how to convert this stuff to properties.
  // Broadcast for subclasses.
  vtkClientServerStream stream;
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  unsigned int i;
  for (i = 0; i < this->LODUpdateSuppressorProxy->GetNumberOfIDs(); i++)
    {
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID(i) << "SetUpdateNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID(i) << "SetUpdatePiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS, stream);
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::CreateVTKObjects(int numObjects)
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

  this->Superclass::CreateVTKObjects(numObjects);
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::CacheUpdate(int idx, int total)
{
  this->Superclass::CacheUpdate(idx, total);
  vtkClientServerStream stream;
  stream
    << vtkClientServerStream::Invoke
    << this->LODMapperProxy->GetID(0) << "Modified"
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream); 
}

//-----------------------------------------------------------------------------
void vtkSMLODDisplayProxy::Update()
{
  this->Superclass::Update();

  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  if (!this->LODGeometryIsValid && pm->GetGlobalLODFlag() && 
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
void vtkSMLODDisplayProxy::InvalidateLODGeometry()
{
  this->LODGeometryIsValid = 0;
  this->LODInformationIsValid = 0;
  this->InvokeEvent(vtkSMLODDisplayProxy::InformationInvalidatedEvent);
  if (this->LODUpdateSuppressorProxy)
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
  this->InvalidateLODGeometry();
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
