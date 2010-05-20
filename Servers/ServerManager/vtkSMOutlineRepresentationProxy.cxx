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
#include "vtkBoundingBox.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp3D.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkSMOutlineRepresentationProxy);
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

  vtkSMPropertyHelper(this->Property, "Ambient").Set(1);
  vtkSMPropertyHelper(this->Property, "Diffuse").Set(0);
  vtkSMPropertyHelper(this->Property, "Specular").Set(0);
  this->Property->UpdateVTKObjects();

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
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA); 
    break;

  case CELL_DATA:
    ivp->SetElement(0,  VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
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
bool vtkSMOutlineRepresentationProxy::HasVisibleProp3D(vtkProp3D* prop)
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
vtkSMProxy* vtkSMOutlineRepresentationProxy::ConvertSelection(
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
      }
    else if(properties->Has(vtkSelectionNode::PROP()))
      {
      hasProp = false;
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      if (properties->Get(vtkSelectionNode::PROP()) == 
        pm->GetObjectFromID(this->Prop3D->GetID()))
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

//-----------------------------------------------------------------------------
bool vtkSMOutlineRepresentationProxy::GetBounds(double bounds[6])
{
  if ( this->Superclass::GetBounds(bounds) == false )
    {
    return false;
    }

  // translation / rotation / scaling
  vtkSMDoubleVectorProperty *posProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Prop3D->GetProperty("Position") );
  vtkSMDoubleVectorProperty *rotProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Prop3D->GetProperty("Orientation") );
  vtkSMDoubleVectorProperty *sclProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->Prop3D->GetProperty("Scale") );

  double *position = posProp->GetElements();
  double *rotation = rotProp->GetElements();
  double *scale = sclProp->GetElements();

  if (scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0 ||
    position[0] != 0.0 || position[1] != 0.0 || position[2] != 0.0 ||
    rotation[0] != 0.0 || rotation[1] != 0.0 || rotation[2] != 0.0)
    {
    vtkSmartPointer<vtkTransform> transform = 
      vtkSmartPointer<vtkTransform>::New();
    transform->Translate(position);
    transform->RotateZ(rotation[2]);
    transform->RotateX(rotation[0]);
    transform->RotateY(rotation[1]);
    transform->Scale(scale);

    int i, j, k;
    double origX[3], x[3];
    vtkBoundingBox bbox;
    for (i = 0; i < 2; i++)
      {
      origX[0] = bounds[i];
      for (j = 0; j < 2; j++)
        {
        origX[1] = bounds[2 + j];
        for (k = 0; k < 2; k++)
          {
          origX[2] = bounds[4 + k];
          transform->TransformPoint(origX, x);
          bbox.AddPoint(x);
          }
        }
      }
    bbox.GetBounds(bounds);
    }
  
  return true;
}

//----------------------------------------------------------------------------
void vtkSMOutlineRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


