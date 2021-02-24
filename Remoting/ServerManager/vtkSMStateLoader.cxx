/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateLoader.h"

#include "vtkClientServerStreamInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettingsProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStateVersionController.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <cstdlib>
#include <vector>

vtkObjectFactoryNewMacro(vtkSMStateLoader);
vtkCxxSetObjectMacro(vtkSMStateLoader, ProxyLocator, vtkSMProxyLocator);
//---------------------------------------------------------------------------
struct vtkSMStateLoaderRegistrationInfo
{
  std::string GroupName;
  std::string ProxyName;
  std::string LogName;
};

struct vtkSMStateLoaderInternals
{
  bool KeepOriginalId;
  typedef std::vector<vtkSMStateLoaderRegistrationInfo> VectorOfRegInfo;
  typedef std::map<int, VectorOfRegInfo> RegInfoMapType;
  RegInfoMapType RegistrationInformation;
  std::vector<vtkTypeUInt32> AlignedMappingIdTable;

  /// Vector filled up in CreatedNewProxy() to keep order in which the proxies
  /// are created.
  typedef std::pair<vtkTypeUInt32, vtkWeakPointer<vtkSMProxy> > ProxyCreationOrderItem;
  typedef std::vector<ProxyCreationOrderItem> ProxyCreationOrderType;
  ProxyCreationOrderType ProxyCreationOrder;
  bool DeferProxyRegistration;

  vtkSMStateLoaderInternals()
    : KeepOriginalId(false)
    , DeferProxyRegistration(false)
  {
  }
};

//---------------------------------------------------------------------------
vtkSMStateLoader::vtkSMStateLoader()
{
  this->Internal = new vtkSMStateLoaderInternals;
  this->ServerManagerStateElement = nullptr;
  this->KeepIdMapping = 0;
  this->ProxyLocator = vtkSMProxyLocator::New();
}

