// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMScalarBarWidgetRepresentationProxy.h"

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkTuple.h"

#include <algorithm>
#include <limits>
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
    auto repr =
      vtkScalarBarRepresentation::SafeDownCast(this->RepresentationProxy->GetClientSideObject());
    if (repr)
    {
      double position[2];
      position[0] = repr->GetPosition()[0];
      position[1] = repr->GetPosition()[1];
      position[0] = std::max(position[0], 0.0);
      position[0] = std::min(position[0], 0.97);
      position[1] = std::max(position[1], 0.0);
      position[1] = std::min(position[1], 0.97);
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

  const vtkSMPropertyHelper reprHelper(view, "Representations");
  for (unsigned int cc = 0, max = reprHelper.GetNumberOfElements(); cc < max; ++cc)
  {
    vtkSMProxy* repr = reprHelper.GetAsProxy(cc);
    if (repr != this && vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(repr) &&
      vtkSMPropertyHelper(repr, "Visibility", /*quiet*/ true).GetAsInt() == 1)
    {
      const int location = vtkSMPropertyHelper(repr, "WindowLocation").GetAsInt();
      occupiedLocations.push_back(location);
    }
  }

  // A scalar bar with ::AnyLocation implies that it is free to move around, hence do not consider
  // it an 'occupied' location
  auto locationTypeIsAny = [](const int& loc)
  { return loc == static_cast<int>(vtkScalarBarRepresentation::AnyLocation); };
  occupiedLocations.erase(
    std::remove_if(occupiedLocations.begin(), occupiedLocations.end(), locationTypeIsAny),
    occupiedLocations.end());

  if (IsAvailable(vtkSMPropertyHelper(this, "WindowLocation").GetAsInt(), occupiedLocations))
  {
    // current position and if available, just return.
    this->UpdateVTKObjects();
    return true;
  }

  // Set up corner codes for the scalar bar representation
  const int locations[] = { 0, 5, 4, 1 };

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

  const int index = repr->GetOrientation() == VTK_ORIENT_HORIZONTAL ? 0 : 1;
  const double length = repr->GetPosition2Coordinate()->GetValue()[index];
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

  const int index = repr->GetOrientation() == VTK_ORIENT_HORIZONTAL ? 0 : 1;
  const double length = vtkSMPropertyHelper(this, "ScalarBarLength").GetAsDouble();
  double pos2[3];
  repr->GetPosition2Coordinate()->GetValue(pos2);
  pos2[index] = length;
  repr->SetPosition2(pos2);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::AddRange(vtkSMRepresentationProxy* proxy)
{
  if (vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy))
  {
    this->Proxies.emplace(proxy, proxy);
  }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::AddBlockRange(
  vtkSMRepresentationProxy* proxy, const std::string& blockSelector)
{
  if (vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(proxy, blockSelector))
  {
    this->BlockProxies.emplace(std::make_pair(proxy, blockSelector), proxy);
  }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::RemoveRange(vtkSMRepresentationProxy* proxy)
{
  try
  {
    this->Proxies.erase(proxy);
  }
  catch (std::exception& e)
  {
    vtkErrorMacro(<< "Proxy " << proxy->GetXMLName() << " was probably not registered.");
    vtkErrorMacro(<< e.what());
  }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::RemoveBlockRange(
  vtkSMRepresentationProxy* proxy, const std::string& blockSelector)
{
  try
  {
    this->BlockProxies.erase(std::make_pair(proxy, blockSelector));
  }
  catch (std::exception& e)
  {
    vtkErrorMacro(<< "Proxy " << proxy->GetXMLName() << " was probably not registered.");
    vtkErrorMacro(<< e.what());
  }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::GetRange(double range[2], int component)
{
  bool valid = false;
  range[0] = std::numeric_limits<double>::infinity();
  range[1] = -range[0];
  if (!this->Proxies.empty())
  {
    double min = std::numeric_limits<double>::infinity();
    double max = -min;

    std::vector<vtkSMRepresentationProxy*> trash;

    for (const auto& pair : this->Proxies)
    {
      vtkSMRepresentationProxy* proxy = pair.second;
      if (!proxy)
      {
        // The weak pointer is nullptr, we need to delete this proxy.
        trash.emplace_back(pair.first);
        continue;
      }
      vtkPVArrayInformation* info =
        vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(proxy);
      if (!info)
      {
        continue;
      }
      valid = true;
      const double* localRange = info->GetComponentRange(component);
      min = std::min(min, localRange[0]);
      max = std::max(max, localRange[1]);
    }

    range[0] = std::min(range[0], min);
    range[1] = std::max(range[1], max);

    for (auto proxy : trash)
    {
      this->Proxies.erase(proxy);
    }
  }
  if (!this->BlockProxies.empty())
  {
    double min = std::numeric_limits<double>::infinity();
    double max = -min;

    std::vector<std::pair<vtkSMRepresentationProxy*, std::string>> trash;

    for (const auto& pair : this->BlockProxies)
    {
      vtkSMRepresentationProxy* proxy = pair.second;
      if (!proxy)
      {
        // The weak pointer is nullptr, we need to delete this proxy.
        trash.emplace_back(pair.first);
        continue;
      }
      vtkPVArrayInformation* info =
        vtkSMColorMapEditorHelper::GetBlockArrayInformationForColorArray(proxy, pair.first.second);
      if (!info)
      {
        continue;
      }
      valid = true;
      const double* localRange = info->GetComponentRange(component);
      min = std::min(min, localRange[0]);
      max = std::max(max, localRange[1]);
    }

    range[0] = std::min(range[0], min);
    range[1] = std::max(range[1], max);

    for (auto& pair : trash)
    {
      this->BlockProxies.erase(pair);
    }
  }

  if (!valid)
  {
    range[0] = std::numeric_limits<double>::quiet_NaN();
    range[1] = std::numeric_limits<double>::quiet_NaN();
  }
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::ClearRange()
{
  this->Proxies.clear();
  this->BlockProxies.clear();
}
