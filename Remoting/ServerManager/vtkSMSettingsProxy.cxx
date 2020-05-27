/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSettingsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSettingsProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVSession.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <string>

//----------------------------------------------------------------------------
class vtkSMSettingsProxy::vtkInternals
{
public:
  struct Item
  {
    std::string SourceName;
    std::string TargetName;
    vtkWeakPointer<vtkSMProperty> Source;
    vtkWeakPointer<vtkSMProxy> TargetProxy;
    vtkWeakPointer<vtkSMProperty> Target;
    unsigned long ObserverId = 0;
  };

  Item* FindItem(vtkSMProperty* source, vtkSMProperty* target)
  {
    auto iter =
      std::find_if(this->Links.begin(), this->Links.end(), [source, target](const Item& item) {
        return (item.Source == source && item.Target == target) ? true : false;
      });
    return (iter != this->Links.end()) ? &(*iter) : nullptr;
  }

  void Prune()
  {
    this->Links.erase(std::remove_if(this->Links.begin(), this->Links.end(),
                        [](const vtkInternals::Item& item) { return (item.Target == nullptr); }),
      this->Links.end());
  }

  std::vector<Item> Links;
  bool SourcePropertyIsBeingModified = false;
};

vtkStandardNewMacro(vtkSMSettingsProxy);
//----------------------------------------------------------------------------
vtkSMSettingsProxy::vtkSMSettingsProxy()
  : Internals(new vtkSMSettingsProxy::vtkInternals())
{
  this->SetLocation(vtkPVSession::CLIENT);
}