//---------------------------------------------------------------------------
vtkSMStateLoader::~vtkSMStateLoader()
{
  this->SetProxyLocator(nullptr);
  this->ServerManagerStateElement = nullptr;
  this->ProxyLocator = nullptr;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoader::LocateExistingProxyUsingRegistrationName(vtkTypeUInt32 id)
{
  vtkSMStateLoaderInternals::RegInfoMapType::iterator iter =
    this->Internal->RegistrationInformation.find(id);
  if (iter == this->Internal->RegistrationInformation.end())
  {
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  assert(pxm != nullptr);

  vtkSMStateLoaderInternals::VectorOfRegInfo::iterator iter2;
  for (iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++)
  {
    vtkSMProxy* proxy = pxm->GetProxy(iter2->GroupName.c_str(), iter2->ProxyName.c_str());
    if (proxy)
    {
      return proxy;
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoader::CreateProxy(
  const char* xml_group, const char* xml_name, const char* subProxyName)
{
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  assert(pxm != nullptr);

  //**************************************************************************
  // This is temporary code until we clean up time-keeper and animation scene
  // interactions. There needs to be some rework with the management of
  // time-keeper, making it a SM-behavior perhaps. Until that happens, I am
  // letting this piece of code be which ensures that there's only open
  // time-keeper and animation scene in the application.
  if (xml_group && xml_name && strcmp(xml_group, "animation") == 0 &&
    strcmp(xml_name, "AnimationScene") == 0)
  {
    // If an animation scene already exists, we use that.
    vtkSMProxy* scene = pxm->FindProxy("animation", "animation", "AnimationScene");
    if (scene)
    {
      scene->Register(this);
      return scene;
    }
  }
  else if (xml_group && xml_name && strcmp(xml_group, "animation") == 0 &&
    strcmp(xml_name, "TimeAnimationCue") == 0)
  {
    // If an animation cue already exists, we use that.
    vtkSMProxy* cue = pxm->FindProxy("animation", "animation", "TimeAnimationCue");
    if (cue)
    {
      cue->Register(this);
      return cue;
    }
  }
  else if (xml_group && xml_name && strcmp(xml_group, "misc") == 0 &&
    strcmp(xml_name, "TimeKeeper") == 0)
  {
    // There is only one time keeper per connection, simply
    // load the state on the timekeeper.
    vtkSMProxy* timekeeper = pxm->FindProxy("timekeeper", xml_group, xml_name);
    if (timekeeper)
    {
      timekeeper->Register(this);
      return timekeeper;
    }
  }
  else if (xml_group && xml_name && strcmp(xml_group, "materials") == 0 &&
    strcmp(xml_name, "MaterialLibrary") == 0)
  {
    // There is only one material library proxy as well.
    vtkSMProxy* materiallibrary = pxm->FindProxy("materiallibrary", "materials", "MaterialLibrary");
    if (materiallibrary)
    {
      materiallibrary->Register(this);
      return materiallibrary;
    }
  }
  else if (xml_group && xml_name && strcmp(xml_group, "coprocessing") == 0 &&
    strcmp(xml_name, "CatalystGlobalOptions") == 0)
  {
    // If a Catalyst export options already exists, we use that.
    vtkSMProxy* exporter = pxm->FindProxy("export_global", "coprocessing", "CatalystGlobalOptions");
    if (exporter)
    {
      exporter->Register(this);
      return exporter;
    }
  }
  else if (xml_group && xml_name && strcmp(xml_group, "settings") == 0 &&
    strcmp(xml_name, "ColorPalette") == 0)
  {
    if (auto palette = pxm->FindProxy("settings", "settings", "ColorPalette"))
    {
      palette->Register(this);
      return palette;
    }
  }

  //**************************************************************************

  // If all else fails, let the superclass handle it:
  return this->Superclass::CreateProxy(xml_group, xml_name, subProxyName);
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::CreatedNewProxy(vtkTypeUInt32 id, vtkSMProxy* proxy)
{
  // set logname first, if provided by state file.
  const auto reginfoIter = this->Internal->RegistrationInformation.find(id);
  if (reginfoIter != this->Internal->RegistrationInformation.end())
  {
    for (const auto& regInfo : reginfoIter->second)
    {
      if (!regInfo.LogName.empty())
      {
        proxy->SetLogNameInternal(regInfo.LogName.c_str(), /*propagate_to_subproxies=*/true,
          /*propagate_to_proxylistdomains=*/false);
        break;
      }
    }
  }

  // Ensure that the proxy is created before it is registered, unless we are
  // reviving the server-side server manager, which needs special handling.
  if (this->Internal->KeepOriginalId)
  {
    proxy->SetGlobalID(id);
  }

  // Calling UpdateVTKObjects() will assign the proxy a GlobalId, if needed.
  proxy->UpdateVTKObjects();
  if (proxy->IsA("vtkSMSourceProxy"))
  {
    vtkSMSourceProxy::SafeDownCast(proxy)->UpdatePipelineInformation();
  }
  if (this->Internal->DeferProxyRegistration)
  {
    this->Internal->ProxyCreationOrder.push_back(
      vtkSMStateLoaderInternals::ProxyCreationOrderItem(id, proxy));
  }
  else
  {
    this->RegisterProxy(id, proxy);
  }
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::RegisterProxy(vtkTypeUInt32 id, vtkSMProxy* proxy)
{
  vtkSMStateLoaderInternals::RegInfoMapType::iterator iter =
    this->Internal->RegistrationInformation.find(id);
  if (iter == this->Internal->RegistrationInformation.end())
  {
    return;
  }
  for (const auto& regInfo : iter->second)
  {
    this->RegisterProxyInternal(regInfo.GroupName.c_str(), regInfo.ProxyName.c_str(), proxy);
  }
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::RegisterProxyInternal(
  const char* cgroup, const char* cname, vtkSMProxy* proxy)
{
  assert(cgroup != nullptr && cname != nullptr);

  std::string group(cgroup);
  std::string name(cname);
  if (this->UpdateRegistrationInfo(group, name, proxy))
  {
    vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
    assert(pxm != nullptr);
    if (pxm->GetProxyName(group.c_str(), proxy))
    {
      // Don't re-register a proxy in the same group.
      return;
    }
    pxm->RegisterProxy(group.c_str(), name.c_str(), proxy);
  }
}

//---------------------------------------------------------------------------
bool vtkSMStateLoader::UpdateRegistrationInfo(
  std::string& group, std::string& name, vtkSMProxy* vtkNotUsed(proxy))
{
  static const char* helper_proxies_prefix = "pq_helper_proxies.";
  static size_t len = strlen(helper_proxies_prefix);
  if (group.compare(0, len, helper_proxies_prefix) == 0)
  {
    // a helper proxy groupname, must update it.
    std::string pid = group.substr(len);
    vtkTypeUInt32 gid = static_cast<vtkTypeUInt32>(std::atoi(pid.c_str()));
    if (vtkSMProxy* helpedProxy = this->ProxyLocator->LocateProxy(gid))
    {
      group = helper_proxies_prefix;
      group += helpedProxy->GetGlobalIDAsString();
    }
  }

  if (group == "lookup_tables" || group == "piecewise_functions")
  {
    // A separated lookup table or piecewise function, must update it.
    const std::string separatePrefix = "Separate_";
    const size_t separateLen = separatePrefix.size();
    if (name.substr(0, separateLen) == separatePrefix)
    {
      // This will change any "Separate_OLDGID_ArrayName" into "Separate_NEWGID_ArrayName"
      size_t gidLen = name.find_first_of('_', separateLen) - separateLen;
      std::string gidStr = name.substr(separateLen, gidLen);
      vtkTypeUInt32 gid = static_cast<vtkTypeUInt32>(std::atoi(gidStr.c_str()));
      if (vtkSMProxy* helpedProxy = this->ProxyLocator->LocateProxy(gid))
      {
        name.replace(separateLen, gidLen, helpedProxy->GetGlobalIDAsString());
      }
    }
  }

  return true;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMStateLoader::LocateProxyElement(vtkTypeUInt32 id)
{
  return this->LocateProxyElementInternal(this->ServerManagerStateElement, id);
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMStateLoader::LocateProxyElementInternal(
  vtkPVXMLElement* root, vtkTypeUInt32 id_)
{
  if (!root)
  {
    vtkErrorMacro("No root is defined. Cannot locate proxy element with id " << id_);
    return nullptr;
  }
  vtkIdType id = static_cast<vtkIdType>(id_);

  unsigned int numElems = root->GetNumberOfNestedElements();
  unsigned int i = 0;
  for (i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    if (currentElement->GetName() && strcmp(currentElement->GetName(), "Proxy") == 0)
    {
      vtkIdType currentId;
      if (!currentElement->GetScalarAttribute("id", &currentId))
      {
        continue;
      }
      if (id != currentId)
      {
        continue;
      }
      return currentElement;
    }
  }

  // If proxy was not found on root level, go into nested elements
  for (i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    vtkPVXMLElement* res = this->LocateProxyElementInternal(currentElement, id);
    if (res)
    {
      return res;
    }
  }

  return nullptr;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::BuildProxyCollectionInformation(vtkPVXMLElement* collectionElement)
{
  const char* groupName = collectionElement->GetAttribute("name");
  if (!groupName)
  {
    vtkErrorMacro("Required attribute name is missing.");
    return 0;
  }
  unsigned int numElems = collectionElement->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = collectionElement->GetNestedElement(i);
    if (currentElement->GetName() && strcmp(currentElement->GetName(), "Item") == 0)
    {
      int id;
      if (!currentElement->GetScalarAttribute("id", &id))
      {
        vtkErrorMacro("Could not read id for Item. Skipping.");
        continue;
      }
      const char* name = currentElement->GetAttribute("name");
      if (!name)
      {
        vtkErrorMacro("Attribute: name is missing. Cannot register proxy "
                      "with the proxy manager.");
        continue;
      }

      auto logname = currentElement->GetAttribute("logname");

      vtkSMStateLoaderRegistrationInfo info;
      info.GroupName = groupName;
      info.ProxyName = name;
      info.LogName = (logname ? logname : "");
      this->Internal->RegistrationInformation[id].push_back(info);
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::HandleProxyCollection(vtkPVXMLElement* collectionElement)
{
  const char* groupName = collectionElement->GetAttribute("name");
  if (!groupName)
  {
    vtkErrorMacro("Required attribute name is missing.");
    return 0;
  }
  unsigned int numElems = collectionElement->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = collectionElement->GetNestedElement(i);
    if (currentElement->GetName() && strcmp(currentElement->GetName(), "Item") == 0)
    {
      int id;
      if (!currentElement->GetScalarAttribute("id", &id))
      {
        vtkErrorMacro("Could not read id for Item. Skipping.");
        continue;
      }

      vtkSMProxy* proxy = this->ProxyLocator->LocateProxy(id);
      if (!proxy)
      {
        continue;
      }
      const char* name = currentElement->GetAttribute("name");
      if (!name)
      {
        vtkErrorMacro("Attribute: name is missing. Cannot register proxy "
                      "with the proxy manager.");
        proxy->Delete();
        continue;
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::HandleCustomProxyDefinitions(vtkPVXMLElement* element)
{
  vtkSMSessionProxyManager* pm = this->GetSessionProxyManager();
  assert(pm != nullptr);
  pm->LoadCustomProxyDefinitions(element);
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::HandleLinks(vtkPVXMLElement* element)
{
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  assert(pxm != nullptr);

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkPVXMLElement* currentElement = element->GetNestedElement(cc);
    const char* name = currentElement->GetName();
    const char* linkname = currentElement->GetAttribute("name");

    if (name && linkname)
    {
      vtkSMLink* link = pxm->GetRegisteredLink(linkname);
      if (link == nullptr)
      {
        std::string classname = "vtkSM";
        classname += name;
        vtkSmartPointer<vtkObjectBase> obj;
        obj.TakeReference(vtkClientServerStreamInstantiator::CreateInstance(classname.c_str()));
        link = vtkSMLink::SafeDownCast(obj);
        if (link == nullptr)
        {
          vtkWarningMacro("Failed to create object for link (name="
            << name << "). Expected type was " << classname.c_str() << ". Skipping.");
          continue;
        }
        pxm->RegisterLink(linkname, link);
      }
      assert(link != nullptr);
      if (!link->LoadXMLState(currentElement, this->ProxyLocator))
      {
        return 0;
      }
    }
  }

  // Load the global_properties
  if (vtkSMProxy* globalPropertiesProxy = pxm->GetProxy("global_properties", "ColorPalette"))
  {
    globalPropertiesProxy->LoadXMLState(element, this->ProxyLocator);
  }

  return 1;
}

//---------------------------------------------------------------------------
bool vtkSMStateLoader::VerifyXMLVersion(vtkPVXMLElement* rootElement)
{
  const char* version = rootElement->GetAttribute("version");
  if (!version)
  {
    vtkWarningMacro("ServerManagerState missing \"version\" information.");
    return true;
  }

  // Nothing to check here really.
  return true;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::LoadState(vtkPVXMLElement* elem, bool keepOriginalId)
{
  this->Internal->KeepOriginalId = keepOriginalId;
  if (!elem)
  {
    vtkErrorMacro("Cannot load state from (null) root element.");
    return 0;
  }

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  if (pxm == nullptr)
  {
    vtkErrorMacro("Cannot load state without a ProxyManager");
    return 0;
  }

  if (!this->ProxyLocator)
  {
    vtkErrorMacro("Please set the locator correctly.");
    return 0;
  }

  this->ProxyLocator->SetDeserializer(this);
  int ret = this->LoadStateInternal(elem);
  this->ProxyLocator->SetDeserializer(nullptr);

  // BUG #10650. When animation scene time ranges are read from the state, they
  // often override those that the timekeeper painstakingly computed. Here we
  // explicitly trigger the timekeeper so that the scene re-determines the
  // ranges, unless they are locked of course.
  vtkSMProxy* timekeeper = pxm->GetProxy("timekeeper", "TimeKeeper");
  if (timekeeper)
  {
    timekeeper->GetProperty("TimeRange")->Modified();
    timekeeper->GetProperty("TimestepValues")->Modified();
  }

  return ret;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::LoadStateInternal(vtkPVXMLElement* parent)
{
  vtkPVXMLElement* rootElement = parent;
  if (rootElement->GetName() && strcmp(rootElement->GetName(), "ServerManagerState") != 0)
  {
    rootElement = rootElement->FindNestedElementByName("ServerManagerState");
    if (!rootElement)
    {
      vtkErrorMacro("Failed to locate <ServerManagerState /> element."
        << "Cannot load server manager state.");
      return 0;
    }
  }

  vtkSMStateVersionController* converter = vtkSMStateVersionController::New();
  if (!converter->Process(parent, this->GetSession()))
  {
    vtkWarningMacro("State converter was not able to convert the state to current "
                    "version successfully");
  }
  converter->Delete();

  if (!this->VerifyXMLVersion(rootElement))
  {
    return 0;
  }

  this->ServerManagerStateElement = rootElement;

  unsigned int numElems = rootElement->GetNumberOfNestedElements();
  unsigned int i;
  for (i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name)
    {
      if (strcmp(name, "ProxyCollection") == 0)
      {
        if (!this->BuildProxyCollectionInformation(currentElement))
        {
          return 0;
        }
      }
    }
  }

  // Load all compound proxy definitions.
  for (i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name)
    {
      if (strcmp(name, "CustomProxyDefinitions") == 0)
      {
        this->HandleCustomProxyDefinitions(currentElement);
      }
    }
  }

  // Iterate over all proxy collections to create all proxies. None are
  // registered at this point, just created. Since we don't register proxies
  // here, we have to take special care for loading state of proxies that are
  // already registered. These include "TimeKeeper", "AnimationScene", and
  // "TimeAnimationCue". We simply defer creation of proxies in "animation"
  // and "timekeeper" group until all other proxies have been created and
  // registered. That way, when properties on TimeKeeper or AnimationScene
  // start getting modified, the proxies they may refer to are already
  // present and registered.
  std::vector<vtkSmartPointer<vtkPVXMLElement> > deferredCollections;
  this->Internal->DeferProxyRegistration = true;
  for (i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name != nullptr && strcmp(name, "ProxyCollection") == 0)
    {
      const char* group_name = currentElement->GetAttributeOrEmpty("name");
      if (strcmp(group_name, "animation") == 0 || strcmp(group_name, "timekeeper") == 0)
      {
        deferredCollections.push_back(currentElement);
      }
      else if (!this->HandleProxyCollection(currentElement))
      {
        return 0;
      }
    }
  }

  // Register proxies in order they were created (as that's a good dependency
  // order).
  for (vtkSMStateLoaderInternals::ProxyCreationOrderType::const_iterator iter =
         this->Internal->ProxyCreationOrder.begin();
       iter != this->Internal->ProxyCreationOrder.end(); ++iter)
  {
    this->RegisterProxy(iter->first, iter->second);
  }
  this->Internal->ProxyCreationOrder.clear();

  // Now handle animation and timekeeper collections. This time, we let the
  // proxies be registered as needed.
  this->Internal->DeferProxyRegistration = false;
  for (size_t cc = 0; cc < deferredCollections.size(); ++cc)
  {
    if (!this->HandleProxyCollection(deferredCollections[cc]))
    {
      return 0;
    }
  }
  assert(this->Internal->ProxyCreationOrder.size() == 0);

  // Process link elements.
  for (i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name && strcmp(name, "Links") == 0)
    {
      this->HandleLinks(currentElement);
    }
  }

  // Process settings links.
  auto pxm = this->GetSessionProxyManager();
  for (i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name && strcmp(name, "Settings") == 0)
    {
      for (int cc = 0, max = currentElement->GetNumberOfNestedElements(); cc < max; ++cc)
      {
        auto sproxy = currentElement->GetNestedElement(cc);
        if (sproxy && sproxy->GetName() && strcmp(sproxy->GetName(), "SettingsProxy") == 0)
        {
          auto settingsProxy = vtkSMSettingsProxy::SafeDownCast(
            pxm->GetProxy(sproxy->GetAttribute("group"), sproxy->GetAttribute("type")));
          if (settingsProxy)
          {
            settingsProxy->LoadLinksState(sproxy, this->ProxyLocator);
          }
        }
      }
    }
  }

  // If KeepIdMapping
  this->Internal->AlignedMappingIdTable.clear();
  if (this->KeepIdMapping != 0)
  {
    vtkSMStateLoaderInternals::RegInfoMapType::iterator iter =
      this->Internal->RegistrationInformation.begin();
    while (iter != this->Internal->RegistrationInformation.end())
    {
      vtkSMProxy* proxy = this->LocateExistingProxyUsingRegistrationName(iter->first);
      if (proxy)
      {
        this->Internal->AlignedMappingIdTable.push_back(iter->first);
        this->Internal->AlignedMappingIdTable.push_back(proxy->GetGlobalID());
      }
      // Move forward
      iter++;
    }
  }

  // Clear internal data structures.
  this->Internal->ProxyCreationOrder.clear();
  this->Internal->RegistrationInformation.clear();
  this->ServerManagerStateElement = nullptr;
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkTypeUInt32* vtkSMStateLoader::GetMappingArray(int& size)
{
  size = static_cast<int>(this->Internal->AlignedMappingIdTable.size());
  return &this->Internal->AlignedMappingIdTable[0];
}
