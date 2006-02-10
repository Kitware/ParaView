/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataObjectDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataObjectDisplayProxy.h"

#include "vtkClientServerStream.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkProcessModule.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMMaterialLoaderProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMDataObjectDisplayProxy);
vtkCxxRevisionMacro(vtkSMDataObjectDisplayProxy, "1.11");


//-----------------------------------------------------------------------------
vtkSMDataObjectDisplayProxy::vtkSMDataObjectDisplayProxy()
{
  this->GeometryFilterProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0; 
  this->PropertyProxy = 0;
  this->ActorProxy = 0;
  this->GeometryIsValid = 0;
  this->VolumeGeometryIsValid = 0;
  this->CanCreateProxy = 0;

  this->VolumeFilterProxy = 0;
  this->VolumeUpdateSuppressorProxy = 0;
  this->VolumePTMapperProxy = 0;
  this->VolumeBunykMapperProxy = 0;
  this->VolumeZSweepMapperProxy = 0;
  this->VolumeActorProxy = 0;
  this->VolumePropertyProxy = 0;
  this->OpacityFunctionProxy = 0;
  this->ColorTransferFunctionProxy = 0;

  this->HasVolumePipeline    = 0; // By Default, don't bother about the Volume Pipeline.
  this->SupportsBunykMapper  = 0;
  this->SupportsZSweepMapper = 0;
  this->VolumeRenderMode     = 0;

  this->Visibility = 1;
  this->Representation = -1;

  this->GeometryInformationIsValid = 0;
  this->GeometryInformation = vtkPVGeometryInformation::New();

}

