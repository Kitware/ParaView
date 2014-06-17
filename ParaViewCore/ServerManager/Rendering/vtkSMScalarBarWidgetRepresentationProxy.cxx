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
#include "vtkTuple.h"

#include <algorithm>
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
    // user interacted. lock the position.
    vtkSMPropertyHelper(this, "LockPosition").Set(1);
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

  inline double clamp(double val, double min, double max)
    {
    val = std::max(val, min);
    val = std::min(val, max);
    return val;
    }

  inline vtkBoundingBox GetBox(
    const vtkTuple<double, 2>& anchor,
    const vtkTuple<double, 2>& size)
    {
    return vtkBoundingBox(
      clamp(anchor[0], 0, 1), clamp(anchor[0] + size[0], 0, 1),
      clamp(anchor[1], 0, 1), clamp(anchor[1] + size[1], 0, 1),
      0, 0);
    }
};

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(vtkSMProxy* view)
{
  if (!view || vtkSMPropertyHelper(this, "LockPosition", /*quiet*/true).GetAsInt() == 1)
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
      vtkSMPropertyHelper(repr, "Visibility", /*quiet*/true).GetAsInt() == 1)
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

  bool isVertical = (vtkSMPropertyHelper(this, "Orientation").GetAsInt() == 1);
  double aspect = vtkSMPropertyHelper(this, "AspectRatio").GetAsDouble();
  aspect = aspect? aspect : 20;

  vtkTuple<double,2> mysize;
  vtkSMPropertyHelper pos2Helper(this, "Position2");
  pos2Helper.Get(mysize.GetData(), 2);
  // if size is invalid, fix it.
  if (mysize[0] <= 0.0) { mysize[0] = 0.23; }
  if (mysize[1] <= 0.0) { mysize[1] = 0.13; }
  pos2Helper.Set(mysize.GetData(), 2);

  vtkTuple<double,2> myanchor;
  vtkSMPropertyHelper posHelper(this, "Position");
  posHelper.Get(myanchor.GetData(), 2);

  vtkBoundingBox mybox = GetBox(myanchor, mysize);
  // let call mybox as (x1,y1):(x2,y2)
  if (IsAvailable(mybox, occupiedBoxes))
    {
    // current position and if available, just return.
    this->UpdateVTKObjects();
    return true;
    }

  std::vector<vtkBoundingBox> regions;
  regions.push_back(mybox); // although we've tested mybox, we add
  // it to the regions since it makes it easier to build the regions list.

  // flip mybox along X axis i.e. (x1, 1-y2):(x2, 1-y1)
  regions.push_back(vtkBoundingBox(
      // xmin, xmax
      mybox.GetMinPoint()[0], mybox.GetMaxPoint()[0],
      // ymin, ymax
      1.0 - mybox.GetMaxPoint()[1], 1.0 - mybox.GetMinPoint()[1],
      // zmin, zmax
      0,  0));

  // flip both mybox and the flipped-X box along Y axis.
  // i.e. (1-x2, y1):(1-x1: y2) and (1-x2, 1-y2):(1-x1, 1-y1)
  for (int cc=1; cc >=0; cc--)
    {
    const vtkBoundingBox& bbox = regions[cc];
    double maxX = 1.0 - bbox.GetMinPoint()[0];
    double minX = 1.0 - bbox.GetMaxPoint()[0];

    if (isVertical)
      {
      // Vertical scalar bars like to stick on the left edge of the bounding box.
      // That results in the scalar bar getting too close the egde on flipping
      // along Y axis. So we adjust it by offsetting the X position.
      double barWidth = bbox.GetLength(1)/aspect;
      double deltaX = maxX - minX;
      minX = clamp(maxX - barWidth, 0, 1);
      maxX = clamp(minX + deltaX, 0, 1);
      }
    vtkBoundingBox flippedYBox(
        // xmin, xmax
        //1.0 - bbox.GetMaxPoint()[0], 1.0 - bbox.GetMinPoint()[0],
        minX, maxX,
        // ymin, ymax
        bbox.GetMinPoint()[1], bbox.GetMaxPoint()[1],
        // zmin, zmax
        0, 0);
    regions.push_back(flippedYBox);
    }

  // we skip the front since we already tested it.
  for (size_t cc=1; cc < regions.size(); ++cc)
    {
    const vtkBoundingBox &curbox = regions[cc];
    if (IsAvailable(curbox, occupiedBoxes))
      {
      posHelper.Set(curbox.GetMinPoint(), 2);
      double lengths[3];
      curbox.GetLengths(lengths);
      pos2Helper.Set(lengths, 2);
      break;
      }
    }

  // otherwise just leave the positions unchanged.
  this->UpdateVTKObjects();
  return true;
}
