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
  tapp->AddProxy(this->ActorProxy);
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
    const vtkBoundingBox& bbox, const std::vector<vtkBoundingBox>& boxes)
    {
    for (std::vector<vtkBoundingBox>::const_iterator iter=boxes.begin();
      iter != boxes.end(); ++iter)
      {
      if (iter->Intersects(bbox) == 1)
        {
        return false;
        }
      }
    return true;
    }

  inline vtkBoundingBox GetBox(
    const double anchor[2], const double position[2])
    {
    return vtkBoundingBox(
      anchor[0], anchor[0] + position[0],
      anchor[1], anchor[1] + position[1],
      0, 0);
    }
};

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(vtkSMProxy* view)
{
  if (!view)
    {
    return false;
    }

  // locate all *other* visible scalar bar in the view and determine their
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
  // if size is invalid, fix it.
  if (mysize[0] <= 0.0) { mysize[0] = 0.23; }
  if (mysize[1] <= 0.0) { mysize[1] = 0.13; }
  pos2Helper.Set(mysize, 2);

  double myanchor[2];
  vtkSMPropertyHelper posHelper(this, "Position");
  posHelper.Get(myanchor, 2);

  // if this scalar bar's current position is available, we don't bother moving
  // it at all.
  if (IsAvailable(GetBox(myanchor, mysize), occupiedBoxes))
    {
    // scalar bar doesn't need to be moved at all.
    this->UpdateVTKObjects();
    return true;
    }

  // FIXME: to respect mysize. Right now I'm just ignoring it.
  // We try the 4 corners first starting with the lower-right corner.
  const double anchorPositions[8][2] =
    {
      // four corners first, starting with lower right.
      {0.75, 0.05},
      {0.75, 0.90},
      {0.02, 0.90},
      {0.02, 0.05},
      // 4 centers next.
      {0.30, 0.05},
      {0.75, 0.40},
      {0.30, 0.90},
      {0.02, 0.40}
    };

  for (int kk=0; kk < 8; ++kk)
    {
    vtkBoundingBox mybox = GetBox(anchorPositions[kk], mysize);
    if (IsAvailable(mybox, occupiedBoxes))
      {
      posHelper.Set(anchorPositions[kk], 2);
      break;
      }
    }

  // otherwise just leave the positions unchanged.
  this->UpdateVTKObjects();
  return true;
}
