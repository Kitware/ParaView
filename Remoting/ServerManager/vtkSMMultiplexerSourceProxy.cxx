/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiplexerSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiplexerSourceProxy.h"

#include "vtkClientServerStream.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

class vtkSMMultiplexerSourceProxy::vtkInternals
{
public:
  struct Item
  {
    vtkSmartPointer<vtkSMProxy> Proxy;
    std::map<vtkSMProperty*, vtkSMProperty*> Links; // key: output, value: input

    void CollectLinksInformation(vtkSMMultiplexerSourceProxy* self, vtkPVXMLElement* hints);
    int IsInDomain(vtkSMInputProperty* requester) const;
    int IsInDomain() const;
  };

  std::vector<Item> Items;
  vtkWeakPointer<vtkSMProxy> SelectedProxy;
  bool SubproxiesInitialized = false;

  static void CopyUnchecked(vtkSMInputProperty* source, vtkSMInputProperty* dest)
  {
    dest->SetNumberOfUncheckedProxies(source->GetNumberOfUncheckedProxies());
    for (unsigned int cc = 0, max = source->GetNumberOfUncheckedProxies(); cc < max; ++cc)
    {
      dest->SetUncheckedInputConnection(
        cc, source->GetUncheckedProxy(cc), source->GetUncheckedOutputPortForConnection(cc));
    }
  }

  static const std::string CreateSubProxyName(vtkSMProxy* proxy)
  {
    std::ostringstream str;
    str << proxy->GetXMLGroup() << "." << proxy->GetXMLName();
    return str.str();
  }
};

//----------------------------------------------------------------------------
void vtkSMMultiplexerSourceProxy::vtkInternals::Item::CollectLinksInformation(
  vtkSMMultiplexerSourceProxy* self, vtkPVXMLElement* hints)
{
  // This helps use handle cases where the property names on the multiplexed
  // proxy and the mutliplexer proxy don't have same names. We look of
  // `LinkProperties`, is present. If not, linking is done using names.
  auto& links = this->Links;
  auto subproxy = this->Proxy;
  if (auto linkHints = (hints ? hints->FindNestedElementByName("LinkProperties") : nullptr))
  {
    for (unsigned int cc = 0; cc < linkHints->GetNumberOfNestedElements(); ++cc)
    {
      auto elem = linkHints->GetNestedElement(cc);
      if (strcmp(elem->GetName(), "Property") == 0)
      {
        auto property = self->GetProperty(elem->GetAttribute("with_property"));
        auto subproperty = subproxy->GetProperty(elem->GetAttribute("name"));
        if (property && subproperty)
        {
          links[subproperty] = property;
        }
        else
        {
          vtkErrorWithObjectMacro(self, "Invalid link specified!");
        }
      }
    }
  }
  else
  {
    auto piter = self->NewPropertyIterator();
    piter->SetTraverseSubProxies(0);
    for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
    {
      if (auto subproperty = subproxy->GetProperty(piter->GetKey()))
      {
        links[subproperty] = piter->GetProperty();
      }
    }
    piter->Delete();
  }
}

//----------------------------------------------------------------------------
int vtkSMMultiplexerSourceProxy::vtkInternals::Item::IsInDomain(vtkSMInputProperty* requester) const
{
  auto iter = std::find_if(this->Links.begin(), this->Links.end(),
    [requester](const std::pair<vtkSMProperty*, vtkSMProperty*>& pair) {
      return (pair.second == requester);
    });

  if (auto target =
        vtkSMInputProperty::SafeDownCast(iter != this->Links.end() ? iter->first : nullptr))
  {
    vtkSMMultiplexerSourceProxy::vtkInternals::CopyUnchecked(requester, target);
    const auto ret = target->IsInDomains();
    target->ClearUncheckedElements();
    return ret;
  }

  return vtkSMDomain::NOT_IN_DOMAIN;
}

