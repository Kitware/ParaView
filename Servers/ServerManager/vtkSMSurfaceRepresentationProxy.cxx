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
#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMSurfaceRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSurfaceRepresentationProxy, "1.11.2.2");
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

  this->SetSelectionSupported(true);
}

//----------------------------------------------------------------------------
vtkSMSurfaceRepresentationProxy::~vtkSMSurfaceRepresentationProxy()
{
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
  this->Connect(this->GetInputProxy(), this->GeometryFilter, "Input");
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");

  this->LinkSelectionProp(this->Prop3D);

  return this->Superclass::EndCreateVTKObjects();
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

  vtkClientServerID sourceId = this->GeometryFilter->GetID();
  vtkClientServerID originalSourceId;
  vtkSMProxy* input = this->GetInputProxy();
  if (vtkSMCompoundProxy* cp = vtkSMCompoundProxy::SafeDownCast(input))
    {
    // For compound proxies, the selected proxy is the consumed proxy.
    originalSourceId = cp->GetConsumableProxy()->GetID();
    }
  else
    {
    originalSourceId = input->GetID();
    }

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
    if (!properties->Has(vtkSelection::PROP_ID()))
      {
      continue;
      }
    vtkClientServerID propId;
    propId.ID = static_cast<vtkTypeUInt32>(properties->Get(
        vtkSelection::PROP_ID()));
    if (propId == this->Prop3D->GetID())
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

  // Convert surface selection to volume selection.
  vtkSmartPointer<vtkSelection> volSelection = 
    vtkSmartPointer<vtkSelection>::New();
  this->ConvertSurfaceSelectionToVolumeSelection(mySelection, volSelection);

  // Create a selection source for the selection.
  vtkSMProxy* selectionSource = 
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(
      this->ConnectionID, volSelection);
  
  return selectionSource;
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
  ivp->SetElement(0, repr);
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

  if (this->Representation != VTK_SURFACE)
    {
    diffuse = 0.0;
    ambient = 1.0;
    specular = 0.0;
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
}


