/*=========================================================================

  Program:   ParaView
  Module:    vtkSMOutlineRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMOutlineRepresentationProxy.h"

#include "vtkAbstractMapper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"

vtkStandardNewMacro(vtkSMOutlineRepresentationProxy);
vtkCxxRevisionMacro(vtkSMOutlineRepresentationProxy, "1.7");
//----------------------------------------------------------------------------
vtkSMOutlineRepresentationProxy::vtkSMOutlineRepresentationProxy()
{
  this->OutlineFilter = 0;
  this->Mapper = 0;
  this->Prop3D = 0;
  this->Property = 0;
}

//----------------------------------------------------------------------------
vtkSMOutlineRepresentationProxy::~vtkSMOutlineRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMOutlineRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a outline filter, the data type fed into the strategy is
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


  strategy->SetEnableLOD(false);

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMDataRepresentationProxy::AddToView()).

  this->Connect(this->OutlineFilter, strategy);
  this->Connect(strategy->GetOutput(), this->Mapper);
  this->AddStrategy(strategy);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  return this->Superclass::InitializeStrategy(view);
}

//----------------------------------------------------------------------------
bool vtkSMOutlineRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  // Set server flags correctly on all subproxies.
  this->OutlineFilter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("OutlineFilter"));
  this->Mapper = this->GetSubProxy("Mapper");
  this->Prop3D = this->GetSubProxy("Prop3D");
  this->Property = this->GetSubProxy("Property");

  this->OutlineFilter->SetServers(vtkProcessModule::DATA_SERVER);
  this->Mapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Prop3D->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Property->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMOutlineRepresentationProxy::EndCreateVTKObjects()
{
  this->Connect(this->GetInputProxy(), this->OutlineFilter, 
    "Input", this->OutputPort);
  this->Connect(this->Mapper, this->Prop3D, "Mapper");
  this->Connect(this->Property, this->Prop3D, "Property");

  this->LinkSelectionProp(this->Prop3D);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->OutlineFilter->GetProperty("UseOutline"));
  if (ivp)
    {
    ivp->SetElement(0, 1);
    this->OutlineFilter->UpdateProperty("UseOutline");
    }


  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMOutlineRepresentationProxy::SetColorArrayName(const char* name)
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
}

//----------------------------------------------------------------------------
void vtkSMOutlineRepresentationProxy::SetColorAttributeType(int type)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Mapper->GetProperty("ScalarMode"));
  switch (type)
    {
  case POINT_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_POINT_DATA); 
    break;

  case CELL_DATA:
    ivp->SetElement(0,  VTK_SCALAR_MODE_USE_CELL_DATA);
    break;

  case FIELD_DATA:
    ivp->SetElement(0,  VTK_SCALAR_MODE_USE_FIELD_DATA);
    break;

  default:
    ivp->SetElement(0,  VTK_SCALAR_MODE_DEFAULT);
    }
  this->Mapper->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMOutlineRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