//----------------------------------------------------------------------------
int vtkSMMultiplexerSourceProxy::vtkInternals::Item::IsInDomain() const
{
  int count = 0;
  for (auto& link : this->Links)
  {
    auto target = vtkSMInputProperty::SafeDownCast(link.first);
    auto src = vtkSMInputProperty::SafeDownCast(link.second);
    if (target && src)
    {
      vtkSMMultiplexerSourceProxy::vtkInternals::CopyUnchecked(src, target);
      const auto ret = target->IsInDomains();
      target->ClearUncheckedElements();
      if (ret == vtkSMDomain::IN_DOMAIN)
      {
        ++count;
      }
      else
      {
        return vtkSMDomain::NOT_IN_DOMAIN;
      }
    }
  }
  return (count == 0) ? vtkSMDomain::NOT_IN_DOMAIN : vtkSMDomain::IN_DOMAIN;
}

vtkStandardNewMacro(vtkSMMultiplexerSourceProxy);
//----------------------------------------------------------------------------
vtkSMMultiplexerSourceProxy::vtkSMMultiplexerSourceProxy()
  : Internals(new vtkSMMultiplexerSourceProxy::vtkInternals())
{
  this->SetSIClassName("vtkSIMultiplexerSourceProxy");
  this->SetVTKClassName("vtkPassThrough");
}

//----------------------------------------------------------------------------
vtkSMMultiplexerSourceProxy::~vtkSMMultiplexerSourceProxy()
{
}

//----------------------------------------------------------------------------
int vtkSMMultiplexerSourceProxy::CreateSubProxiesAndProperties(
  vtkSMSessionProxyManager* pxm, vtkPVXMLElement* element)
{
  if (!this->Superclass::CreateSubProxiesAndProperties(pxm, element))
  {
    return 0;
  }

  auto& internals = (*this->Internals);
  auto xmlgroup = this->GetXMLGroup();
  auto xmlname = this->GetXMLName();

  // TODO: make this fast. We should update vtkSIProxyDefinitionManager to give
  // api to get this with ease.
  auto iter = pxm->GetProxyDefinitionManager()->NewIterator();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto hints = iter->GetProxyHints();
    for (unsigned int cc = 0; cc < (hints ? hints->GetNumberOfNestedElements() : 0); ++cc)
    {
      auto child = hints->GetNestedElement(cc);
      if (child->GetName() && strcmp(child->GetName(), "MultiplexerSourceProxy") == 0 &&
        strcmp(child->GetAttributeOrEmpty("proxygroup"), xmlgroup) == 0 &&
        strcmp(child->GetAttributeOrEmpty("proxyname"), xmlname) == 0)
      {
        auto subproxy = pxm->NewProxy(iter->GetGroupName(), iter->GetProxyName());
        if (!subproxy)
        {
          vtkWarningMacro("Could not create proxy. Skipping.");
          continue;
        }
        vtkInternals::Item item;
        item.Proxy.TakeReference(subproxy);
        item.CollectLinksInformation(this, child);
        internals.Items.push_back(std::move(item));
      }
    }
  }
  iter->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMMultiplexerSourceProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  auto& internals = (*this->Internals);
  if (!internals.SubproxiesInitialized)
  {
    internals.SubproxiesInitialized = true;
    this->CurateApplicableProxies();
    this->SetupSubProxies();
  }
  this->Superclass::CreateVTKObjects();
  if (this->ObjectsCreated)
  {
    this->Select(this->Internals->SelectedProxy);
  }
}

//----------------------------------------------------------------------------
int vtkSMMultiplexerSourceProxy::IsInDomain(vtkSMInputProperty* prop)
{
  // if at least one of the "items" is in domain, we're in domain.
  const auto& internals = (*this->Internals);
  for (const auto& item : internals.Items)
  {
    if (item.IsInDomain(prop) == vtkSMDomain::IN_DOMAIN)
    {
      return vtkSMDomain::IN_DOMAIN;
    }
  }
  return vtkSMDomain::NOT_IN_DOMAIN;
}

