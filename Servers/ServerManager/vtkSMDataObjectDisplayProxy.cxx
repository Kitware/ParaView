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
#include "vtkCommand.h"
#include "vtkHAVSVolumeMapper.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVOpenGLExtensionsInformation.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMMaterialLoaderProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMDataObjectDisplayProxy);
vtkCxxRevisionMacro(vtkSMDataObjectDisplayProxy, "1.41");


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

  this->VolumePTMapperProxy = 0;
  this->VolumeHAVSMapperProxy = 0;
  this->VolumeBunykMapperProxy = 0;
  this->VolumeZSweepMapperProxy = 0;

  this->VolumeFixedPointRayCastMapperProxy = 0;

  this->VolumeFilterProxy = 0;
  this->VolumeUpdateSuppressorProxy = 0;
  this->VolumeActorProxy = 0;
  this->VolumePropertyProxy = 0;

  // We haven't determined if this display can support volume rendering
  // hence we mark it INVALID.
  this->VolumePipelineType    = INVALID;

  this->SupportsBunykMapper  = 0;
  this->SupportsZSweepMapper = 0;
  this->SupportsHAVSMapper   = 0;
  this->VolumeRenderMode     = 0;

  this->Visibility = 1;
  this->Representation = -1;

  this->DisplayedDataInformationIsValid = 0;
  this->DisplayedDataInformation = vtkPVGeometryInformation::New();

  this->CacherProxy = 0;
  this->VolumeCacherProxy = 0;

  this->RenderModuleExtensionsTested = 0;
  this->UpdateTime = 0.0;

  this->ColorArrayLink = vtkSMPropertyLink::New();
  this->LookupTableLink = vtkSMPropertyLink::New();
}

//-----------------------------------------------------------------------------
vtkSMDataObjectDisplayProxy::~vtkSMDataObjectDisplayProxy()
{
  this->GeometryFilterProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0; 
  this->PropertyProxy = 0;
  this->ActorProxy = 0;

  this->VolumePTMapperProxy = 0;
  this->VolumeHAVSMapperProxy = 0;
  this->VolumeBunykMapperProxy = 0;
  this->VolumeZSweepMapperProxy = 0;
  this->VolumeFilterProxy = 0;

  this->VolumeFixedPointRayCastMapperProxy = 0;

  this->VolumeUpdateSuppressorProxy = 0;
  this->VolumeActorProxy = 0;
  this->VolumePropertyProxy = 0;
  this->DisplayedDataInformation->Delete();

  this->ColorArrayLink->Delete();
  this->LookupTableLink->Delete();
}