//-----------------------------------------------------------------------------
vtkSMDataObjectDisplayProxy::~vtkSMDataObjectDisplayProxy()
{
  this->GeometryFilterProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0; 
  this->PropertyProxy = 0;
  this->ActorProxy = 0;

  this->VolumeFilterProxy = 0;
  this->VolumeUpdateSuppressorProxy = 0;
  this->VolumePTMapperProxy = 0;
  this->VolumeBunykMapperProxy = 0;
  this->VolumeZSweepMapperProxy = 0;
  this->VolumeActorProxy = 0;
  this->VolumePropertyProxy = 0;
  this->OpacityFunctionProxy = 0;
  this->ColorTransferFunctionProxy = 0;
  this->GeometryInformation->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated || !this->CanCreateProxy)
    {
    return;
    }
  this->GeometryFilterProxy = this->GetSubProxy("GeometryFilter");
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->MapperProxy = this->GetSubProxy("Mapper");
  this->PropertyProxy = this->GetSubProxy("Property");
  this->ActorProxy = this->GetSubProxy("Prop");

  this->GeometryFilterProxy->SetServers(vtkProcessModule::DATA_SERVER);
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->MapperProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->ActorProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->PropertyProxy->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  // Volume Stuff.
 
  if (this->HasVolumePipeline)
    {
    this->VolumeFilterProxy = this->GetSubProxy("VolumeFilter");
    this->VolumeUpdateSuppressorProxy
      = this->GetSubProxy("VolumeUpdateSuppressor");
    this->VolumePTMapperProxy = this->GetSubProxy("VolumePTMapper");
    this->VolumeBunykMapperProxy = this->GetSubProxy("VolumeBunykMapper");
    this->VolumeZSweepMapperProxy = this->GetSubProxy("VolumeZSweepMapper");
    this->VolumeActorProxy = this->GetSubProxy("VolumeActor");
    this->VolumePropertyProxy = this->GetSubProxy("VolumeProperty");
    this->OpacityFunctionProxy = this->GetSubProxy("OpacityFunction");
    this->ColorTransferFunctionProxy
      = this->GetSubProxy("ColorTransferFunction");

    this->VolumeFilterProxy->SetServers(vtkProcessModule::DATA_SERVER);
    this->VolumeUpdateSuppressorProxy->SetServers(
                                          vtkProcessModule::CLIENT_AND_SERVERS);
    this->VolumePTMapperProxy->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    this->VolumeBunykMapperProxy->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    this->VolumeZSweepMapperProxy->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    this->VolumeActorProxy->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    this->VolumePropertyProxy->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    this->OpacityFunctionProxy->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    this->ColorTransferFunctionProxy->SetServers(
      vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    }
  else
    {
    // Remove all volume related subproxies (so that they are not created).
    this->RemoveSubProxy("VolumeFilter");
    this->RemoveSubProxy("VolumeUpdateSuppressor");
    this->RemoveSubProxy("VolumePTMapper");
    this->RemoveSubProxy("VolumeBunykMapper");
    this->RemoveSubProxy("VolumeZSweepMapper");
    this->RemoveSubProxy("VolumeActor");
    this->RemoveSubProxy("VolumeProperty");
    this->RemoveSubProxy("OpacityFunction");
    this->RemoveSubProxy("ColorTransferFunction");
    }

  this->Superclass::CreateVTKObjects(numObjects);

  vtkSMMaterialLoaderProxy* mlp = vtkSMMaterialLoaderProxy::SafeDownCast(
    this->GetSubProxy("MaterialLoader"));
  if (mlp)
    {
    mlp->SetPropertyProxy(this->PropertyProxy);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::AddInput(vtkSMSourceProxy* input, const char*, 
                                       int)
{
  this->SetInput(input);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetInput(vtkSMProxy* input)
{
  if (input == NULL)
    {
    vtkWarningMacro("Trying to set a NULL input.");
    return;
    }
  //This is where the pipeline is setup.
  this->SetInputInternal(vtkSMSourceProxy::SafeDownCast(input));
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetInputInternal(vtkSMSourceProxy* input)
{
  int num = 0;
  if (input)
    {
    num = input->GetNumberOfParts();
    if (!num)
      {
      input->CreateParts();
      num = input->GetNumberOfParts();
      }
    }
  if (num == 0)
    {
    vtkErrorMacro("Input proxy has no output! Cannot create the display");
    return;
    }
  
  // This will create all the subproxies with correct number of parts.
  if (input)
    {
    this->CanCreateProxy = 1;
    }

  // Determine if VolumePipeline should be enabled.
  vtkSMProxy* p = this->GetSubProxy("VolumeFilter");
  vtkSMDataTypeDomain* domain = vtkSMDataTypeDomain::SafeDownCast(
    p->GetProperty("Input")->GetDomain("input_type"));
  this->HasVolumePipeline =  (domain->IsInDomain(input))? 1 : 0;
  this->SupportsBunykMapper = 0;
  this->SupportsZSweepMapper = 0;
  
  if ( this->HasVolumePipeline )
    {
    if (input->GetDataInformation()->GetNumberOfCells() < 1000000)
      {
      this->SupportsZSweepMapper = 1;
      }
    if (input->GetDataInformation()->GetNumberOfCells() < 500000)
      {
      this->SupportsBunykMapper = 1;
      }
    }
    
  this->CreateVTKObjects(num);

  vtkSMInputProperty* ip;
 
  input->UpdateVTKObjects();
  ip = vtkSMInputProperty::SafeDownCast(
    this->GeometryFilterProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(input);
  if (!ip->GetImmediateUpdate())
    {
    this->GeometryFilterProxy->UpdateVTKObjects();
    }

  if (this->HasVolumePipeline)
    {
    ip = vtkSMInputProperty::SafeDownCast(
      this->VolumeFilterProxy->GetProperty("Input"));
    ip->RemoveAllProxies();
    ip->AddProxy(input);
    if (!ip->GetImmediateUpdate())
      {
      this->VolumeFilterProxy->UpdateVTKObjects();
      }
    }
 
  if (input)
    {
    // First, setup the pipeline.
    this->SetupPipeline();
    // Second, set default property values.
    this->SetupDefaults();
    }

  if (this->HasVolumePipeline)
    {
    // Set up the Volume Pipeline if needed.
    this->SetupVolumePipeline();
    this->SetupVolumeDefaults();
    }
}
//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetupPipeline()
{
  vtkSMInputProperty* ipp = 0;
  vtkSMProxyProperty* pp = 0;
  vtkSMStringVectorProperty* svp = 0;
  
  ipp = vtkSMInputProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Input"));
  if (!ipp)
    {
    vtkErrorMacro("Failed to find property Input on UpdateSuppressor.");
    return;
    }
  ipp->RemoveAllProxies();
  ipp->AddProxy(this->GeometryFilterProxy);

  svp  = vtkSMStringVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("OutputType"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property OutputType on UpdateSuppressorProxy.");
    return;
    }
  svp->SetElement(0,"vtkPolyData");
  this->UpdateSuppressorProxy->UpdateVTKObjects();

  ipp = vtkSMInputProperty::SafeDownCast(
    this->MapperProxy->GetProperty("Input"));
  if (!ipp)
    {
    vtkErrorMacro("Failed to find property Input on MapperProxy.");
    return;
    }
  ipp->RemoveAllProxies();
  ipp->AddProxy(this->UpdateSuppressorProxy);
  this->MapperProxy->UpdateVTKObjects();

  pp = vtkSMProxyProperty::SafeDownCast(
    this->ActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on ActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->MapperProxy);
  

  pp = vtkSMProxyProperty::SafeDownCast(
    this->ActorProxy->GetProperty("Property"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Property on ActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->PropertyProxy);

  this->ActorProxy->UpdateVTKObjects();

}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetupDefaults()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("ProcessModule should be set before setting up the display "
      "pipeline.");
    return;
    }
  vtkSMIntVectorProperty* ivp = 0;
  vtkSMDoubleVectorProperty* dvp = 0;
  unsigned int i;

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GeometryFilterProxy->GetProperty("UseStrips"));
  ivp->SetElement(0, 0);

  //TODO: stuff for logging geometry filter times.
  vtkClientServerStream stream;
  for (i = 0; i < this->GeometryFilterProxy->GetNumberOfIDs(); i++)
    {  
    // Keep track of how long each geometry filter takes to execute.
    vtkClientServerStream start;
    start << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
      << "LogStartEvent" << "Execute Geometry" 
      << vtkClientServerStream::End;
    vtkClientServerStream end;
    end << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
      << "LogEndEvent" << "Execute Geometry" 
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
      << this->GeometryFilterProxy->GetID(i) 
      << "AddObserver"
      << "StartEvent"
      << start
      << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
      << this->GeometryFilterProxy->GetID(i) 
      << "AddObserver"
      << "EndEvent"
      << end
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::DATA_SERVER, stream);
  // Init Mapper properties.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->MapperProxy->GetProperty("UseLookupTableScalarRange"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property UseLookupTableScalarRange "
      "on MapperProxy.");
    return;
    }
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->MapperProxy->GetProperty("InterpolateScalarsBeforeMapping"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property InterpolateScalarsBeforeMapping "
      "on MapperProxy.");
    return;
    }
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->MapperProxy->GetProperty("ImmediateModeRendering"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ImmediateModeRendering on MapperProxy.");
    return;
    }
  ivp->SetElement(0, 0);

  // Init Property properties.
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("Ambient"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Ambient on PropertyProxy.");
    return;
    }
  dvp->SetElement(0, 0.0);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("Diffuse"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Diffuse on PropertyProxy.");
    return;
    }
  dvp->SetElement(0, 1.0);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("Specular"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Specular on PropertyProxy.");
    return;
    }
  dvp->SetElement(0, 0.1);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("SpecularPower"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property SpecularPower on PropertyProxy.");
    return;
    }
  dvp->SetElement(0, 100);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("SpecularColor"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property SpecularColor on PropertyProxy.");
    return;
    }
  dvp->SetElement(0, 1.0);
  dvp->SetElement(1, 1.0);
  dvp->SetElement(2, 1.0);

  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  for (i=0; i < this->UpdateSuppressorProxy->GetNumberOfIDs(); i++)
    {
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(i) << "SetUpdateNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID(i) << "SetUpdatePiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID,
    this->UpdateSuppressorProxy->GetServers(), stream);

  // This is here just for streaming (can be removed if streaming is removed).
  vtkClientServerStream stream2;
  for (i=0; i < this->UpdateSuppressorProxy->GetNumberOfIDs(); i++)
    {
    stream2
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->MapperProxy->GetID(i) << "SetNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream2
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->MapperProxy->GetID(i) << "SetPiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  // Do we need to client too?
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, stream2);  
  
  //  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetupVolumePipeline()
{
  if (!this->HasVolumePipeline)
    {
    return;
    }

  vtkSMInputProperty* ip;
  vtkSMProxyProperty* pp;
  vtkSMStringVectorProperty* svp;
  
  ip = vtkSMInputProperty::SafeDownCast(
                       this->VolumeUpdateSuppressorProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on VolumeUpdateSuppressor.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeFilterProxy);

  svp  = vtkSMStringVectorProperty::SafeDownCast(
                  this->VolumeUpdateSuppressorProxy->GetProperty("OutputType"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property OutputType on "
                  "VolumeUpdateSuppressorProxy.");
    return;
    }
  svp->SetElement(0,"vtkUnstructuredGrid");
  this->VolumeUpdateSuppressorProxy->UpdateVTKObjects();

  ip = vtkSMInputProperty::SafeDownCast(
    this->VolumePTMapperProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on VolumePTMapperProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeUpdateSuppressorProxy);


  ip = vtkSMInputProperty::SafeDownCast(
    this->VolumeBunykMapperProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on VolumeBunykMapperProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeUpdateSuppressorProxy);


  ip = vtkSMInputProperty::SafeDownCast(
    this->VolumeZSweepMapperProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on VolumeZSweepMapperProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->VolumeUpdateSuppressorProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->VolumePTMapperProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("Property"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Property on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->VolumePropertyProxy);
 
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumePropertyProxy->GetProperty("ColorTransferFunction"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property ColorTransferFunction on VolumePropertyProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->ColorTransferFunctionProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumePropertyProxy->GetProperty("ScalarOpacityFunction"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property ScalarOpacityFunction on VolumePropertyProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->OpacityFunctionProxy);

}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetupVolumeDefaults()
{
  if (!this->HasVolumePipeline)
    {
    return;
    }
  // VolumeFilterProxy  defaults.
  // No defaults to set.

  // VolumePTMapperProxy defaults.
  // No defaults to set.

  // VolumeActorProxy defaults.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on VolumeActorProxy.");
    return;
    }
  ivp->SetElement(0, 0);

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("ProcessModule should be set before setting up the display "
                  "pipeline.");
    return;
    }
  vtkClientServerStream stream;
  unsigned int i;
  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  for (i = 0; i < this->VolumeUpdateSuppressorProxy->GetNumberOfIDs(); i++)
    {
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->VolumeUpdateSuppressorProxy->GetID(i)
      << "SetUpdateNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->VolumeUpdateSuppressorProxy->GetID(i) << "SetUpdatePiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID,
    this->VolumeUpdateSuppressorProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
vtkPVGeometryInformation* vtkSMDataObjectDisplayProxy::GetGeometryInformation()
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return 0;
    }
  if (!this->GeometryInformationIsValid)
    {
    this->GatherGeometryInformation();
    }
  return this->GeometryInformation;
}
//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetRepresentation(int representation)
{
  if (!this->ObjectsCreated)
    {
    return;
    }
    
  if (this->Representation == representation)
    {
    return;
    }
  
  vtkSMIntVectorProperty* ivp;
  if (representation == vtkSMDataObjectDisplayProxy::VOLUME)
    {
    if (!this->HasVolumePipeline)
      {
      vtkErrorMacro("Display does not have Volume Rendering support.");
      return;
      }
    this->VolumeRenderModeOn();
    }
  else
    {
    this->VolumeRenderModeOff();
    }
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GeometryFilterProxy->GetProperty("UseOutline"));
  int outline = (representation == vtkSMDataObjectDisplayProxy::OUTLINE)? 1 : 0;
  ivp->SetElement(0, outline);
  this->GeometryFilterProxy->UpdateVTKObjects();

  if (representation == vtkSMDataObjectDisplayProxy::POINTS ||
    representation == vtkSMDataObjectDisplayProxy::WIREFRAME || 
    representation == vtkSMDataObjectDisplayProxy::SURFACE)
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->PropertyProxy->GetProperty("Representation"));
    ivp->SetElement(0, representation);
    this->PropertyProxy->UpdateVTKObjects();
    }
  // Handle specularity and lighting. All but surface turns shading off.
  double diffuse = 0.0;
  double ambient = 1.0;
  double specularity = 0.0;

  if (representation == vtkSMDataObjectDisplayProxy::SURFACE)
    {
    diffuse = 1.0;
    ambient = 0.0;
    // Turn on specularity when coloring by property.
    if ( !this->GetScalarVisibilityCM())
      {
      specularity = 0.1;
      }
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("Ambient"));
  dvp->SetElement(0, ambient);
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("Diffuse"));
  dvp->SetElement(0, diffuse);
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PropertyProxy->GetProperty("Specular"));
  dvp->SetElement(0, specularity);
  
  // We need to invalidate geometry so the representation changes are passed thru 
  // the update suppressor.
  this->InvalidateGeometry();
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::VolumeRenderModeOn()
{
  if (this->VolumeRenderMode)
    {
    return;
    }
  this->VolumeRenderMode = 1;
  if (this->Visibility)
    {
    this->SetVisibility(1);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::VolumeRenderModeOff()
{
  if (!this->VolumeRenderMode)
    {
    return;
    }
  this->VolumeRenderMode = 0;
  if (this->Visibility)
    {
    this->SetVisibility(1);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::ResetTransferFunctions()
{
  if (!this->HasVolumePipeline)
    {
    vtkErrorMacro("This display does not support Volume Rendering.");
    return;
    }

  vtkSMIntVectorProperty* ivp;
  vtkSMStringVectorProperty* svp;
  vtkSMInputProperty* ip;
  
  int mode;
  const char* arrayname;
  
  // 1) Determine the scalar mode. (Point data or cell data?).
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->VolumePTMapperProxy->GetProperty("ScalarMode"));
  mode = ivp->GetElement(0);
  if (mode != vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA && 
    mode != vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
    {
    vtkErrorMacro("Only Point Field Data and Cell Field Data can be used for "
      "volume rendering.");
    return;
    }

  // 2) Determine the array used for volume rendering.
  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->VolumePTMapperProxy->GetProperty("SelectScalarArray"));
  arrayname = svp->GetElement(0);
 
  // 3) Get the Input Proxy.
  ip = vtkSMInputProperty::SafeDownCast(
    this->VolumeFilterProxy->GetProperty("Input"));
  if (ip->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Either no input set or too many inputs set for the DisplayProxy.");
    return;
    }
  vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(
    ip->GetProxy(0));
  if (!sp)
    {
    vtkErrorMacro("Input to a DisplayProxy must be a source proxy.");
    return;
    }
  
  vtkPVDataInformation* dataInfo = sp->GetDataInformation();
  vtkPVDataSetAttributesInformation* attrInfo =
    (mode == vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA) ?
    dataInfo->GetPointDataInformation() :  dataInfo->GetCellDataInformation();
  vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(arrayname);

  this->ResetTransferFunctions(dataInfo, arrayInfo);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::ResetTransferFunctions(
  vtkPVDataInformation* dataInfo, vtkPVArrayInformation* arrayInfo)
{
  if (!dataInfo || !arrayInfo)
    {
    return;
    }
  double range[2];
  arrayInfo->GetComponentRange(0, range);
  
  double bounds[6];
  dataInfo->GetBounds(bounds);
  double diameter = 
    sqrt( (bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
          (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
          (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]) );
  
  int numCells = dataInfo->GetNumberOfCells();
  double linearNumCells = pow( (double) numCells, (1.0/3.0) );
  double unitDistance = diameter;
  if (linearNumCells != 0.0)
    {
    unitDistance = diameter / linearNumCells;
    }

  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
 
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->OpacityFunctionProxy->GetProperty("Points"));
  dvp->SetNumberOfElements(4);
  dvp->SetElement(0, range[0]);
  dvp->SetElement(1, 0.0);
  dvp->SetElement(2, range[1]);
  dvp->SetElement(3, 1.0);
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ColorTransferFunctionProxy->GetProperty("RGBPoints"));
  dvp->SetNumberOfElements(8);
  double r, g, b;
  vtkMath::HSVToRGB(.667, 1, 1, &r, &g , &b);
  dvp->SetElement(0, range[0]);
  dvp->SetElement(1, r);
  dvp->SetElement(2, g);
  dvp->SetElement(3, b);
 
  vtkMath::HSVToRGB(0, 1, 1, &r, &g , &b);
  dvp->SetElement(4, range[1]);
  dvp->SetElement(5, r);
  dvp->SetElement(6, g);
  dvp->SetElement(7, b);
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ColorTransferFunctionProxy->GetProperty("ColorSpace"));
  ivp->SetElement(0, 1);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ColorTransferFunctionProxy->GetProperty("HSVWrap"));
  ivp->SetElement(0, 0);

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->VolumePropertyProxy->GetProperty(
      "ScalarOpacityUnitDistance"));
  dvp->SetElement(0, unitDistance);

  this->OpacityFunctionProxy->UpdateVTKObjects();
  this->ColorTransferFunctionProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetVolumeMapperToBunykCM()
{
  if ( !this->HasVolumePipeline )
    {
    return;
    }

  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->VolumeBunykMapperProxy);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetVolumeMapperToPTCM()
{
  if ( !this->HasVolumePipeline )
    {
    return;
    }

  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->VolumePTMapperProxy);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetVolumeMapperToZSweepCM()
{
  if ( !this->HasVolumePipeline )
    {
    return;
    }
  
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->VolumeZSweepMapperProxy);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetVolumeMapperTypeCM()
{
  if ( !this->HasVolumePipeline )
    {
    return vtkSMDataObjectDisplayProxy::UNKNOWN_VOLUME_MAPPER;
    }
  
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
    return vtkSMDataObjectDisplayProxy::UNKNOWN_VOLUME_MAPPER;
    }
  
  vtkSMProxy *p = pp->GetProxy(0);
  
  if ( !p )
    {
    vtkErrorMacro("Failed to find proxy in Mapper proxy property!");
    return vtkSMDataObjectDisplayProxy::UNKNOWN_VOLUME_MAPPER;
    }
  
  if ( !strcmp(p->GetVTKClassName(), "vtkProjectedTetrahedraMapper" ) )
    {
    return vtkSMDataObjectDisplayProxy::PROJECTED_TETRA_VOLUME_MAPPER;
    }

  if ( !strcmp(p->GetVTKClassName(), "vtkUnstructuredGridVolumeZSweepMapper" ) )
    {
    return vtkSMDataObjectDisplayProxy::ZSWEEP_VOLUME_MAPPER;
    }
  
  if ( !strcmp(p->GetVTKClassName(), "vtkUnstructuredGridVolumeRayCastMapper" ) )
    {
    return vtkSMDataObjectDisplayProxy::BUNYK_RAY_CAST_VOLUME_MAPPER;
    }
  
  return vtkSMDataObjectDisplayProxy::UNKNOWN_VOLUME_MAPPER;
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetVisibility(int visible)
{
  this->Visibility = visible;
  int geom_visibility = (!this->VolumeRenderMode && visible)? 1 : 0;
  int vol_visibility = (this->VolumeRenderMode && visible)? 1 : 0;

  if (!this->ActorProxy)
    {
    return;
    }
  vtkSMIntVectorProperty* ivp;
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->ActorProxy->GetProperty("Visibility"));
  if (ivp->GetElement(0) != geom_visibility)
    {
    ivp->SetElement(0, geom_visibility);
    this->ActorProxy->UpdateVTKObjects();
    }
 
  if (this->HasVolumePipeline)
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->VolumeActorProxy->GetProperty("Visibility"));
    if (ivp->GetElement(0) != vol_visibility)
      {
      ivp->SetElement(0, vol_visibility);
      this->VolumeActorProxy->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::CacheUpdate(int idx, int total)
{
  if (!this->UpdateSuppressorProxy)
    {
    vtkErrorMacro("Objects not created yet.");
    return;
    }

  
  vtkSMIntVectorProperty* ivp;
  // Cache at the appropriate update suppressor depending
  // on if we are rendering volume or polygons.
  if (this->VolumeRenderMode)
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->VolumeUpdateSuppressorProxy->GetProperty("CacheUpdate"));
    }
  else
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->UpdateSuppressorProxy->GetProperty("CacheUpdate"));
    }
  ivp->SetElement(0, idx);
  ivp->SetElement(1, total);
  this->UpdateVTKObjects();
  
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  // TODO: Must propertify this.....(or overcome it all together).
  vtkClientServerStream stream;
  stream
    << vtkClientServerStream::Invoke
    << this->MapperProxy->GetID(0) << "Modified"
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER, stream);
  
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::InvalidateGeometryInternal(int useCache)
{
  this->VolumeGeometryIsValid = 0;
  this->GeometryInformationIsValid = 0;
  if (!useCache)
    {
    this->GeometryIsValid = 0;
    if (this->UpdateSuppressorProxy)
      {
      vtkSMProperty *p = 
        this->UpdateSuppressorProxy->GetProperty("RemoveAllCaches");
      p->Modified();
      this->UpdateSuppressorProxy->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::Update()
{
  if (this->VolumeRenderMode)
    {
    if (this->VolumeGeometryIsValid || !this->VolumeUpdateSuppressorProxy)
      {
      return;
      }
    vtkSMProperty *p
      = this->VolumeUpdateSuppressorProxy->GetProperty("ForceUpdate");
    p->Modified();
    this->VolumeGeometryIsValid = 1;
    }
  else
    {
    if (this->GeometryIsValid || !this->UpdateSuppressorProxy)
      {
      return;
      }
    vtkSMProperty* p = this->UpdateSuppressorProxy->GetProperty("ForceUpdate");
    p->Modified();
    this->GeometryIsValid = 1;
    this->GeometryInformationIsValid = 0;
    }
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Display proxy not created!");
    return;
    }
  // add this->ActorProxy to the render module.
  this->Superclass::AddToRenderModule(rm);
  if (this->HasVolumePipeline)
    {
    this->AddPropToRenderer(this->VolumeActorProxy, rm);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Display proxy not created!");
    return;
    }
  // removes this->ActorProxy from the render module.
  this->Superclass::RemoveFromRenderModule(rm);
  if (this->HasVolumePipeline)
    {
    this->RemovePropFromRenderer(this->VolumeActorProxy, rm);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::GatherGeometryInformation()
{
  this->GeometryInformation->Initialize();
  if (this->GeometryFilterProxy->GetNumberOfIDs() < 1)
    {
    vtkErrorMacro("Display has no associated object, can not gather info.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  pm->SendPrepareProgress(this->ConnectionID);
  this->Update();
  pm->SendCleanupPendingProgress(this->ConnectionID);

  int num, i;
  vtkPVGeometryInformation* information;
  num = this->GeometryFilterProxy->GetNumberOfIDs();
  information = vtkPVGeometryInformation::New();
  for (i = 0; i < num; ++i)
    {
    pm->GatherInformation(this->ConnectionID,
      this->GeometryFilterProxy->GetServers(),
      information, this->GeometryFilterProxy->GetID(i));
    this->GeometryInformation->AddInformation(information);
    }
  information->Delete();
  // Skip generation of names.
  this->GeometryInformationIsValid = 1;
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetInputAsGeometryFilter(vtkSMProxy *onProxy)
{
  if (!onProxy || !this->GeometryFilterProxy)
    {
    return;
    }
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    onProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find proeprty Input.");
    return;
    }
  ip->AddProxy(this->GeometryFilterProxy);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetInterpolationCM(int flag)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Interpolation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Interpolation on Display Proxy.");
    return ;
    }
  ivp->SetElement(0, flag);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetInterpolationCM()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Interpolation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Interpolation on Display Proxy.");
    return -1;
    } 
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetPointSizeCM(double size)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("PointSize"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property PointSize on DisplayProxy.");
    return ;
    }
  dvp->SetElement(0, size);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
double vtkSMDataObjectDisplayProxy::GetPointSizeCM()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("PointSize"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property PointSize on DisplayProxy.");
    return 0.0;
    }
  return dvp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetLineWidthCM(double width)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("LineWidth"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property LineWidth on DisplayProxy.");
    return ;
    }
  dvp->SetElement(0, width);
  this->UpdateVTKObjects(); 
}

//-----------------------------------------------------------------------------
double vtkSMDataObjectDisplayProxy::GetLineWidthCM()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("LineWidth"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property LineWidth on DisplayProxy.");
    return 0.0;
    }
  return dvp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetScalarModeCM(int mode)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ScalarMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, mode);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetScalarModeCM()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ScalarMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return -1;
    }
  return ivp->GetElement(0);
}
//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetColorModeCM(int mode)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ColorMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, mode);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetColorModeCM()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ColorMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetScalarArrayCM(const char* arrayname)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty("SelectScalarArray"));
  
  if (!svp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return;
    }
  svp->SetElement(0, arrayname);
  this->UpdateVTKObjects();
    
}