//----------------------------------------------------------------------------
void vtkSMMultiplexerSourceProxy::CurateApplicableProxies()
{
  auto& internals = (*this->Internals);
  auto iter = std::remove_if(internals.Items.begin(), internals.Items.end(),
    [](const vtkInternals::Item& item) { return item.IsInDomain() != vtkSMDomain::IN_DOMAIN; });
  internals.Items.erase(iter, internals.Items.end());
  if (internals.Items.size() > 1)
  {
    vtkErrorMacro("Multiple proxies matched. This use-case is currently not supported.");
  }
}

//----------------------------------------------------------------------------
void vtkSMMultiplexerSourceProxy::SetupSubProxies()
{
  assert(!this->ObjectsCreated);
  auto& internals = (*this->Internals);
  for (const auto& item : internals.Items)
  {
    this->SetupSubProxy(item.Proxy);
  }
  if ((internals.Items.size() > 0) && (internals.SelectedProxy == nullptr))
  {
    internals.SelectedProxy = internals.Items[0].Proxy;
  }
}

//----------------------------------------------------------------------------
void vtkSMMultiplexerSourceProxy::SetupSubProxy(vtkSMProxy* subproxy)
{
  assert(!this->ObjectsCreated);
  assert(subproxy != nullptr);

  auto& internals = (*this->Internals);
  auto iter = std::find_if(internals.Items.begin(), internals.Items.end(),
    [subproxy](const vtkInternals::Item& item) { return (item.Proxy == subproxy); });
  assert(iter != internals.Items.end());

  // add as subproxy
  const auto subproxyname = vtkInternals::CreateSubProxyName(subproxy);
  this->AddSubProxy(subproxyname.c_str(), subproxy);

  // setup links: link properties on subproxy with properties on this proxy.
  // if hints are specified, we use those else we link all properties based on
  // the names.

  const auto& links = iter->Links;
  for (auto& pair : links)
  {
    this->LinkProperty(pair.second, pair.first);
  }

  // expose properties that are not linked.
  auto piter = subproxy->NewPropertyIterator();
  for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
  {
    if (links.find(piter->GetProperty()) == links.end())
    {
      this->ExposeSubProxyProperty(subproxyname.c_str(), piter->GetKey(), piter->GetKey(), 0);
    }
  }
  piter->Delete();

  // expose property groups
  for (size_t cc = 0, max = subproxy->GetNumberOfPropertyGroups(); cc < max; ++cc)
  {
    this->AppendPropertyGroup(subproxy->GetPropertyGroup(cc));
  }
}