//-----------------------------------------------------------------------------
bool vtkSMDataObjectDisplayProxy::Connect(vtkSMProxy* consumer, vtkSMProxy* producer)
{
  if (!consumer)
    {
    return false;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    consumer->GetProperty("Input"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(producer);
    consumer->UpdateProperty("Input");
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::CreateVTKObjects()
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
  this->MapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->ActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->PropertyProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  // Volume Stuff.
  this->VolumeFilterProxy = this->GetSubProxy("VolumeFilter");
  this->VolumePTMapperProxy = this->GetSubProxy("VolumePTMapper");
  this->VolumeHAVSMapperProxy = this->GetSubProxy("VolumeHAVSMapper");
  this->VolumeBunykMapperProxy = this->GetSubProxy("VolumeBunykMapper");
  this->VolumeZSweepMapperProxy = this->GetSubProxy("VolumeZSweepMapper");

  this->VolumeFilterProxy->SetServers(vtkProcessModule::DATA_SERVER);
  this->VolumePTMapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeHAVSMapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);    
  this->VolumeBunykMapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeZSweepMapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->VolumeFixedPointRayCastMapperProxy =
    this->GetSubProxy("VolumeFixedPointRayCastMapper");
  this->VolumeFixedPointRayCastMapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->VolumeUpdateSuppressorProxy = 
    this->GetSubProxy("VolumeUpdateSuppressor");
  this->VolumeUpdateSuppressorProxy->SetServers(
    vtkProcessModule::CLIENT_AND_SERVERS);
  this->VolumeActorProxy = this->GetSubProxy("VolumeActor");
  this->VolumeActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumePropertyProxy = this->GetSubProxy("VolumeProperty");
  this->VolumePropertyProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects();

  // Link "ColorArray" property to 
  // "SelectScalarArray" property of the VolumeMappers.
  this->ColorArrayLink->AddLinkedProperty(
    this->MapperProxy->GetProperty("ColorArray"), vtkSMLink::INPUT);
  this->ColorArrayLink->AddLinkedProperty(
    this->GetSubProxy("VolumeDummyMapper")->GetProperty("SelectScalarArray"), 
    vtkSMLink::OUTPUT);

  // Link "LookupTable" from geometry mapper
  // to "ColorTransferFunction" on  volume property.
  this->LookupTableLink->AddLinkedProperty(
    this->MapperProxy->GetProperty("LookupTable"), vtkSMLink::INPUT);
  this->LookupTableLink->AddLinkedProperty(
    this->VolumePropertyProxy->GetProperty("ColorTransferFunction"),
    vtkSMLink::OUTPUT);

  vtkSMMaterialLoaderProxy* mlp = vtkSMMaterialLoaderProxy::SafeDownCast(
    this->GetSubProxy("MaterialLoader"));
  if (mlp)
    {
    mlp->SetPropertyProxy(this->PropertyProxy);
    }

  // Set the default cache keepers.
  this->CacherProxy = this->UpdateSuppressorProxy;
  this->VolumeCacherProxy = this->VolumeUpdateSuppressorProxy;
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::AddInput(
  vtkSMSourceProxy* input, const char*, int)
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
vtkSMProxy * vtkSMDataObjectDisplayProxy::GetInput(int i)
{
  vtkSMProxy *ret = NULL;
  vtkSMInputProperty* ip = 
    vtkSMInputProperty::SafeDownCast(this->GetProperty("Input"));
  if (ip)
    {
    ret = ip->GetProxy(i);
    }
  return ret;
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetInputInternal(vtkSMSourceProxy* input)
{
  if (!input)
    {
    return;
    }

  input->CreateParts();
  int numInputs = input->GetNumberOfParts();
  if (numInputs == 0)
    {
    vtkErrorMacro("Input proxy has no output! Cannot create the display");
    return;
    }
  
  // This will create all the subproxies with correct number of parts.
  this->CanCreateProxy = 1;

  // We haven't determined yet if volume rendering is supported.
  this->VolumePipelineType = INVALID;

  this->CreateVTKObjects();

  // We only set up the geometry pipeline.
  this->Connect(this->GeometryFilterProxy, input);
  // First, setup the pipeline.
  this->SetupPipeline();
  // Second, set default property values.
  this->SetupDefaults();

  // We leave the volume pipeline uninitialized.
  // It will be setup once we know more about the input data
  // type.
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetTexture(vtkSMProxy *texture)
{
  if (!this->ActorProxy)
    {
    return;
    }

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->ActorProxy->GetProperty("Texture"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Texture on ActorProxy.");
    return;
    }
  
  pp->RemoveAllProxies();
  if (texture)
    {
    pp->AddProxy(texture);
    }

  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDataObjectDisplayProxy::GetTexture()
{
  if (!this->ActorProxy)
    {
    return 0;
    }
  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->ActorProxy->GetProperty("Texture"));
  if (!pp || !pp->GetNumberOfProxies())
    {
    return 0;
    }
  return pp->GetProxy(0);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetupPipeline()
{
  this->Connect(this->UpdateSuppressorProxy, this->GeometryFilterProxy);
  this->Connect(this->MapperProxy, this->UpdateSuppressorProxy);

  vtkSMProxyProperty* pp = 0;
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

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GeometryFilterProxy->GetProperty("UseStrips"));
  ivp->SetElement(0, 0);

  //TODO: stuff for logging geometry filter times.
  vtkClientServerStream stream;
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
         << this->GeometryFilterProxy->GetID() 
         << "AddObserver"
         << "StartEvent"
         << start
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke 
         << this->GeometryFilterProxy->GetID() 
         << "AddObserver"
         << "EndEvent"
         << end
         << vtkClientServerStream::End;
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
    this->PropertyProxy->GetProperty("Diffuse"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Diffuse on PropertyProxy.");
    return;
    }
  dvp->SetElement(0, 1.0);

  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID() << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->UpdateSuppressorProxy->GetID() << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 this->UpdateSuppressorProxy->GetServers(), stream);
  
  if (pm->GetStreamBlock())
    {
    // This is here just for streaming (can be removed if streaming is removed).
    vtkClientServerStream stream2;
    stream2
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->MapperProxy->GetID() << "SetNumberOfPieces"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    stream2
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetPartitionId"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->MapperProxy->GetID() << "SetPiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;

    // Do we need to client too?
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::RENDER_SERVER, stream2);  
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetupVolumePipeline()
{
  if (this->VolumePipelineType == NONE)
    {
    return;
    }

  vtkSMProxyProperty* pp;

  vtkSMProxy* input = this->GetInput(0);
  if (this->VolumePipelineType == UNSTRUCTURED_GRID)
    {
    this->Connect(this->VolumeFilterProxy, input);
    this->Connect(this->VolumeUpdateSuppressorProxy, this->VolumeFilterProxy);
    this->Connect(this->VolumePTMapperProxy, this->VolumeUpdateSuppressorProxy);
    this->Connect(this->VolumeHAVSMapperProxy, this->VolumeUpdateSuppressorProxy);
    this->Connect(this->VolumeBunykMapperProxy, this->VolumeUpdateSuppressorProxy);
    this->Connect(this->VolumeZSweepMapperProxy, this->VolumeUpdateSuppressorProxy);

    pp = vtkSMProxyProperty::SafeDownCast(
      this->VolumeActorProxy->GetProperty("Mapper"));
    if (!pp)
      {
      vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
      return;
      }
    pp->RemoveAllProxies();
    if (this->SupportsHAVSMapper)
      {
      pp->AddProxy(this->VolumeHAVSMapperProxy);
      }
    else
      {
      pp->AddProxy(this->VolumePTMapperProxy);
      }
    this->VolumeActorProxy->UpdateVTKObjects();
    }
  else if (this->VolumePipelineType == IMAGE_DATA)
    {
    this->Connect(this->VolumeUpdateSuppressorProxy, input);
    this->Connect(this->VolumeFixedPointRayCastMapperProxy, 
      this->VolumeUpdateSuppressorProxy);

    pp = vtkSMProxyProperty::SafeDownCast(
      this->VolumeActorProxy->GetProperty("Mapper"));
    if (!pp)
      {
      vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
      return;
      }
    pp->RemoveAllProxies();
    pp->AddProxy(this->VolumeFixedPointRayCastMapperProxy);
    this->VolumeActorProxy->UpdateVTKObjects();
    }

  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActorProxy->GetProperty("Property"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Property on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->VolumePropertyProxy);
  this->VolumeActorProxy->UpdateVTKObjects();

  this->VolumePropertyProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetupVolumeDefaults()
{
  if (this->VolumePipelineType == NONE)
    {
    return;
    }

  // VolumeFilterProxy  defaults.
  // No defaults to set.

  // VolumePTMapperProxy defaults.
  // No defaults to set.

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("ProcessModule should be set before setting up the display "
                  "pipeline.");
    return;
    }
  vtkClientServerStream stream;
  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->VolumeUpdateSuppressorProxy->GetID()
    << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->VolumeUpdateSuppressorProxy->GetID() << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;

  pm->SendStream(this->ConnectionID,
    this->VolumeUpdateSuppressorProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
vtkPVGeometryInformation* vtkSMDataObjectDisplayProxy::GetDisplayedDataInformation()
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return 0;
    }
  if (!this->DisplayedDataInformationIsValid)
    {
    this->GatherDisplayedDataInformation();
    }
  return this->DisplayedDataInformation;
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
  
  this->Representation = representation;
  vtkSMIntVectorProperty* ivp;
  if (representation == vtkSMDataObjectDisplayProxy::VOLUME)
    {
    if (this->VolumePipelineType == NONE)
      {
      vtkErrorMacro("Display does not have Volume Rendering support.");
      return;
      }

    // Notice that we allow the representation to be volume
    // if we haven't determined if the display can render volumes.
    // If we later realize that the data is not suitable for
    // volume rendering, we flash an error and switch representation
    // to Surface. This makes it possible to load state for displays
    // with volume rendering enabled and still deferring the 
    // volume check till Update().
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

/* OBSOLETE CODE: keeping it in as a reference until the GUI code 
 * has been updated.
//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::ResetTransferFunctions()
{
  if (this->VolumePipelineType == INVALID)
    {
    vtkErrorMacro(
      "Please Update the display before calling ResetTransferFunctions().");
    return;
    }

  if (this->VolumePipelineType == NONE)
    {
    vtkErrorMacro("This display does not support Volume Rendering.");
    return;
    }

  vtkSMIntVectorProperty* ivp;
  vtkSMStringVectorProperty* svp;
  vtkSMInputProperty* ip;
  
  int mode;
  const char* arrayname = 0;
  vtkSMSourceProxy* sp = 0;
  
  if (this->VolumePipelineType == UNSTRUCTURED_GRID)
    {
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
      vtkErrorMacro("Either no input set or too many inputs set for "
                    "the DisplayProxy.");
      return;
      }
    sp = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0));
    if (!sp)
      {
      vtkErrorMacro("Input to a DisplayProxy must be a source proxy.");
      return;
      }
    }
  else if (this->VolumePipelineType == IMAGE_DATA)
    {
    // 1) Determine the scalar mode. (Point data or cell data?).
    mode = vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA;

    // 2) Determine the array used for volume rendering.
    svp = vtkSMStringVectorProperty::SafeDownCast(
      this->VolumeFixedPointRayCastMapperProxy->GetProperty(
        "SelectScalarArray"));
    arrayname = svp->GetElement(0);

    // 3) Get the Input Proxy.
    ip = vtkSMInputProperty::SafeDownCast(
      this->VolumeUpdateSuppressorProxy->GetProperty("Input"));
    if (ip->GetNumberOfProxies() != 1)
      {
      vtkErrorMacro("Either no input set or too many inputs set for "
                    "the DisplayProxy.");
      return;
      }
    sp = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0));
    if (!sp)
      {
      vtkErrorMacro("Input to a DisplayProxy must be a source proxy.");
      return;
      }
    }
  
  if (!arrayname)
    {
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
*/

/*
//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetVolumeMapperToBunykCM()
{
  if ( this->VolumePipelineType != UNSTRUCTURED_GRID )
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
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActorProxy.");
    }
  pp->SetProxy(0, this->VolumeBunykMapperProxy);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetVolumeMapperToPTCM()
{
  if ( this->VolumePipelineType != UNSTRUCTURED_GRID )
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
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActorProxy.");
    }
  pp->SetProxy(0, this->VolumePTMapperProxy);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetVolumeMapperToHAVSCM()
{
  if ( this->VolumePipelineType != UNSTRUCTURED_GRID)
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
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActorProxy.");
    }
  pp->SetProxy(0, this->VolumeHAVSMapperProxy);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetVolumeMapperToZSweepCM()
{
  if ( this->VolumePipelineType != UNSTRUCTURED_GRID )
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
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActorProxy.");
    }
  pp->SetProxy(0, this->VolumeZSweepMapperProxy);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetVolumeMapperTypeCM()
{
  if ( this->VolumePipelineType == NONE  || 
    this->VolumePipelineType == INVALID)
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

  if ( !strcmp(p->GetVTKClassName(), "vtkHAVSVolumeMapper" ) )
    {
    return vtkSMDataObjectDisplayProxy::HAVS_VOLUME_MAPPER;
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
*/

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
 
  if (this->VolumePipelineType != NONE)
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

  // Cache at the appropriate update suppressor depending
  // on if we are rendering volume or polygons.
  vtkSMProxy* cacheKeeper = (this->VolumeRenderMode)?
    this->VolumeCacherProxy : this->CacherProxy;

  if (!cacheKeeper)
    {
    vtkWarningMacro("Failed to locate the cache keeper proxy in the pipeline."
      "Cannot update cache.");
    return;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    cacheKeeper->GetProperty("CacheUpdate"));
  ivp->SetElement(0, idx);
  ivp->SetElement(1, total);
  cacheKeeper->UpdateProperty("CacheUpdate");
  
  // I don't like calling Modified directly, but I need the scalars to be
  // remapped through the lookup table, and this causes that to happen.
  // TODO: Must propertify this.....(or overcome it all together).
  vtkClientServerStream stream;
  stream
    << vtkClientServerStream::Invoke
    << this->MapperProxy->GetID() << "Modified"
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER, stream);
  
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::InvalidateGeometryInternal(int useCache)
{
  this->VolumeGeometryIsValid = 0;
  this->DisplayedDataInformationIsValid = 0;
  if (!useCache)
    {
    this->GeometryIsValid = 0;
    if (this->CacherProxy)
      {
      this->CacherProxy->InvokeCommand("RemoveAllCaches");
      }
    if (this->VolumeCacherProxy)
      {
      this->VolumeCacherProxy->InvokeCommand("RemoveAllCaches");
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::UpdateRequired()
{
  if (this->VolumePipelineType == INVALID || !this->RenderModuleExtensionsTested)
    {
    return 1;
    }

  if (this->VolumeRenderMode)
    {
    if (this->VolumeGeometryIsValid || !this->VolumeUpdateSuppressorProxy)
      {
      return 0;
      }
    }
  else
    {
    if (this->GeometryIsValid || !this->UpdateSuppressorProxy)
      {
      return 0;
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::DetermineVolumeSupport()
{
  if (this->VolumePipelineType != INVALID)
    {
    // Already determined support.
    return;
    }

  this->VolumePipelineType = NONE;

  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(this->GetInput(0));
  vtkSMDataTypeDomain* domain = vtkSMDataTypeDomain::SafeDownCast(
    this->VolumeFilterProxy->GetProperty("Input")->GetDomain("input_type"));
  if (domain && domain->IsInDomain(input))
    {
    this->VolumePipelineType = UNSTRUCTURED_GRID;

    vtkPVDataInformation* datainfo = input->GetDataInformation();
    if (datainfo->GetNumberOfCells() < 1000000)
      {
      this->SupportsZSweepMapper = 1;
      }
    if (datainfo->GetNumberOfCells() < 500000)
      {
      this->SupportsBunykMapper = 1;
      }
    // HAVS support is determined when the display is added to a render 
    // module.
    }
  else 
    {
    domain = vtkSMDataTypeDomain::SafeDownCast(
      this->VolumeFixedPointRayCastMapperProxy->GetProperty("Input")->
      GetDomain("input_type"));
    if (domain && domain->IsInDomain(input))
      {
      this->VolumePipelineType = IMAGE_DATA;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::UpdateRenderModuleExtensions(
  vtkSMAbstractViewModuleProxy* view)
{
  vtkSMRenderModuleProxy* rm = vtkSMRenderModuleProxy::SafeDownCast(view);
  if (!rm)
    {
    return;
    }
  vtkPVOpenGLExtensionsInformation* glinfo =
    rm->GetOpenGLExtensionsInformation();
  if (glinfo)
    {
    // These are extensions needed for HAVS. It would be nice
    // if these was some way of asking the HAVS mapper the extensions
    // it needs rather than hardcoding it here.
    int supports_GL_EXT_texture3D =
      glinfo->ExtensionSupported( "GL_EXT_texture3D");
    int supports_GL_EXT_framebuffer_object =
      glinfo->ExtensionSupported( "GL_EXT_framebuffer_object");
    int supports_GL_ARB_fragment_program =
      glinfo->ExtensionSupported( "GL_ARB_fragment_program" );
    int supports_GL_ARB_vertex_program =
      glinfo->ExtensionSupported( "GL_ARB_vertex_program" );
    int supports_GL_ARB_texture_float =
      glinfo->ExtensionSupported( "GL_ARB_texture_float" );
    int supports_GL_ATI_texture_float =
      glinfo->ExtensionSupported( "GL_ATI_texture_float" );

    if ( !supports_GL_EXT_texture3D ||
      !supports_GL_EXT_framebuffer_object ||
      !supports_GL_ARB_fragment_program ||
      !supports_GL_ARB_vertex_program ||
      !(supports_GL_ARB_texture_float || supports_GL_ATI_texture_float))
      {
      this->SupportsHAVSMapper = 0;
      }
    else
      {
      this->SupportsHAVSMapper = 1;
      }
    }
  this->RenderModuleExtensionsTested = 1;
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetUpdateTime(double time)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created!");
    return;
    }

  this->UpdateTime = time;

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("UpdateTime"));
  dvp->SetElement(0, time);
  // UpdateTime is immediate update, so no need to update.

  // Go upstream to the reader and mark it modified.
  this->MarkUpstreamModified();
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::Update(vtkSMAbstractViewModuleProxy* view)
{
  if (!this->RenderModuleExtensionsTested)
    {
    this->UpdateRenderModuleExtensions(view);
    }

  if (this->VolumeRenderMode && this->VolumePipelineType == NONE)
    {
    vtkErrorMacro("The display's input cannot be rendered as a volume."
      << " Switching to surface rendering.");
    this->SetRepresentation(vtkSMDataObjectDisplayProxy::SURFACE);
    }

  if (this->VolumeRenderMode)
    {
    if (this->VolumeGeometryIsValid || !this->VolumeUpdateSuppressorProxy)
      {
      return;
      }
    this->VolumeUpdateSuppressorProxy->InvokeCommand("ForceUpdate");
    this->VolumeGeometryIsValid = 1;
    }
  else
    {
    if (this->GeometryIsValid || !this->UpdateSuppressorProxy)
      {
      return;
      }
    this->UpdateSuppressorProxy->InvokeCommand("ForceUpdate");
    this->GeometryIsValid = 1;
    this->DisplayedDataInformationIsValid = 0;
    }

  // Do this after the update so that the first update is not caused by 
  // getting the data information. This assumes that the default 
  // representation is never volume rendering.
  if (this->VolumePipelineType == INVALID)
    {
    this->DetermineVolumeSupport();

    // We haven't determined if the pipeline can support volume
    // rendering. Determine that and accordingly update the
    // display pipeline.
    this->SetupVolumePipeline();

    this->SetupVolumeDefaults();
    }

  this->InvokeEvent(vtkSMAbstractDisplayProxy::ForceUpdateEvent);
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
  
  // We cannnot rely on this->VolumePipelineType being set
  // when this gets called since the display can be added to a render module
  // before it is updated. Hence, we assume that volume rendering is supported
  // and always add the volume rendering actor.
  this->AddPropToRenderer(this->VolumeActorProxy, rm);

  // This will ensure that on update we'll check if the
  // view supports certain extensions.
  this->RenderModuleExtensionsTested = 0;

  // We don't support HAVS unless we've verified that we do.
  this->SupportsHAVSMapper = 0;
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Display proxy not created!");
    return;
    }
  this->SupportsHAVSMapper = 0;
  // removes this->ActorProxy from the render module.
  this->Superclass::RemoveFromRenderModule(rm);
  this->RemovePropFromRenderer(this->VolumeActorProxy, rm);
}

//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::GatherDisplayedDataInformation()
{
  this->DisplayedDataInformation->Initialize();
  if (this->GeometryFilterProxy->GetID().IsNull())
    {
    vtkErrorMacro("Display has no associated object, can not gather info.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  pm->SendPrepareProgress(this->ConnectionID);
  this->Update();
  pm->SendCleanupPendingProgress(this->ConnectionID);

  if (this->Representation != VOLUME)
    {
    vtkPVGeometryInformation* information;
    information = vtkPVGeometryInformation::New();
    pm->GatherInformation(this->ConnectionID,
                          this->GeometryFilterProxy->GetServers(),
                          information, this->GeometryFilterProxy->GetID());
    this->DisplayedDataInformation->AddInformation(information);
    information->Delete();
    }
  else
    {
    vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(this->GetInput(0));
    if (input)
      {
      this->DisplayedDataInformation->AddInformation(
        input->GetDataInformation());
      }
    }

  // Skip generation of names.
  this->DisplayedDataInformationIsValid = 1;
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
    vtkErrorMacro("Failed to find property Input.");
    return;
    }
  ip->AddProxy(this->GeometryFilterProxy);
}

/*
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
*/

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

/*
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
*/

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

/*
//-----------------------------------------------------------------------------
void vtkSMDataObjectDisplayProxy::SetColorModeCM(int mode)
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("ColorMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ColorMode on DisplayProxy.");
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
    vtkErrorMacro("Failed to find property ColorMode on DisplayProxy.");
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
    vtkErrorMacro("Failed to find property SelectScalarArray on DisplayProxy.");
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
    vtkErrorMacro("Failed to find property SelectScalarArray on DisplayProxy.");
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
*/

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

/*
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
*/

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

/*
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
*/

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

/*
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
void vtkSMDataObjectDisplayProxy::SetPickableCM(int op)
{
  vtkSMIntVectorProperty* dvp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Pickable"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Pickable on DisplayProxy.");
    return ;
    }
  dvp->SetElement(0, op);
  this->UpdateVTKObjects(); 

}

//-----------------------------------------------------------------------------
int vtkSMDataObjectDisplayProxy::GetPickableCM()
{
  vtkSMIntVectorProperty* dvp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Pickable"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Pickable on DisplayProxy.");
    return 0;
    }
  return dvp->GetElement(0);
}
*/

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
  os << indent << "VolumePipelineType: " << this->VolumePipelineType << endl;
  os << indent << "VolumeRenderMode: " << this->VolumeRenderMode << endl;
  os << indent << "SupportsHAVSMapper: " << this->SupportsHAVSMapper << endl;
  os << indent << "SupportsBunykMapper: " << this->SupportsBunykMapper << endl;
  os << indent << "SupportsZSweepMapper: " << this->SupportsZSweepMapper << endl;
  os << indent << "UpdateTime: " << this->UpdateTime << endl;
}