//-----------------------------------------------------------------------------
const char* vtkSMDataObjectDisplayProxy::GetScalarArrayCM()
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty("SelectScalarArray"));

  if (!svp)
    {
    vtkErrorMacro("Failed to find property ScalarMode on DisplayProxy.");
    return 0;
    }
  return svp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetOpacityCM(double op)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Opacity"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Opacity on DisplayProxy.");
    return ;
    }
  dvp->SetElement(0, op);
  this->UpdateVTKObjects(); 

}

//-----------------------------------------------------------------------------
double vtkSMDataObjectDisplayProxy::GetOpacityCM()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Opacity"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Opacity on DisplayProxy.");
    return 0;
    }
  return dvp->GetElement(0);
}


//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetColorCM(double rgb[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Color"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Color on DisplayProxy.");
    return;
    }
  dvp->SetElements(rgb);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::GetColorCM(double rgb[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Color"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Color on DisplayProxy.");
    return;
    }
  rgb[0] = dvp->GetElement(0);
  rgb[1] = dvp->GetElement(1);
  rgb[2] = dvp->GetElement(2);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetInterpolateScalarsBeforeMappingCM(int flag)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("InterpolateScalarsBeforeMapping"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property InterpolateScalarsBeforeMapping on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, flag);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetInterpolateScalarsBeforeMappingCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("InterpolateScalarsBeforeMapping"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property InterpolateScalarsBeforeMapping on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetScalarVisibilityCM(int v)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ScalarVisibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarVisibility on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, v);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetScalarVisibilityCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ScalarVisibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ScalarVisibility on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetPositionCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Position on DisplayProxy.");
    return;
    }
  dvp->SetElements(pos);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::GetPositionCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Position"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Position on DisplayProxy.");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
  pos[2] = dvp->GetElement(2);
}
//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::GetScaleCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Scale"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Scale on DisplayProxy.");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
  pos[2] = dvp->GetElement(2);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetScaleCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Scale"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Scale on DisplayProxy.");
    return;
    }
  dvp->SetElements(pos);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::GetOrientationCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Orientation"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Orientation on DisplayProxy.");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
  pos[2] = dvp->GetElement(2);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetOrientationCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Orientation"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Orientation on DisplayProxy.");
    return;
    }
  dvp->SetElements(pos);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::GetOriginCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Origin"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Origin on DisplayProxy.");
    return;
    }
  pos[0] = dvp->GetElement(0);
  pos[1] = dvp->GetElement(1);
  pos[2] = dvp->GetElement(2);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetOriginCM(double pos[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Origin"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Origin on DisplayProxy.");
    return;
    }
  dvp->SetElements(pos);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetRepresentationCM(int r)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Representation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Representation on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, r);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetRepresentationCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Representation"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Representation on DisplayProxy.");
    return 0;
    } 
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetImmediateModeRenderingCM(int i)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ImmediateModeRendering"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ImmediateModeRendering.");
    return;
    }
  ivp->SetElement(0, i);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetImmediateModeRenderingCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ImmediateModeRendering"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ImmediateModeRendering.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetMaterialCM(const char* name)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Shading"));
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty("Material"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property Material on display proxy.");
    return;
    }
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Shading on display proxy.");
    return;
    }
  if (!name || strlen(name) == 0)
    {
    ivp->SetElement(0, 0);
    svp->SetElement(0, "");
    }
  else
    {
    svp->SetElement(0, name);
    ivp->SetElement(0, 1);
    }

  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
