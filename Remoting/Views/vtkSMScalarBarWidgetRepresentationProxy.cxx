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
#include "vtkPVArrayInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkTuple.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkSMScalarBarWidgetRepresentationProxy);

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetRepresentationProxy::vtkSMScalarBarWidgetRepresentationProxy()
{
  this->ActorProxy = nullptr;
  this->TraceItem = nullptr;
}

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetRepresentationProxy::~vtkSMScalarBarWidgetRepresentationProxy()
{
  this->ActorProxy = nullptr;
  delete this->TraceItem;
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

  this->ActorProxy->SetLocation(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects();

  if (!this->RepresentationProxy)
  {
    vtkErrorMacro("Failed to find subproxy Prop2D.");
    return;
  }

  vtkSMProxyProperty* tapp =
    vtkSMProxyProperty::SafeDownCast(this->RepresentationProxy->GetProperty("ScalarBarActor"));
  if (!tapp)
  {
    vtkErrorMacro("Failed to find property ScalarBarActor on ScalarBarRepresentation proxy.");
    return;
  }
  tapp->AddProxy(this->ActorProxy);

  // Initialize the scalar bar widget from the ScalarBarLength property.
  this->ScalarBarLengthToScalarBarWidgetPosition2();

  // Add observer on the ScalarBarLength property to convert its value to
  // Position2 of the widget.
  this->GetProperty("ScalarBarLength")
    ->AddObserver(vtkCommand::ModifiedEvent, this,
      &vtkSMScalarBarWidgetRepresentationProxy::ScalarBarLengthToScalarBarWidgetPosition2);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  if (event == vtkCommand::StartInteractionEvent)
  {
    this->BeginTrackingPropertiesForTrace();
  }
  else if (event == vtkCommand::InteractionEvent)
  {
    // BUG #5399. If the widget's position is beyond the viewport, fix it.
    vtkScalarBarRepresentation* repr =
      vtkScalarBarRepresentation::SafeDownCast(this->RepresentationProxy->GetClientSideObject());
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
    vtkSMPropertyHelper(this, "WindowLocation").Set(0);

    this->ScalarBarWidgetPosition2ToScalarBarLength();
  }

  this->Superclass::ExecuteEvent(event);

  if (event == vtkCommand::EndInteractionEvent)
  {
    this->EndTrackingPropertiesForTrace();
  }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::BeginTrackingPropertiesForTrace()
{
  assert(this->TraceItem == nullptr);
  this->TraceItem = new vtkSMTrace::TraceItem("ScalarBarInteraction");
  (*this->TraceItem) =
    vtkSMTrace::TraceItemArgs().arg("proxy", this).arg("comment", " change scalar bar placement");
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::EndTrackingPropertiesForTrace()
{
  delete this->TraceItem;
  this->TraceItem = nullptr;
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::UpdateComponentTitle(vtkPVArrayInformation* arrayInfo)
{
  vtkSMProperty* compProp = this->GetProperty("ComponentTitle");
  if (compProp == nullptr)
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

  if (arrayInfo == nullptr || arrayInfo->GetNumberOfComponents() > 1)
  {
    const char* componentNameFromData =
      arrayInfo ? arrayInfo->GetComponentName(component) : nullptr;
    if (componentNameFromData == nullptr)
    {
      // just use the component number directly.
      if (component >= 0)
      {
        std::ostringstream cname;
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
inline bool IsAvailable(const int location, const std::vector<int>& existingLocations)
{
  for (size_t i = 0; i < existingLocations.size(); ++i)
  {
    if (location == existingLocations[i])
    {
      return false;
    }
  }
  return true;
}
}

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::PlaceInView(vtkSMProxy* view)
{
  if (!view || vtkSMPropertyHelper(this, "LockPosition", /*quiet*/ true).GetAsInt() == 1)
  {
    return false;
  }

  // locate all *other* visible scalar bar in the view.
  std::vector<int> occupiedLocations;

  vtkSMPropertyHelper reprHelper(view, "Representations");
  for (unsigned int cc = 0, max = reprHelper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMProxy* repr = reprHelper.GetAsProxy(cc);
    if (repr != this && vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(repr) &&
      vtkSMPropertyHelper(repr, "Visibility", /*quiet*/ true).GetAsInt() == 1)
    {
      int location = vtkSMPropertyHelper(repr, "WindowLocation").GetAsInt();
      occupiedLocations.push_back(location);
    }
  }

  if (IsAvailable(vtkSMPropertyHelper(this, "WindowLocation").GetAsInt(), occupiedLocations))
  {
    // current position and if available, just return.
    this->UpdateVTKObjects();
    return true;
  }

  // Set up corner codes for the scalar bar representation
  int locations[] = { 0, 5, 4, 1 };

  // we skip the front since we already tested it.
  for (size_t cc = 1; cc < sizeof(locations) / sizeof(int); ++cc)
  {
    if (IsAvailable(locations[cc], occupiedLocations))
    {
      vtkSMPropertyHelper(this, "WindowLocation").Set(locations[cc]);
      break;
    }
  }

  // otherwise just leave the positions unchanged.
  this->UpdateVTKObjects();
  return true;
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::ScalarBarWidgetPosition2ToScalarBarLength()
{
  vtkScalarBarRepresentation* repr =
    vtkScalarBarRepresentation::SafeDownCast(this->RepresentationProxy->GetClientSideObject());
  if (!repr)
  {
    return;
  }

  int index = repr->GetOrientation() == VTK_ORIENT_HORIZONTAL ? 0 : 1;
  double length = repr->GetPosition2Coordinate()->GetValue()[index];
  vtkSMPropertyHelper(this, "ScalarBarLength").Set(length);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::ScalarBarLengthToScalarBarWidgetPosition2()
{
  // Set the scalar bar representation length from the ScalarBarlLength property
  vtkScalarBarRepresentation* repr =
    vtkScalarBarRepresentation::SafeDownCast(this->RepresentationProxy->GetClientSideObject());
  if (!repr)
  {
    return;
  }

  int index = repr->GetOrientation() == VTK_ORIENT_HORIZONTAL ? 0 : 1;
  double length = vtkSMPropertyHelper(this, "ScalarBarLength").GetAsDouble();
  double pos2[3];
  repr->GetPosition2Coordinate()->GetValue(pos2);
  pos2[index] = length;
  repr->SetPosition2(pos2);
}