//----------------------------------------------------------------------------
vtkSMSettingsProxy::~vtkSMSettingsProxy()
{
  if (this->ObjectsCreated && this->VTKObjectObserverId)
  {
    if (auto object = vtkObject::SafeDownCast(this->GetClientSideObject()))
    {
      object->RemoveObserver(this->VTKObjectObserverId);
      this->VTKObjectObserverId = 0;
    }
  }

  for (auto& item : this->Internals->Links)
  {
    if (item.Target && item.ObserverId > 0)
    {
      item.Target->RemoveObserver(item.ObserverId);
    }
  }

  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
int vtkSMSettingsProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
  {
    return 0;
  }

  int serializable = 0;
  if (element->GetScalarAttribute("serializable", &serializable) && serializable == 1)
  {
    this->SetIsSerializable(true);
  }

  // Now link information properties that provide the current value of the VTK
  // object with the corresponding proxy property
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(this->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProperty* property = iter->GetProperty();
    if (property)
    {
      vtkSMProperty* infoProperty = property->GetInformationProperty();
      if (infoProperty)
      {
        this->LinkProperty(infoProperty, property);
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();
  if (auto object = vtkObject::SafeDownCast(this->GetClientSideObject()))
  {
    this->VTKObjectObserverId =
      object->AddObserver(vtkCommand::ModifiedEvent, this, &vtkSMSettingsProxy::VTKObjectModified);
  }
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::VTKObjectModified()
{
  this->UpdatePropertyInformation();
  this->InvokeEvent(vtkCommand::ModifiedEvent);

  // FIXME: this should happen on UpdateProperty or PropertyModified, and not here.
  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->SetProxySettings(this);
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IsSerializable: " << this->IsSerializable << endl;
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::AddLink(const char* sourcePropertyName, vtkSMProxy* target,
  const char* targetPropertyName, bool unlink_if_modified)
{
  auto sourceProp = this->GetProperty(sourcePropertyName);
  if (!sourceProp)
  {
    vtkErrorMacro("Missing property with name '" << sourcePropertyName << "'.");
    return;
  }

  auto targetProp = target->GetProperty(targetPropertyName);
  if (!targetProp)
  {
    vtkErrorMacro("Missing property with name '" << targetPropertyName << "'.");
    return;
  }

  assert(sourceProp != targetProp);

  auto& internals = (*this->Internals);
  auto pitem = internals.FindItem(sourceProp, targetProp);
  if (pitem == nullptr)
  {
    sourceProp->AddLinkedProperty(targetProp);
    targetProp->Copy(sourceProp);
    target->UpdateVTKObjects();

    vtkInternals::Item item;
    item.Source = sourceProp;
    item.Target = targetProp;
    item.TargetProxy = target;
    item.SourceName = sourcePropertyName;
    item.TargetName = targetPropertyName;
    internals.Links.push_back(item);
    pitem = internals.FindItem(sourceProp, targetProp);
  }
  assert(pitem != nullptr);

  if (unlink_if_modified && pitem->ObserverId == 0)
  {
    pitem->ObserverId = targetProp->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkSMSettingsProxy::AutoBreakMapPropertyLink);
  }
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::RemoveLink(
  const char* sourcePropertyName, vtkSMProxy* target, const char* targetPropertyName)
{
  auto targetProperty = target ? target->GetProperty(targetPropertyName) : nullptr;
  auto sourceProperty = this->GetProperty(sourcePropertyName);

  auto& internals = (*this->Internals);
  for (auto& item : internals.Links)
  {
    if (item.Target == targetProperty && item.Source == sourceProperty)
    {
      // break this link.
      if (item.ObserverId > 0)
      {
        item.Target->RemoveObserver(item.ObserverId);
      }
      item.Source->RemoveLinkedProperty(item.Target);
      item.Target = nullptr;
      item.Source = nullptr;
      item.ObserverId = 0;
    }
  }

  internals.Prune();
}

//----------------------------------------------------------------------------
const char* vtkSMSettingsProxy::GetSourcePropertyName(
  vtkSMProxy* target, const char* targetPropertyName)
{
  auto targetProperty = target ? target->GetProperty(targetPropertyName) : nullptr;
  auto& internals = (*this->Internals);
  for (auto& item : internals.Links)
  {
    if (targetProperty != nullptr && item.Target == targetProperty)
    {
      // break this link.
      return item.SourceName.c_str();
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::AutoBreakMapPropertyLink(vtkObject* target, unsigned long, void*)
{
  auto& internals = (*this->Internals);
  if (internals.SourcePropertyIsBeingModified)
  {
    // nothing to break since the change is triggered due to a change in the
    // settings property.
    return;
  }

  for (auto& item : internals.Links)
  {
    if (item.Target == target && item.ObserverId != 0)
    {
      // break this link.
      // vtkLogF(INFO, "breaking link");
      item.Target->RemoveObserver(item.ObserverId);
      item.Source->RemoveLinkedProperty(item.Target);
      item.Target = nullptr;
      item.Source = nullptr;
      item.ObserverId = 0;
    }
  }

  internals.Prune();
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  auto& internals = (*this->Internals);
  internals.SourcePropertyIsBeingModified = true;
  this->Superclass::SetPropertyModifiedFlag(name, flag);
  internals.SourcePropertyIsBeingModified = false;
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::SaveLinksState(vtkPVXMLElement* root)
{
  vtkNew<vtkPVXMLElement> settings;
  settings->SetName("SettingsProxy");
  settings->AddAttribute("group", this->GetXMLGroup());
  settings->AddAttribute("type", this->GetXMLName());
  root->AddNestedElement(settings);

  vtkNew<vtkPVXMLElement> links;
  links->SetName("Links");
  settings->AddNestedElement(links);

  auto& internals = (*this->Internals);
  internals.Prune();
  for (const auto& item : internals.Links)
  {
    vtkNew<vtkPVXMLElement> property;
    property->SetName("Property");
    property->AddAttribute("source_property", item.SourceName.c_str());
    property->AddAttribute("target_id", item.TargetProxy->GetGlobalIDAsString());
    property->AddAttribute("target_property", item.TargetName.c_str());
    property->AddAttribute("unlink_if_modified", (item.ObserverId > 0 ? 1 : 0));
    links->AddNestedElement(property);
  }
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::LoadLinksState(vtkPVXMLElement* root, vtkSMProxyLocator* locator)
{
  auto links = root ? root->FindNestedElementByName("Links") : nullptr;
  if (!links)
  {
    return;
  }

  for (unsigned int cc = 0, max = links->GetNumberOfNestedElements(); cc < max; ++cc)
  {
    auto child = links->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Property") == 0)
    {
      auto source_property = child->GetAttribute("source_property");
      auto target_property = child->GetAttribute("target_property");
      int target_id = 0;
      if (child->GetScalarAttribute("target_id", &target_id) && source_property != nullptr &&
        target_property != nullptr && this->GetProperty(source_property))
      {
        auto target = locator ? locator->LocateProxy(target_id) : nullptr;
        if (target && target->GetProperty(target_property))
        {
          int unlink_if_modified = 0;
          child->GetScalarAttribute("unlink_if_modified", &unlink_if_modified);
          this->AddLink(source_property, target, target_property, unlink_if_modified != 0);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::ProcessPropertyLinks(vtkSMProxy* proxy)
{
  if (!proxy)
  {
    return;
  }

  auto pxm = proxy->GetSessionProxyManager();
  auto iter = proxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProperty* prop = iter->GetProperty();
    assert(prop != nullptr);
    auto hints = prop->GetHints();
    if (!hints)
    {
      continue;
    }

    const char* sourceGroupName = nullptr;
    const char* sourceProxyName = nullptr;
    const char* sourcePropertyName = nullptr;
    bool unlink_if_modified = false;

    if (auto gpLinkHint = hints->FindNestedElementByName("GlobalPropertyLink"))
    {
      sourceGroupName = "settings";
      sourceProxyName = gpLinkHint->GetAttributeOrEmpty("type");
      sourcePropertyName = gpLinkHint->GetAttributeOrEmpty("property");
      unlink_if_modified = true;
      if (sourceProxyName == nullptr || sourcePropertyName == nullptr)
      {
        vtkGenericWarningMacro("Invalid GlobalPropertyLink hint.");
        continue;
      }

      vtkGenericWarningMacro(
        "`GlobalPropertyLink` tag is deprecated. "
        "Please replace with `PropertyLink` tag. See documentation for details.");
    }
    else if (auto plHint = hints->FindNestedElementByName("PropertyLink"))
    {
      sourceGroupName = plHint->GetAttribute("group");
      sourceProxyName = plHint->GetAttribute("proxy");
      sourcePropertyName = plHint->GetAttribute("property");
      int iunlink_if_modified = 0;
      if (plHint->GetScalarAttribute("unlink_if_modified", &iunlink_if_modified))
      {
        unlink_if_modified = (iunlink_if_modified != 0);
      }
    }

    if (sourceGroupName && sourcePropertyName && sourceProxyName)
    {
      auto gpproxy =
        vtkSMSettingsProxy::SafeDownCast(pxm->GetProxy(sourceGroupName, sourceProxyName));
      if (gpproxy)
      {
        gpproxy->AddLink(sourcePropertyName, proxy, iter->GetKey(), unlink_if_modified);
      }
    }
  }
  iter->Delete();
}
