/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyPropertyInternals.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStateLocator.h"
#include "vtkSmartPointer.h"

#include <assert.h>

namespace
{
template <class T>
void vtkGetProxyListValues(
  T& tvalues, vtkSMProxyListDomain* tpld, const T& svalues, vtkSMProxyListDomain* spld)
{
  for (typename T::const_iterator siter = svalues.begin(); siter != svalues.end(); ++siter)
  {
    vtkSMProxy* svalue = siter->GetPointer();
    vtkSMProxy* tvalue = tpld->GetProxyWithName(spld->GetProxyName(svalue));
    if (tvalue && svalue)
    {
      tvalue->Copy(svalue);
    }
    tvalues.push_back(tvalue);
  }
}
}

bool vtkSMProxyProperty::CreateProxyAllowed = false; // static init

//***************************************************************************
vtkStandardNewMacro(vtkSMProxyProperty);
//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->PPInternals = new vtkSMProxyProperty::vtkPPInternals(this);
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  delete this->PPInternals;
  this->PPInternals = NULL;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy)
{
  if (this->PPInternals->Add(proxy))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveProxy(vtkSMProxy* proxy)
{
  if (this->PPInternals->Remove(proxy))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllProxies()
{
  if (this->PPInternals->Clear())
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetProxy(unsigned int index, vtkSMProxy* proxy)
{
  if (this->PPInternals->Set(index, proxy))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetProxies(unsigned int numProxies, vtkSMProxy* proxies[])
{
  if (this->PPInternals->Set(numProxies, proxies))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetNumberOfProxies(unsigned int count)
{
  if (this->PPInternals->Resize(count))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddUncheckedProxy(vtkSMProxy* proxy)
{
  if (this->PPInternals->AddUnchecked(proxy))
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy)
{
  if (this->PPInternals->SetUnchecked(idx, proxy))
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllUncheckedProxies()
{
  if (this->PPInternals->ClearUnchecked())
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
bool vtkSMProxyProperty::IsProxyAdded(vtkSMProxy* proxy)
{
  return this->PPInternals->IsAdded(proxy);
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfProxies()
{
  return static_cast<unsigned int>(this->PPInternals->GetProxies().size());
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfUncheckedProxies()
{
  if (this->PPInternals->GetUncheckedProxies().size() > 0)
  {
    return static_cast<unsigned int>(this->PPInternals->GetUncheckedProxies().size());
  }
  else
  {
    return this->GetNumberOfProxies();
  }
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetProxy(unsigned int idx)
{
  return this->PPInternals->Get(idx);
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetUncheckedProxy(unsigned int idx)
{
  return this->PPInternals->GetUnchecked(idx);
}
//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetNumberOfUncheckedProxies(unsigned int count)
{
  if (this->PPInternals->ResizeUnchecked(count))
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::WriteTo(vtkSMMessage* message)
{
  ProxyState_Property* prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  this->PPInternals->WriteTo(prop->mutable_value());

  // Generally, domains don't need to be serialized in protobuf state. The only
  // except in vtkSMProxyListDomain. That domain has a state that need to
  // serialized and preserved esp. for undo-redo and collaboration to work
  // correctly.
  for (this->DomainIterator->Begin(); !this->DomainIterator->IsAtEnd();
       this->DomainIterator->Next())
  {
    vtkSMProxyListDomain* pld =
      vtkSMProxyListDomain::SafeDownCast(this->DomainIterator->GetDomain());
    if (pld)
    {
      // for each vtkSMProxyListDomain, add all proxies in that domain to the
      // message.
      ProxyState_UserData* user_data = prop->add_user_data();
      user_data->set_key("ProxyListDomain"); // fixme -- add domain name, maybe?
      Variant* variant = user_data->add_variant();
      variant->set_type(Variant::PROXY);
      for (unsigned int cc = 0, max = pld->GetNumberOfProxies(); cc < max; cc++)
      {
        variant->add_proxy_global_id(pld->GetProxy(cc) ? pld->GetProxy(cc)->GetGlobalID() : 0);
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::ReadFrom(
  const vtkSMMessage* message, int msg_offset, vtkSMProxyLocator* locator)
{
  const ProxyState_Property* prop = &message->GetExtension(ProxyState::property, msg_offset);
  if (strcmp(prop->name().c_str(), this->GetXMLName()) != 0)
  {
    vtkErrorMacro("Invalid offset for this property. "
                  "Typically indicates that state being loaded is invalid/corrupted.");
    return;
  }

  if (this->PPInternals->ReadFrom(prop->value(), locator))
  {
    this->Modified();
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }

  // The EXCEPTION case: read proxy list domain state.
  int pld_index = 0;
  for (this->DomainIterator->Begin(); !this->DomainIterator->IsAtEnd();
       this->DomainIterator->Next())
  {
    vtkSMProxyListDomain* pld =
      vtkSMProxyListDomain::SafeDownCast(this->DomainIterator->GetDomain());
    if (pld)
    {
      if (pld_index >= prop->user_data_size())
      {
        vtkWarningMacro("Missing matched in ProxyListDomain state.");
        continue;
      }

      const ProxyState_UserData& user_data = prop->user_data(pld_index);
      pld_index++;

      std::vector<vtkSMProxy*> proxies;

      assert(user_data.key() == "ProxyListDomain");
      assert(user_data.variant_size() == 1);

      for (int cc = 0, max = user_data.variant(0).proxy_global_id_size(); cc < max; cc++)
      {
        vtkTypeUInt32 gid = user_data.variant(0).proxy_global_id(cc);

        // either ask the locator for the proxy, or find an existing one.
        vtkSMProxy* proxy = NULL;
        if (locator && vtkSMProxyProperty::CanCreateProxy())
        {
          proxy = locator->LocateProxy(gid);
        }
        else
        {
          proxy = vtkSMProxy::SafeDownCast(this->GetParent()->GetSession()->GetRemoteObject(gid));
        }
        if (proxy != NULL || gid == 0)
        {
          proxies.push_back(proxy);
        }
      }
      proxies.push_back(NULL);

      pld->SetProxies(&proxies[0], static_cast<unsigned int>(proxies.size() - 1));
    }
  }
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element)
{
  return this->Superclass::ReadXMLAttributes(parent, element);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);
  vtkSMProxyProperty* psrc = vtkSMProxyProperty::SafeDownCast(src);
  if (!psrc)
  {
    return;
  }

  auto spld = src->FindDomain<vtkSMProxyListDomain>();
  auto tpld = this->FindDomain<vtkSMProxyListDomain>();

  bool modified = false;
  bool unchecked_modified = false;

  if (spld && tpld && psrc->GetNumberOfProxies() > 0)
  {
    vtkPPInternals::SmartVectorOfProxies tvalues;
    vtkGetProxyListValues(tvalues, tpld, psrc->PPInternals->GetProxies(), spld);
    modified = this->PPInternals->SetProxies(tvalues, psrc->PPInternals->GetPorts());

    vtkPPInternals::WeakVectorOfProxies tuvalues;
    vtkGetProxyListValues(tuvalues, tpld, psrc->PPInternals->GetUncheckedProxies(), spld);
    unchecked_modified =
      this->PPInternals->SetUncheckedProxies(tuvalues, psrc->PPInternals->GetUncheckedPorts());
  }
  else
  {
    modified =
      this->PPInternals->SetProxies(psrc->PPInternals->GetProxies(), psrc->PPInternals->GetPorts());
    unchecked_modified = this->PPInternals->SetUncheckedProxies(
      psrc->PPInternals->GetUncheckedProxies(), psrc->PPInternals->GetUncheckedPorts());
  }

  if (modified)
  {
    this->Modified();
  }
  if (modified || unchecked_modified)
  {
    this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // os << indent << "Values: ";
  // for (unsigned int i=0; i<this->GetNumberOfProxies(); i++)
  //  {
  //  os << this->GetProxy(i) << " ";
  //  }
  // os << endl;
}
//---------------------------------------------------------------------------
void vtkSMProxyProperty::SaveStateValues(vtkPVXMLElement* propertyElement)
{
  unsigned int size = this->GetNumberOfProxies();
  if (size > 0)
  {
    propertyElement->AddAttribute("number_of_elements", size);
  }
  for (unsigned int i = 0; i < size; i++)
  {
    this->AddProxyElementState(propertyElement, i);
  }
}
//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyProperty::AddProxyElementState(vtkPVXMLElement* prop, unsigned int idx)
{
  vtkSMProxy* proxy = this->GetProxy(idx);
  vtkPVXMLElement* proxyElement = 0;
  if (proxy)
  {
    proxyElement = vtkPVXMLElement::New();
    proxyElement->SetName("Proxy");
    proxyElement->AddAttribute("value", static_cast<unsigned int>(proxy->GetGlobalID()));
    prop->AddNestedElement(proxyElement);
    proxyElement->FastDelete();
  }
  return proxyElement;
}
//---------------------------------------------------------------------------
// NOTE: This method is duplicated in some way in vtkSMInputProperty
// Therefore, care must be taken to keep the two in sync.
int vtkSMProxyProperty::LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader)
{
  if (!loader)
  {
    // no loader specified, state remains unchanged.
    return 1;
  }

  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  this->Superclass::LoadState(element, loader);

  // If "clear" is present and is 0, it implies that the proxy elements
  // currently in the property should not be cleared before loading
  // the new state.
  int clear = 1;
  element->GetScalarAttribute("clear", &clear);
  if (clear)
  {
    this->PPInternals->Clear();
  }

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() && (strcmp(currentElement->GetName(), "Element") == 0 ||
                                       strcmp(currentElement->GetName(), "Proxy") == 0))
    {
      int id;
      if (currentElement->GetScalarAttribute("value", &id))
      {
        if (id)
        {
          int port = 0;
          // if output_port is not present, port remains 0.
          currentElement->GetScalarAttribute("output_port", &port);
          vtkSMProxy* proxy = loader->LocateProxy(id);
          if (proxy)
          {
            this->PPInternals->Add(proxy, port);
          }
          else
          {
            // It is not an error to have missing proxies in a proxy property.
            // We simply ignore such proxies.
            // vtkErrorMacro("Could not create proxy of id: " << id);
            // return 0;
          }
        }
        else
        {
          this->PPInternals->Add(NULL);
        }
      }
    }
  }

  // Do not immediately update. Leave it to the loader.
  this->Modified();
  this->InvokeEvent(vtkCommand::UncheckedPropertyModifiedEvent);

  this->ImmediateUpdate = prevImUpdate;
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::UpdateAllInputs()
{
  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int idx = 0; idx < numProxies; idx++)
  {
    vtkSMProxy* proxy = this->GetProxy(idx);
    if (proxy)
    {
      proxy->UpdateSelfAndAllInputs();
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::EnableProxyCreation()
{
  vtkSMProxyProperty::CreateProxyAllowed = true;
}
//---------------------------------------------------------------------------
void vtkSMProxyProperty::DisableProxyCreation()
{
  vtkSMProxyProperty::CreateProxyAllowed = false;
}
//---------------------------------------------------------------------------
bool vtkSMProxyProperty::CanCreateProxy()
{
  return vtkSMProxyProperty::CreateProxyAllowed;
}

//---------------------------------------------------------------------------
bool vtkSMProxyProperty::IsValueDefault()
{
  // proxy properties are default only if they contain no proxies
  return this->GetNumberOfProxies() == 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::ResetToXMLDefaults()
{
  this->RemoveAllProxies();
}
