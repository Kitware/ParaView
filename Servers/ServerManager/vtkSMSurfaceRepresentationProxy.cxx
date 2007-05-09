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

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMSurfaceRepresentationProxy);
vtkCxxRevisionMacro(vtkSMSurfaceRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMSurfaceRepresentationProxy::vtkSMSurfaceRepresentationProxy()
{
  this->GeometryFilter = 0;
  this->Mapper = 0;
  this->LODMapper = 0;
  this->Prop3D = 0;
  this->Property = 0;
}

//----------------------------------------------------------------------------
vtkSMSurfaceRepresentationProxy::~vtkSMSurfaceRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::AddToView(vtkSMViewProxy* view)
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

  renderView->AddPropToRenderer(this->Prop3D);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  renderView->RemovePropFromRenderer(this->Prop3D);
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a geometry filter, the data type fed into the strategy is
  // always polydata.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(
    view->NewStrategy(VTK_POLY_DATA, vtkSMRenderViewProxy::SURFACE));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  this->SetStrategy(strategy);

  strategy->SetEnableLOD(true);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMPipelineRepresentationProxy::AddToView()).

  strategy->SetInput(this->GeometryFilter);
  this->Connect(strategy->GetOutput(), this->Mapper, "Input");
  this->Connect(strategy->GetLODOutput(), this->LODMapper, "Input");
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceRepresentationProxy::BeginCreateVTKObjects(int numObjects)
{
  if (!this->Superclass::BeginCreateVTKObjects(numObjects))
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
bool vtkSMSurfaceRepresentationProxy::EndCreateVTKObjects(int numObjects)
{
  this->Connect(this->GetInputProxy(), this->GeometryFilter, "Input");
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");


  return this->Superclass::EndCreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMSurfaceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


