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

#include "vtkAbstractMapper.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp3D.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOpenGLExtensionsInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMRepresentationStrategyVector.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMUnstructuredGridVolumeRepresentationProxy);
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
  this->VolumeDummyMapper = 0;
  this->VolumeLODMapper = 0;

  this->SupportsBunykMapper  = 0;
  this->SupportsZSweepMapper = 0;
  this->SupportsHAVSMapper   = 0;
  this->RenderViewExtensionsTested = 0;

  vtkMemberFunctionCommand<vtkSMUnstructuredGridVolumeRepresentationProxy>* command =
    vtkMemberFunctionCommand<vtkSMUnstructuredGridVolumeRepresentationProxy>::New();
  command->SetCallback(*this,
    &vtkSMUnstructuredGridVolumeRepresentationProxy::ProcessViewInformation);
  this->ViewInformationObserver = command;
}

//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeRepresentationProxy::~vtkSMUnstructuredGridVolumeRepresentationProxy()
{
  this->SetViewInformation(0);
  this->ViewInformationObserver->Delete();

  this->VolumeFilter = 0;
  this->VolumePTMapper = 0;
  this->VolumeHAVSMapper = 0;
  this->VolumeBunykMapper = 0;
  this->VolumeZSweepMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;
  this->VolumeDummyMapper = 0;
  this->VolumeLODMapper = 0;

}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (!this->RenderViewExtensionsTested)
    {
    this->UpdateRenderViewExtensions(view);
    }

  this->DetermineVolumeSupport();
  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetViewInformation(
  vtkInformation* info)
{
  if (this->ViewInformation)
    {
    this->ViewInformation->RemoveObserver(this->ViewInformationObserver);
    }
  this->Superclass::SetViewInformation(info);
  if (this->ViewInformation)
    {
    this->ViewInformation->AddObserver(vtkCommand::ModifiedEvent,
      this->ViewInformationObserver);
    // Get the current values from the view helper.
    this->ProcessViewInformation();
    }
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::ProcessViewInformation()
{
  if (!this->ViewInformation || !this->ObjectsCreated)
    {
    return;
    }

  bool use_lod = false;
  if (this->ViewInformation && 
    this->ViewInformation->Has(vtkSMRenderViewProxy::USE_LOD()))
    {
    use_lod = this->ViewInformation->Get(vtkSMRenderViewProxy::USE_LOD()) > 0;
    }
  
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->VolumeActor->GetProperty("EnableLOD"));
  ivp->SetElement(0, use_lod? 1:0); 
  this->VolumeActor->UpdateProperty("EnableLOD");

  if (this->ViewInformation->Has(
      vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_RENDER())
    && this->ViewInformation->Get(
      vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_RENDER())==1 
    && use_lod == false)
    {
    // We must use LOD on client side.
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->VolumeActor->GetID()
            << "SetEnableLOD" << 1
            << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::CLIENT, stream);   
    }
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

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a geometry filter, the data type fed into the strategy is
  // always polydata.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(
    view->NewStrategy(VTK_UNSTRUCTURED_GRID));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  strategy->SetEnableLOD(true);

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMPipelineRepresentationProxy::AddToView()).
  this->Connect(this->VolumeFilter, strategy, "Input");
  strategy->UpdateVTKObjects();

  this->Connect(strategy->GetOutput(), this->VolumeHAVSMapper);
  this->Connect(strategy->GetOutput(), this->VolumeBunykMapper);
  this->Connect(strategy->GetOutput(), this->VolumeZSweepMapper);
  this->Connect(strategy->GetOutput(), this->VolumePTMapper);
  this->Connect(strategy->GetLODOutput(), this->VolumeLODMapper);

  this->AddStrategy(strategy);
  
  return this->Superclass::InitializeStrategy(view);
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
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
  this->VolumeActor = this->GetSubProxy("Prop3D");
  this->VolumeProperty = this->GetSubProxy("VolumeProperty");
  this->VolumeDummyMapper = this->GetSubProxy("VolumeDummyMapper");
  this->VolumeLODMapper = this->GetSubProxy("VolumeLODMapper");

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
  this->VolumeDummyMapper->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->VolumeLODMapper->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::EndCreateVTKObjects()
{
  this->Connect(this->GetInputProxy(), this->VolumeFilter, 
    "Input", this->OutputPort);
  /*
  this->Connect(this->VolumeBunykMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeHAVSMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeZSweepMapper, this->VolumeActor, "Mapper");
  */
  this->Connect(this->VolumePTMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeLODMapper, this->VolumeActor, "LODMapper");
  this->Connect(this->VolumeProperty, this->VolumeActor, "Property");


  this->ProcessViewInformation();
  return this->Superclass::EndCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::DetermineVolumeSupport()
{
  vtkSMDataTypeDomain* domain = vtkSMDataTypeDomain::SafeDownCast(
    this->VolumeFilter->GetProperty("Input")->GetDomain("input_type"));
  if (domain && domain->IsInDomain(this->GetInputProxy(), this->OutputPort))
    {
    vtkPVDataInformation* datainfo = 
      this->GetInputProxy()->GetDataInformation();
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
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetColorArrayName(
  const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->VolumeDummyMapper->GetProperty("SelectScalarArray"));
  vtkSMIntVectorProperty* lod_ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->VolumeLODMapper->GetProperty("ScalarVisibility"));
  vtkSMStringVectorProperty* lod_svp = vtkSMStringVectorProperty::SafeDownCast(
    this->VolumeLODMapper->GetProperty("ColorArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    lod_ivp->SetElement(0, 1);
    lod_svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    lod_ivp->SetElement(0, 0);
    lod_svp->SetElement(0, "");
    }

  this->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetColorAttributeType(
  int type)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->VolumeDummyMapper->GetProperty("ScalarMode"));
  vtkSMIntVectorProperty* lod_ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->VolumeLODMapper->GetProperty("ScalarMode"));

  switch (type)
    {
  case POINT_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA); 
    lod_ivp->SetElement(0, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA); 
    break;

  case CELL_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    lod_ivp->SetElement(0, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case FIELD_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_FIELD_DATA);
    lod_ivp->SetElement(0, VTK_SCALAR_MODE_USE_FIELD_DATA);
    break;

  default:
    ivp->SetElement(0,  VTK_SCALAR_MODE_DEFAULT);
    lod_ivp->SetElement(0,  VTK_SCALAR_MODE_DEFAULT);
    }

  this->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetLookupTable(vtkSMProxy* lut)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeProperty->GetProperty("ColorTransferFunction"));
  pp->RemoveAllProxies();
  pp->AddProxy(lut);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->VolumeLODMapper->GetProperty("LookupTable"));
  pp->RemoveAllProxies();
  pp->AddProxy(lut);

  this->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMUnstructuredGridVolumeRepresentationProxy::HasVisibleProp3D(
  vtkProp3D* prop)
{
  if(!prop)
  {
    return false;
  }

  if(this->Superclass::HasVisibleProp3D(prop))
  {
    return true;
  }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (this->GetVisibility() && 
    pm->GetIDFromObject(prop) == this->VolumeActor->GetID())
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMUnstructuredGridVolumeRepresentationProxy::ConvertSelection(
  vtkSelection* userSel)
{
  if (!this->GetVisibility())
    {
    return 0;
    }

  vtkSmartPointer<vtkSelection> mySelection = 
    vtkSmartPointer<vtkSelection>::New();

  unsigned int numNodes = userSel->GetNumberOfNodes();
  for (unsigned int cc=0; cc < numNodes; cc++)
    {
    vtkSelectionNode* node = userSel->GetNode(cc);
    vtkInformation* properties = node->GetProperties();
    // If there is no PROP_ID or PROP key set, we assume the selection
    // is valid on all representations
    bool hasProp = true;
    if (properties->Has(vtkSelectionNode::PROP_ID()))
      {
      hasProp = false;
      vtkClientServerID propId;

      propId.ID = static_cast<vtkTypeUInt32>(properties->Get(
        vtkSelectionNode::PROP_ID()));
      if (propId == this->VolumeActor->GetID())
        {
        hasProp = true;
        }
      }
    else if(properties->Has(vtkSelectionNode::PROP()))
      {
      hasProp = false;
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      if (properties->Get(vtkSelectionNode::PROP()) == 
        pm->GetObjectFromID(this->VolumeActor->GetID()))
        {
        hasProp = true;
        }
      }
    if(hasProp)
      {
      vtkSelectionNode* myNode = vtkSelectionNode::New();
      myNode->ShallowCopy(node);
      mySelection->AddNode(myNode);
      myNode->Delete();
      }
    }

  if (mySelection->GetNumberOfNodes() == 0)
    {
    return 0;
    }

  // Create a selection source for the selection.
  vtkSMProxy* selectionSource = 
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(
      this->ConnectionID, mySelection);
  
  return selectionSource;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetSelectedMapperIndex(int index)
{
  this->SelectedMapperIndex = index;
  switch (index)
    {
    case vtkSMUnstructuredGridVolumeRepresentationProxy::PROJECTED_TETRA_VOLUME_MAPPER:
      this->SetVolumeMapperToPTCM();
      break;
    case vtkSMUnstructuredGridVolumeRepresentationProxy::HAVS_VOLUME_MAPPER:
      this->SetVolumeMapperToHAVSCM();
      break;
    case vtkSMUnstructuredGridVolumeRepresentationProxy::ZSWEEP_VOLUME_MAPPER:
      this->SetVolumeMapperToZSweepCM();
      break;
    case vtkSMUnstructuredGridVolumeRepresentationProxy::BUNYK_RAY_CAST_VOLUME_MAPPER:
      this->SetVolumeMapperToBunykCM();
      break;
    default:
      vtkDebugMacro("Unknown volume mapper index " << index);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::SetSelectedMapperIndexIfSupported(int index)
{

  if (index == vtkSMUnstructuredGridVolumeRepresentationProxy::PROJECTED_TETRA_VOLUME_MAPPER)
    {
    this->SetVolumeMapperToPTCM();
    }
  else if (index == vtkSMUnstructuredGridVolumeRepresentationProxy::HAVS_VOLUME_MAPPER
          && this->GetSupportsHAVSMapper())
    {
    this->SetVolumeMapperToHAVSCM();
    }
  else if (index == vtkSMUnstructuredGridVolumeRepresentationProxy::ZSWEEP_VOLUME_MAPPER
           && this->GetSupportsZSweepMapper())
    {
    this->SetVolumeMapperToZSweepCM();
    }
  else if (index == vtkSMUnstructuredGridVolumeRepresentationProxy::BUNYK_RAY_CAST_VOLUME_MAPPER
           && this->GetSupportsBunykMapper())
    {
    this->SetVolumeMapperToBunykCM();
    }
  else
    {
    vtkDebugMacro("Requested volume mapper index " << index << " is not supported.");
    return;
    }

  // Only set the index if we were successful
  this->SelectedMapperIndex = index;
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VolumeFilter: " << this->VolumeFilter << endl;
  os << indent << "VolumePropertyProxy: " << this->VolumeProperty << endl;
  os << indent << "VolumeActorProxy: " << this->VolumeActor << endl;
  os << indent << "SupportsHAVSMapper: " << this->SupportsHAVSMapper << endl;
  os << indent << "SupportsBunykMapper: " << this->SupportsBunykMapper << endl;
  os << indent << "SupportsZSweepMapper: " << this->SupportsZSweepMapper << endl;
  os << indent << "RenderViewExtensionsTested: " << this->RenderViewExtensionsTested << endl;
  os << indent << "SelectedMapperIndex: " << this->SelectedMapperIndex << endl;
}


