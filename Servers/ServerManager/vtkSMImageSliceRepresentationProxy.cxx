/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImageSliceRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMImageSliceRepresentationProxy.h"

#include "vtkAbstractMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"

vtkStandardNewMacro(vtkSMImageSliceRepresentationProxy);
vtkCxxRevisionMacro(vtkSMImageSliceRepresentationProxy, "1.2");
//-----------------------------------------------------------------------------
vtkSMImageSliceRepresentationProxy::vtkSMImageSliceRepresentationProxy()
{
  this->Slicer = 0;
  this->Mapper = 0;
  this->LODMapper = 0;
  this->Prop3D = 0;
  this->Property = 0;
}

//-----------------------------------------------------------------------------
vtkSMImageSliceRepresentationProxy::~vtkSMImageSliceRepresentationProxy()
{
}

//-----------------------------------------------------------------------------
bool vtkSMImageSliceRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  // Setup pointers to subproxies  for easy access and set server flags. 
  this->Slicer = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Slicer"));
  this->Mapper = this->GetSubProxy("Mapper");
  this->LODMapper = this->GetSubProxy("LODMapper");
  this->Prop3D = this->GetSubProxy("Prop3D");
  this->Property = this->GetSubProxy("Property");

  this->Slicer->SetServers(
    vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER);
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


//-----------------------------------------------------------------------------
bool vtkSMImageSliceRepresentationProxy::EndCreateVTKObjects()
{
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
//  this->Connect(this->LODMapper, this->Prop3D, "LODMapper");
  this->Connect(this->Property, this->Prop3D, "Property");

  return this->Superclass::EndCreateVTKObjects();
}


//-----------------------------------------------------------------------------
bool vtkSMImageSliceRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // We know the input data type: it has to be a vtkImageData. Hence we can
  // simply ask the view for the correct strategy.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(view->NewStrategy(VTK_IMAGE_DATA));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use."
      "Cannot be rendered in this view of type: " << view->GetClassName());
    return false;
    }

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMDataRepresentationProxy::AddToView()).

  // TODO: For now, I am not going to worry about the LOD pipeline.
  strategy->SetEnableLOD(false);

  this->Connect(this->GetInputProxy(), this->Slicer,
    "Input", this->OutputPort);

  this->Connect(this->Slicer, strategy);
  this->Connect(strategy->GetOutput(), this->Mapper);
  // this->Connect(strategy->GetLODOutput(), this->LODMapper);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  this->AddStrategy(strategy);

  return this->Superclass::InitializeStrategy(view);
}

//-----------------------------------------------------------------------------
void vtkSMImageSliceRepresentationProxy::SetColorAttributeType(int type)
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

  default:
    vtkWarningMacro("Incorrect Color attribute type.");
    ivp->SetElement(0,  VTK_SCALAR_MODE_DEFAULT);
    }
  this->Mapper->UpdateVTKObjects();
  //this->LODMapper->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMImageSliceRepresentationProxy::SetColorArrayName(const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ColorArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }

  this->Mapper->UpdateVTKObjects();
  //this->LODMapper->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMImageSliceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
