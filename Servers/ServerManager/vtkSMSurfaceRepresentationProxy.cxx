/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSurfaceRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSurfaceRepresentationProxy.h"

#include "vtkAbstractMapper.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp3D.h"
#include "vtkProperty.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMMaterialLoaderProxy.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMSurfaceRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSurfaceRepresentationProxy, "1.24");
//----------------------------------------------------------------------------
vtkSMSurfaceRepresentationProxy::vtkSMSurfaceRepresentationProxy()
{
  this->GeometryFilter = 0;
  this->Mapper = 0;
  this->LODMapper = 0;
  this->Prop3D = 0;
  this->Property = 0;
  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.1;
  this->Representation = VTK_SURFACE;

  vtkMemberFunctionCommand<vtkSMSurfaceRepresentationProxy>* command =
    vtkMemberFunctionCommand<vtkSMSurfaceRepresentationProxy>::New();
  command->SetCallback(*this,
    &vtkSMSurfaceRepresentationProxy::ProcessViewInformation);
  this->ViewInformationObserver = command;
}

//----------------------------------------------------------------------------
vtkSMSurfaceRepresentationProxy::~vtkSMSurfaceRepresentationProxy()
{
  this->SetViewInformation(0);
  this->ViewInformationObserver->Delete();
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::GetOrderedCompositingNeeded()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Opacity"));
  return (dvp && dvp->GetElement(0) < 1.0);
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a geometry filter, the data type fed into the strategy is
  // always polydata.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(
    view->NewStrategy(VTK_POLY_DATA));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMDataRepresentationProxy::AddToView()).

  strategy->SetEnableLOD(true);

  this->Connect(this->GeometryFilter, strategy);
  this->Connect(strategy->GetOutput(), this->Mapper);
  this->Connect(strategy->GetLODOutput(), this->LODMapper);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  this->AddStrategy(strategy);

  return this->Superclass::InitializeStrategy(view);
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  // Set server flags correctly on all subproxies.
  this->GeometryFilter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("GeometryFilter"));
  this->Mapper = this->GetSubProxy("Mapper");
  this->LODMapper = this->GetSubProxy("LODMapper");
  this->Prop3D = this->GetSubProxy("Prop3D");
  this->Property = this->GetSubProxy("Property");

  this->GeometryFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->Mapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->LODMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Prop3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::EndCreateVTKObjects()
{
  this->Connect(this->GetInputProxy(), this->GeometryFilter, 
    "Input", this->OutputPort);
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");

  vtkSMMaterialLoaderProxy* mlp = vtkSMMaterialLoaderProxy::SafeDownCast(
    this->GetSubProxy("MaterialLoader"));
  if (mlp)
    {
    mlp->SetPropertyProxy(this->Property);
    }

  this->LinkSelectionProp(this->Prop3D);

  this->ProcessViewInformation();
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::SetViewInformation(vtkInformation* info)
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
void vtkSMSurfaceRepresentationProxy::ProcessViewInformation()
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
    this->Prop3D->GetProperty("EnableLOD"));
  ivp->SetElement(0, use_lod? 1:0); 
  this->Prop3D->UpdateProperty("EnableLOD");

  if (this->ViewInformation->Has(
      vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_RENDER())
    && this->ViewInformation->Get(
      vtkSMIceTMultiDisplayRenderViewProxy::CLIENT_RENDER())==1 
    && use_lod == false)
    {
    // We must use LOD on client side.
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->Prop3D->GetID()
            << "SetEnableLOD" << 1
            << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::CLIENT, stream);   
    }
}

