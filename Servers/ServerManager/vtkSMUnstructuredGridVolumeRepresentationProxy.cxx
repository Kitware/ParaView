/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredGridVolumeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUnstructuredGridVolumeRepresentationProxy.h"

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkPVOpenGLExtensionsInformation.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMUnstructuredGridVolumeRepresentationProxy);
vtkCxxRevisionMacro(vtkSMUnstructuredGridVolumeRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeRepresentationProxy::vtkSMUnstructuredGridVolumeRepresentationProxy()
{
  this->VolumeFilter = 0;
  this->VolumePTMapper = 0;
  this->VolumeHAVSMapper = 0;
  this->VolumeBunykMapper = 0;
  this->VolumeZSweepMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;

  this->ExtractSelection = 0;
  this->SelectionGeometryFilter = 0;
  this->SelectionMapper = 0;
  this->SelectionLODMapper = 0;
  this->SelectionProp3D = 0;
  this->SelectionProperty = 0;

  this->SupportsBunykMapper  = 0;
  this->SupportsZSweepMapper = 0;
  this->SupportsHAVSMapper   = 0;
  this->RenderViewExtensionsTested = 0;

  // This representation supports selection.
  this->SetSelectionSupported(true);
}

//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeRepresentationProxy::~vtkSMUnstructuredGridVolumeRepresentationProxy()
{
  this->VolumeFilter = 0;
  this->VolumePTMapper = 0;
  this->VolumeHAVSMapper = 0;
  this->VolumeBunykMapper = 0;
  this->VolumeZSweepMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;

  this->ExtractSelection = 0;
  this->SelectionGeometryFilter = 0;
  this->SelectionMapper = 0;
  this->SelectionLODMapper = 0;
  this->SelectionProp3D = 0;
  this->SelectionProperty = 0;
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::GetSelectionVisibility()
{
  if (!this->Superclass::GetSelectionVisibility())
    {
    return false;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetProperty("Selection"));
  return (pp && pp->GetNumberOfProxies() > 0);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::UpdateSelectionPropVisibility()
{
  int visibility  = this->GetSelectionVisibility()? 1 : 0;
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->SelectionProp3D->GetProperty("Visibility"));
  ivp->SetElement(0, visibility);
  this->SelectionProp3D->UpdateProperty("Visibility");
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->UpdateSelectionPropVisibility();
  if (!this->RenderViewExtensionsTested)
    {
    this->UpdateRenderViewExtensions(view);
    }

  this->DetermineVolumeSupport();
  this->SetupVolumePipeline();

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  renderView->AddPropToRenderer(this->VolumeActor);
  if (this->GetSelectionSupported())
    {
    renderView->AddPropToRenderer(this->SelectionProp3D);
    }

  // This will ensure that on update we'll check if the
  // view supports certain extensions.
  this->RenderViewExtensionsTested = 0;

  // We don't support HAVS unless we've verified that we do.
  this->SupportsHAVSMapper = 0;

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->VolumeActor);
  renderView->RemovePropFromRenderer(this->SelectionProp3D);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a geometry filter, the data type fed into the strategy is
  // always polydata.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(
    view->NewStrategy(VTK_UNSTRUCTURED_GRID, vtkSMRenderViewProxy::VOLUME));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  this->SetStrategy(strategy);

  strategy->SetEnableLOD(false);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMPipelineRepresentationProxy::AddToView()).
  this->Connect(this->VolumeFilter, strategy, "Input");
  
  // Initialize strategy for the selection pipeline.
  strategy.TakeReference(
    view->NewStrategy(VTK_POLY_DATA, vtkSMRenderViewProxy::SURFACE));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("Could not create strategy for selection pipeline. Disabling selection.");
    this->SetSelectionSupported(false);
    }
  else
    {
    this->SetStrategyForSelection(strategy);
    strategy->SetEnableLOD(true);
    strategy->UpdateVTKObjects();

    strategy->SetInput(this->SelectionGeometryFilter);
    this->Connect(strategy->GetOutput(), this->SelectionMapper);
    this->Connect(strategy->GetLODOutput(), this->SelectionLODMapper);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::BeginCreateVTKObjects(int numObjects)
{
  if (!this->Superclass::BeginCreateVTKObjects(numObjects))
    {
    return false;
    }

  // Set server flags correctly on all subproxies.
  this->VolumeFilter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("VolumeFilter"));

  this->VolumePTMapper = this->GetSubProxy("VolumePTMapper");
  this->VolumeBunykMapper = this->GetSubProxy("VolumeBunykMapper");
  this->VolumeZSweepMapper = this->GetSubProxy("VolumeZSweepMapper");
  this->VolumeHAVSMapper = this->GetSubProxy("VolumeHAVSMapper");
  this->VolumeActor = this->GetSubProxy("VolumeActor");
  this->VolumeProperty = this->GetSubProxy("VolumeProperty");

  this->VolumeFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->VolumeBunykMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeZSweepMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumePTMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeHAVSMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeActor->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeProperty->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  // Initialize selection pipeline subproxies.
  this->ExtractSelection = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("ExtractSelection"));
  this->SelectionGeometryFilter = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("SelectionGeometryFilter"));
  this->SelectionMapper = this->GetSubProxy("SelectionMapper");
  this->SelectionLODMapper = this->GetSubProxy("SelectionLODMapper");
  this->SelectionProp3D = this->GetSubProxy("SelectionProp3D");
  this->SelectionProperty = this->GetSubProxy("SelectionProperty");

  this->ExtractSelection->SetServers(vtkProcessModule::DATA_SERVER);
  this->SelectionGeometryFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->SelectionMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->SelectionLODMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->SelectionProp3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->SelectionProperty->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::EndCreateVTKObjects(int numObjects)
{
  this->Connect(this->GetInputProxy(), this->VolumeFilter, "Input");
  this->Connect(this->VolumeBunykMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeHAVSMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumePTMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeZSweepMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeProperty, this->VolumeActor, "Property");

  // Setup selection pipeline connections.
  this->Connect(this->GetInputProxy(), this->ExtractSelection);
  this->Connect(this->ExtractSelection, this->SelectionGeometryFilter);
  this->Connect(this->SelectionMapper, this->SelectionProp3D, "Mapper");
  this->Connect(this->SelectionLODMapper, this->SelectionProp3D, "LODMapper");
  this->Connect(this->SelectionProperty, this->SelectionProp3D, "Property");

  // Selection prop is not pickable.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->SelectionProp3D->GetProperty("Pickable"));
  ivp->SetElement(0, 0);
  this->SelectionProp3D->UpdateProperty("Pickable");

  return this->Superclass::EndCreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::GetSelectableProps(
  vtkCollection* collection)
{
  collection->AddItem(this->VolumeActor);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::ConvertSurfaceSelectionToVolumeSelection(
  vtkSelection* input, vtkSelection* output)
{
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
    this->ConnectionID, input, output);
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::DetermineVolumeSupport()
{
  vtkSMDataTypeDomain* domain = vtkSMDataTypeDomain::SafeDownCast(
    this->VolumeFilter->GetProperty("Input")->GetDomain("input_type"));
  if (domain && domain->IsInDomain(this->GetInputProxy()))
    {

    vtkPVDataInformation* datainfo = this->GetInputProxy()->GetDataInformation();
    if (datainfo->GetNumberOfCells() < 1000000)
      {
      this->SupportsZSweepMapper = 1;
      }
    if (datainfo->GetNumberOfCells() < 500000)
      {
      this->SupportsBunykMapper = 1;
      }
    
    // HAVS support is determined when the representation is added to a view
    }
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetupVolumePipeline()
{

  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  if (this->SupportsHAVSMapper)
    {
    this->Connect(this->GetStrategy()->GetOutput(), this->VolumeHAVSMapper);
    pp->AddProxy(this->VolumeHAVSMapper);
    }
  else if(this->SupportsBunykMapper)
    {
    this->Connect(this->GetStrategy()->GetOutput(), this->VolumeBunykMapper);
    pp->AddProxy(this->VolumeBunykMapper);
    }
  else if(this->SupportsZSweepMapper)
    {
    this->Connect(this->GetStrategy()->GetOutput(), this->VolumeZSweepMapper);
    pp->AddProxy(this->VolumeZSweepMapper);
    }
  else
    {
    this->Connect(this->GetStrategy()->GetOutput(), this->VolumePTMapper);
    pp->AddProxy(this->VolumePTMapper);
    }
  this->VolumeActor->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::UpdateRenderViewExtensions(
  vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* rvp = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!rvp)
    {
    return;
    }
  vtkPVOpenGLExtensionsInformation* glinfo =
    rvp->GetOpenGLExtensionsInformation();
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
  this->RenderViewExtensionsTested = 1;
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetVolumeMapperToBunykCM()
{
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return;
    }
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActor.");
    }
  pp->SetProxy(0, this->VolumeBunykMapper);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetVolumeMapperToPTCM()
{
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return;
    }
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActor.");
    }
  pp->SetProxy(0, this->VolumePTMapper);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetVolumeMapperToHAVSCM()
{
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return;
    }
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActor.");
    }
  pp->SetProxy(0, this->VolumeHAVSMapper);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetVolumeMapperToZSweepCM()
{
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return;
    }
  if (pp->GetNumberOfProxies() != 1)
    {
    vtkErrorMacro("Expected one proxy in Mapper's VolumeActor.");
    }
  pp->SetProxy(0, this->VolumeZSweepMapper);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMUnstructuredGridVolumeRepresentationProxy::GetVolumeMapperTypeCM()
{ 
  vtkSMProxyProperty* pp;
  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeActor->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on VolumeActor.");
    return vtkSMUnstructuredGridVolumeRepresentationProxy::UNKNOWN_VOLUME_MAPPER;
    }
  
  vtkSMProxy *p = pp->GetProxy(0);
  
  if ( !p )
    {
    vtkErrorMacro("Failed to find proxy in Mapper proxy property!");
    return vtkSMUnstructuredGridVolumeRepresentationProxy::UNKNOWN_VOLUME_MAPPER;
    }
  
  if ( !strcmp(p->GetVTKClassName(), "vtkProjectedTetrahedraMapper" ) )
    {
    return vtkSMUnstructuredGridVolumeRepresentationProxy::PROJECTED_TETRA_VOLUME_MAPPER;
    }

  if ( !strcmp(p->GetVTKClassName(), "vtkHAVSVolumeMapper" ) )
    {
    return vtkSMUnstructuredGridVolumeRepresentationProxy::HAVS_VOLUME_MAPPER;
    }

  if ( !strcmp(p->GetVTKClassName(), "vtkUnstructuredGridVolumeZSweepMapper" ) )
    {
    return vtkSMUnstructuredGridVolumeRepresentationProxy::ZSWEEP_VOLUME_MAPPER;
    }
  
  if ( !strcmp(p->GetVTKClassName(), "vtkUnstructuredGridVolumeRayCastMapper" ) )
    {
    return vtkSMUnstructuredGridVolumeRepresentationProxy::BUNYK_RAY_CAST_VOLUME_MAPPER;
    }
  
  return vtkSMUnstructuredGridVolumeRepresentationProxy::UNKNOWN_VOLUME_MAPPER;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VolumeFilterProxy: " << this->VolumeFilter << endl;
  os << indent << "StrategyProxy: " << this->GetStrategy() << endl;
  os << indent << "VolumePropertyProxy: " << this->VolumeProperty << endl;
  os << indent << "VolumeActorProxy: " << this->VolumeActor << endl;
  os << indent << "SupportsHAVSMapper: " << this->SupportsHAVSMapper << endl;
  os << indent << "SupportsBunykMapper: " << this->SupportsBunykMapper << endl;
  os << indent << "SupportsZSweepMapper: " << this->SupportsZSweepMapper << endl;
  os << indent << "RenderViewExtensionsTested: " << this->RenderViewExtensionsTested << endl;
}


