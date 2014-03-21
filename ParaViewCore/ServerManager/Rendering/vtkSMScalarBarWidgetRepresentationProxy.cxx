/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarWidgetRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScalarBarWidgetRepresentationProxy.h"

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"

#include <string>
#include <vector>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMScalarBarWidgetRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetRepresentationProxy::vtkSMScalarBarWidgetRepresentationProxy()
{
  this->ActorProxy = NULL;
}

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetRepresentationProxy::~vtkSMScalarBarWidgetRepresentationProxy()
{
  this->ActorProxy = NULL;
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->ActorProxy = this->GetSubProxy("Prop2DActor");
  if (!this->ActorProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2DActor.");
    return;
    }

  this->ActorProxy->SetLocation( vtkProcessModule::CLIENT |
                                 vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects();

  if (!this->RepresentationProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2D.");
    return;
    }

  vtkSMProxyProperty* tapp = vtkSMProxyProperty::SafeDownCast(
                      this->RepresentationProxy->GetProperty("ScalarBarActor"));
  if (!tapp)
    {
    vtkErrorMacro("Failed to find property ScalarBarActor on ScalarBarRepresentation proxy.");
    return;
    }
  if(!tapp->AddProxy(this->ActorProxy))
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  if (event == vtkCommand::InteractionEvent)
    {
    // BUG #5399. If the widget's position is beyond the viewport, fix it.
    vtkScalarBarRepresentation* repr = vtkScalarBarRepresentation::SafeDownCast(
      this->RepresentationProxy->GetClientSideObject());
    if (repr)
      {
      double position[2];
      position[0] = repr->GetPosition()[0];
      position[1] = repr->GetPosition()[1];
      if (position[0] < 0.0)
        {
        position[0] = 0.0;
        }
      if (position[0] > 0.97)
        {
        position[0] = 0.97;
        }
      if (position[1] < 0.0)
        {
        position[1] = 0.0;
        }
      if (position[1] > 0.97)
        {
        position[1] = 0.97;
        }
      repr->SetPosition(position);
      }
    }
  this->Superclass::ExecuteEvent(event);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::UpdateComponentTitle(
  vtkPVArrayInformation* arrayInfo)
{
  vtkSMProperty* compProp = this->GetProperty("ComponentTitle");
  if (compProp == NULL)
    {
    vtkErrorMacro("Failed to locate ComponentTitle property.");
    return false;
    }

  vtkSMProxy* lutProxy = vtkSMPropertyHelper(this, "LookupTable").GetAsProxy();

  std::string componentName;
  int component = -1;
  if (lutProxy && vtkSMPropertyHelper(lutProxy, "VectorMode").GetAsInt() != 0)
    {
    component = vtkSMPropertyHelper(lutProxy, "VectorComponent").GetAsInt();
    }

  if (arrayInfo == NULL || arrayInfo->GetNumberOfComponents() > 1)
    {
    const char* componentNameFromData = arrayInfo? arrayInfo->GetComponentName(component): NULL;
    if (componentNameFromData == NULL)
      {
      // just use the component number directly.
      if (component >= 0)
        {
        vtksys_ios::ostringstream cname;
        cname << component;
        componentName = cname.str();
        }
      else
        {
        componentName = "Magnitude";
        }
      }
    else
      {
      componentName = componentNameFromData;
      }
    }
  vtkSMPropertyHelper(compProp).Set(componentName.c_str());
  this->UpdateVTKObjects();
  return true;
}

namespace
{
  inline bool IsAvailable(
    double x, double y, const std::vector<vtkBoundingBox>& boxes)
    {
    for (std::vector<vtkBoundingBox>::const_iterator iter=boxes.begin();
      iter != boxes.end(); ++iter)
      {
      if (iter->ContainsPoint(x, y, 0.0) == 1)
        {
        return false;
        }
      }
    return true;
    }
};
//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(vtkSMProxy* view)
{
  if (!view)
    {
    return false;
    }

  // locate all other visible scalar bar in the view and determine their
  // positions.
  std::vector<vtkBoundingBox> occupiedBoxes;

  vtkSMPropertyHelper reprHelper(view, "Representations");
  for (unsigned int cc=0, max=reprHelper.GetNumberOfElements(); cc<max; ++cc)
    {
    vtkSMProxy* repr = reprHelper.GetAsProxy(cc);
    if (repr != this &&
      vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(repr) &&
      vtkSMPropertyHelper(repr, "Visibility").GetAsInt() == 1)
      {
      double pos1[2], pos2[2];
      vtkSMPropertyHelper(repr, "Position").Get(pos1, 2);
      vtkSMPropertyHelper(repr, "Position2").Get(pos2, 2);

      vtkBoundingBox bbox;
      bbox.AddPoint(pos1[0], pos1[1], 0.0);
      bbox.AddPoint(pos1[0] + pos2[0], pos1[1] + pos2[1], 0.0);
      occupiedBoxes.push_back(bbox);
      }
    }

  double mysize[2];
  vtkSMPropertyHelper pos2Helper(this, "Position2");
  pos2Helper.Get(mysize, 2);
  if (mysize[0] <= 0.0) { mysize[0] = 0.23; }
  if (mysize[1] <= 0.0) { mysize[1] = 0.13; }
  pos2Helper.Set(mysize, 2);

  // FIXME: to respect mysize. Right now I'm just ignoring it.
  // We try the 4 corners first starting with the lower-right corner.
  if (IsAvailable(0.75, 0.05, occupiedBoxes))
    {
    double pos[]= {0.75, 0.05};
    vtkSMPropertyHelper(this, "Position").Set(pos, 2);
    }
  else if (IsAvailable(0.75, 0.9, occupiedBoxes))
    {
    double pos[]= {0.75, 0.9};
    vtkSMPropertyHelper(this, "Position").Set(pos, 2);
    }
  else if (IsAvailable(0.02, 0.9, occupiedBoxes))
    {
    double pos[]= {0.02, 0.9};
    vtkSMPropertyHelper(this, "Position").Set(pos, 2);
    }
  else if (IsAvailable(0.02, 0.05, occupiedBoxes))
    {
    double pos[]= {0.02, 0.05};
    vtkSMPropertyHelper(this, "Position").Set(pos, 2);
    }
  // Next try the 4 centers.
  else if (IsAvailable(0.3, 0.05, occupiedBoxes))
    {
    double pos[]= {0.3, 0.05};
    vtkSMPropertyHelper(this, "Position").Set(pos, 2);
    }
  else if (IsAvailable(0.75, 0.4, occupiedBoxes))
    {
    double pos[]= {0.75, 0.4};
    vtkSMPropertyHelper(this, "Position").Set(pos, 2);
    }
  else if (IsAvailable(0.3, 0.9, occupiedBoxes))
    {
    double pos[]= {0.3, 0.9};
    vtkSMPropertyHelper(this, "Position").Set(pos, 2);
    }
  else if (IsAvailable(0.02, 0.4, occupiedBoxes))
    {
    double pos[]= {0.02, 0.4};
    vtkSMPropertyHelper(this, "Position").Set(pos, 2);
    }
  // otherwise just leave the positions unchanged.
  this->UpdateVTKObjects();
  return true;
}