//----------------------------------------------------------------------------
static void vtkSMSurfaceRepresentationProxyAddSourceIDs(
  vtkSelection* sel, vtkClientServerID propId, vtkClientServerID sourceId,
  vtkClientServerID originalSourceId)
{
  unsigned int numChildren = sel->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; cc++)
    {
    vtkSMSurfaceRepresentationProxyAddSourceIDs(sel->GetChild(cc),
      propId, sourceId, originalSourceId);
    }

  vtkInformation* properties = sel->GetProperties();
  if (!properties->Has(vtkSelection::PROP_ID()) || 
    propId.ID != static_cast<vtkTypeUInt32>(
      properties->Get(vtkSelection::PROP_ID())))
    {
    return;
    }
  properties->Set(vtkSelection::SOURCE_ID(), sourceId.ID);
  properties->Set(vtkSelectionSerializer::ORIGINAL_SOURCE_ID(), 
    originalSourceId.ID);
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::ConvertSurfaceSelectionToVolumeSelection(
  vtkSelection* selInput, vtkSelection* selOutput)
{
  // Process selInput to add SOURCE_ID() and ORIGINAL_SOURCE_ID() keys to it to
  // help the converter in converting the selection.

  // Don't directly use the input->GetID() to handle case for
  // vtkSMCompoundSourceProxy.
  vtkClientServerID sourceId = this->GeometryFilter->GetID();
  vtkSMSourceProxy* input = this->GetInputProxy();
  vtkSMOutputPort* port = input->GetOutputPort(this->OutputPort);
  vtkClientServerID originalSourceId = port->GetProducerID();

  vtkSMSurfaceRepresentationProxyAddSourceIDs(selInput,
    this->Prop3D->GetID(), sourceId, originalSourceId);
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
    this->ConnectionID, selInput, selOutput);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMSurfaceRepresentationProxy::ConvertSelection(
  vtkSelection* surfaceSel)
{
  if (!this->GetVisibility())
    {
    return 0;
    }

  vtkSmartPointer<vtkSelection> mySelection = 
    vtkSmartPointer<vtkSelection>::New();
  mySelection->GetProperties()->Copy(surfaceSel->GetProperties(), 0);

  unsigned int numChildren = surfaceSel->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; cc++)
    {
    vtkSelection* child = surfaceSel->GetChild(cc);
    vtkInformation* properties = child->GetProperties();
    // If there is no PROP_ID or PROP key set, we assume the selection
    // is valid on all representations
    bool hasProp = true;
    if (properties->Has(vtkSelection::PROP_ID()))
      {
      hasProp = false;
      vtkClientServerID propId;

      propId.ID = static_cast<vtkTypeUInt32>(properties->Get(
        vtkSelection::PROP_ID()));
      if (propId == this->Prop3D->GetID())
        {
        hasProp = true;
        }
      }
    else if(properties->Has(vtkSelection::PROP()))
      {
      hasProp = false;
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      if (properties->Get(vtkSelection::PROP()) == 
        pm->GetObjectFromID(this->Prop3D->GetID()))
        {
        hasProp = true;
        }
      }
    if(hasProp)
      {
      vtkSelection* myChild = vtkSelection::New();
      myChild->ShallowCopy(child);
      mySelection->AddChild(myChild);
      myChild->Delete();
      }
    }

  if (mySelection->GetNumberOfChildren() == 0)
    {
    return 0;
    }

  vtkSMProxy* selectionSource = NULL;
  if(mySelection->GetChild(0)->GetContentType() == vtkSelection::FRUSTUM)
    {
    // Create a selection source for the selection.
    selectionSource = 
      vtkSMSelectionHelper::NewSelectionSourceFromSelection(
        this->ConnectionID, mySelection);
    }
  else
    {
    // Convert surface selection to volume selection.
    vtkSmartPointer<vtkSelection> volSelection = vtkSmartPointer<vtkSelection>::New();
    this->ConvertSurfaceSelectionToVolumeSelection(mySelection, volSelection);

    // Create a selection source for the selection.
    selectionSource = 
      vtkSMSelectionHelper::NewSelectionSourceFromSelection(
        this->ConnectionID, volSelection);
    }

  return selectionSource;
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::HasVisibleProp3D(vtkProp3D* prop)
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
    pm->GetIDFromObject(prop) == this->Prop3D->GetID())
    {
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::SetColorArrayName(const char* name)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ScalarVisibility"));
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ColorArray"));

  if (name && name[0])
    {
    ivp->SetElement(0, 1);
    svp->SetElement(0, name);
    }
  else
    {
    ivp->SetElement(0, 0);
    svp->SetElement(0, "");
    }

  this->Mapper->UpdateVTKObjects();
  this->LODMapper->UpdateVTKObjects();

  // Update specularity.
  this->UpdateShadingParameters();
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::SetColorAttributeType(int type)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ScalarMode"));
  switch (type)
    {
  case POINT_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA); 
    break;

  case CELL_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case FIELD_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_FIELD_DATA);
    break;

  default:
    ivp->SetElement(0,  VTK_SCALAR_MODE_DEFAULT);
    }
  this->Mapper->UpdateVTKObjects();
  this->LODMapper->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::SetRepresentation(int repr)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Property->GetProperty("Representation"));
  vtkSMIntVectorProperty* edgeVisibility = vtkSMIntVectorProperty::SafeDownCast(
    this->Property->GetProperty("EdgeVisibility"));
  if (repr == VTK_SURFACE_WITH_EDGES)
    {
    ivp->SetElement(0, VTK_SURFACE);
    edgeVisibility->SetElement(0, 1);
    }
  else
    {
    ivp->SetElement(0, repr);
    edgeVisibility->SetElement(0, 0);
    }
  this->Property->UpdateVTKObjects();

  this->Representation = repr;

  // Change shading off for that wireframe/points.
  this->UpdateShadingParameters();
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::UpdateShadingParameters()
{
  double diffuse = this->Diffuse;
  double specular = this->Specular;
  double ambient = this->Ambient;

  if (this->Representation != VTK_SURFACE && 
    this->Representation != VTK_SURFACE_WITH_EDGES)
    {
    diffuse = 0.0;
    ambient = 1.0;
    specular = 0.0;
    }
  else
    {
    // Disable specular highlighting is coloring by scalars.
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->Mapper->GetProperty("ScalarVisibility"));
    if (ivp->GetElement(0))
      {
      specular = 0.0;
      }
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Property->GetProperty("Ambient"));
  dvp->SetElement(0, ambient);
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Property->GetProperty("Diffuse"));
  dvp->SetElement(0, diffuse);
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Property->GetProperty("Specular"));
  dvp->SetElement(0, specular);
  this->Property->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Prop3D: " << this->Prop3D << endl;
}