//----------------------------------------------------------------------------
void vtkSMMultiplexerSourceProxy::Select(vtkSMProxy* proxy)
{
  this->CreateVTKObjects();

  auto& internals = (*this->Internals);
  internals.SelectedProxy = proxy;
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << SIPROXY(this) << "Select" << SIPROXY(proxy)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMMultiplexerSourceProxy::SaveXMLState(
  vtkPVXMLElement* root, vtkSMPropertyIterator* iter)
{
  if (auto elem = this->Superclass::SaveXMLState(root, iter))
  {
    auto& internals = (*this->Internals);
    vtkNew<vtkPVXMLElement> muxElem;
    muxElem->SetName("MultiplexerSourceProxy");
    if (internals.SelectedProxy)
    {
      muxElem->AddAttribute("selected_group", internals.SelectedProxy->GetXMLGroup());
      muxElem->AddAttribute("selected_type", internals.SelectedProxy->GetXMLName());
    }
    for (const auto& item : internals.Items)
    {
      vtkNew<vtkPVXMLElement> enabled_type;
      enabled_type->SetName("AvailableProxy");
      enabled_type->AddAttribute("group", item.Proxy->GetXMLGroup());
      enabled_type->AddAttribute("type", item.Proxy->GetXMLName());
      muxElem->AddNestedElement(enabled_type);
    }
    elem->AddNestedElement(muxElem);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkSMMultiplexerSourceProxy::LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator)
{
  auto muxElem = element->FindNestedElementByName("MultiplexerSourceProxy");
  if (!muxElem)
  {
    vtkErrorMacro("Failed to find 'MultiplexerSourceProxy' element. State incorrect!");
    return 0;
  }

  auto& internals = (*this->Internals);
  if (!internals.SubproxiesInitialized)
  {
    // curate subproxies based on the state and add them.
    internals.SubproxiesInitialized = true;

    std::set<std::pair<std::string, std::string> > available;
    for (unsigned int cc = 0, max = muxElem->GetNumberOfNestedElements(); cc < max; ++cc)
    {
      auto child = muxElem->GetNestedElement(cc);
      if (child && child->GetName() && strcmp(child->GetName(), "AvailableProxy") == 0 &&
        child->GetAttribute("group") && child->GetAttribute("type"))
      {
        available.insert(std::pair<std::string, std::string>(
          child->GetAttribute("group"), child->GetAttribute("type")));
      }
    }
    auto iter = std::remove_if(
      internals.Items.begin(), internals.Items.end(), [&available](const vtkInternals::Item& item) {
        const std::pair<std::string, std::string> pair(
          item.Proxy->GetXMLGroup(), item.Proxy->GetXMLName());
        return available.find(pair) == available.end();
      });
    internals.Items.erase(iter, internals.Items.end());
    this->SetupSubProxies();
  }

  // mark selected.
  const std::pair<std::string, std::string> selected(
    muxElem->GetAttributeOrEmpty("selected_group"), muxElem->GetAttributeOrEmpty("selected_type"));
  for (auto& item : internals.Items)
  {
    if (selected.first == item.Proxy->GetXMLGroup() && selected.second == item.Proxy->GetXMLName())
    {
      internals.SelectedProxy = item.Proxy;
      break;
    }
  }

  return this->Superclass::LoadXMLState(element, locator);
}

//----------------------------------------------------------------------------
void vtkSMMultiplexerSourceProxy::LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  auto& internals = (*this->Internals);
  if (!internals.SubproxiesInitialized)
  {
    // curate subproxies based on the state and add them.
    internals.SubproxiesInitialized = true;
    const int num_subproxies = msg->ExtensionSize(ProxyState::subproxy);
    std::set<std::string> chosen_proxynames;
    for (int cc = 0; cc < num_subproxies; ++cc)
    {
      const auto& subproxy_msg = msg->GetExtension(ProxyState::subproxy, cc);
      chosen_proxynames.insert(subproxy_msg.name());
    }
    auto iter = std::remove_if(internals.Items.begin(), internals.Items.end(),
      [&chosen_proxynames](const vtkInternals::Item& item) {
        return (chosen_proxynames.find(item.Proxy->GetXMLName()) == chosen_proxynames.end());
      });
    internals.Items.erase(iter, internals.Items.end());
    this->SetupSubProxies();
  }

  this->Superclass::LoadState(msg, locator);
}

//----------------------------------------------------------------------------
void vtkSMMultiplexerSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  const auto& internals = (*this->Internals);
  os << indent << "Available multiplexer proxy types (size=" << internals.Items.size() << ")"
     << endl;
  for (const auto& item : internals.Items)
  {
    os << indent.GetNextIndent() << "Proxy: group='" << item.Proxy->GetXMLGroup() << "', "
       << "name='" << item.Proxy->GetXMLName() << "'" << endl;
  }
  if (internals.SelectedProxy == nullptr)
  {
    os << indent << "Selected proxy type: (none)" << endl;
  }
  else
  {
    os << indent << "Selected proxy type: group='" << internals.SelectedProxy->GetXMLGroup()
       << "', "
       << "name='" << internals.SelectedProxy->GetXMLName() << "'" << endl;
  }
}