const char* vtkSMDataObjectDisplayProxy::GetMaterialCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Shading"));
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetProperty("Material"));
  if (!svp)
    {
    vtkErrorMacro("Failed to find property Material on display proxy.");
    return 0;
    }
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Shading on display proxy.");
    return 0;
    }
  if (!ivp->GetElement(0))
    {
    return 0;
    }
  return svp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GeometryFilterProxy: " << this->GeometryFilterProxy << endl;
  os << indent << "UpdateSuppressorProxy: " << this->UpdateSuppressorProxy << endl;
  os << indent << "MapperProxy: " << this->MapperProxy << endl;
  os << indent << "PropertyProxy: " << this->PropertyProxy << endl;
  os << indent << "ActorProxy: " << this->ActorProxy << endl;
  os << indent << "GeometryIsValid: " << this->GeometryIsValid << endl;
  os << indent << "VolumeGeometryIsValid: "
     << this->VolumeGeometryIsValid << endl;
  os << indent << "HasVolumePipeline: " << this->HasVolumePipeline << endl;
  os << indent << "VolumeRenderMode: " << this->VolumeRenderMode << endl;
  os << indent << "SupportsBunykMapper: " << this->SupportsBunykMapper << endl;
  os << indent << "SupportsZSweepMapper: " << this->SupportsZSweepMapper << endl;
}
