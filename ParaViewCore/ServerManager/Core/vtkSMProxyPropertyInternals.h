/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyPropertyInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMProxyPropertyInternals_h
#define vtkSMProxyPropertyInternals_h

#include "vtkCommand.h"
#include "vtkSMMessage.h"
#include "vtkSMObject.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSession.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

/// This class is used by vtkSMProxyProperty to keep track of the proxies. It
/// keeps proxies as well as output port information which is needed for
/// vtkSMInputProperty. That makes it easier and less error prone to keep the
/// number of ports and proxies in sync and correctly check when values are
/// modified.
class vtkSMProxyProperty::vtkPPInternals
{
public:
  typedef std::vector<vtkSmartPointer<vtkSMProxy> > SmartVectorOfProxies;
  typedef std::vector<vtkWeakPointer<vtkSMProxy> > WeakVectorOfProxies;
  typedef std::vector<unsigned int> VectorOfUInts;

private:
  vtkWeakPointer<vtkSMProxyProperty> Self;
  SmartVectorOfProxies Proxies;
  SmartVectorOfProxies PreviousProxies; // < These are used to remove observers
                                        // and break producer/consumer links
                                        // whenever the value is changed.
  WeakVectorOfProxies UncheckedProxies;

  VectorOfUInts Ports;
  VectorOfUInts PreviousPorts;
  VectorOfUInts UncheckedPorts;

  std::map<void*, unsigned long> ObserverIds;

private:
  void UpdateProducerConsumerLinks()
  {
    if (this->Self == nullptr || this->Self->GetParent() == nullptr ||
      this->PreviousProxies == this->Proxies)
    {
      return;
    }

    vtkSMProxyProperty* self = this->Self;

    // create sets for each.
    typedef std::set<vtkSMProxy*> proxy_set;
    proxy_set previous_proxies(this->PreviousProxies.begin(), this->PreviousProxies.end());
    proxy_set proxies(this->Proxies.begin(), this->Proxies.end());

    proxy_set removed;
    std::set_difference(previous_proxies.begin(), previous_proxies.end(), proxies.begin(),
      proxies.end(), std::insert_iterator<proxy_set>(removed, removed.begin()));
    for (proxy_set::iterator iter = removed.begin(); iter != removed.end(); ++iter)
    {
      vtkSMProxy* producer = (*iter);
      if (producer)
      {
        // remove observer.
        producer->RemoveObserver(this->ObserverIds[producer]);
        this->ObserverIds.erase(producer);

        // break producer/consumer link.
        producer->RemoveConsumer(self, self->GetParent());
        self->GetParent()->RemoveProducer(self, producer);
      }
    }
    removed.clear();

    proxy_set added;
    std::set_difference(proxies.begin(), proxies.end(), previous_proxies.begin(),
      previous_proxies.end(), std::insert_iterator<proxy_set>(added, added.begin()));
    for (proxy_set::iterator iter = added.begin(); iter != added.end(); ++iter)
    {
      vtkSMProxy* producer = (*iter);
      if (producer)
      {
        // observe UpdateDataEvent, so we can update dependent domains when the
        // data changes.
        this->ObserverIds[producer] = producer->AddObserver(
          vtkCommand::UpdateDataEvent, self, &vtkSMProxyProperty::OnUpdateDataEvent);

        // add producer/consumer link.
        producer->AddConsumer(self, self->GetParent());
        self->GetParent()->AddProducer(self, producer);
      }
    }
    this->PreviousProxies = this->Proxies;
  }

  bool CheckedValueChanged()
  {
    this->ClearUnchecked();
    this->UpdateProducerConsumerLinks();
    return true;
  }

public:
  vtkPPInternals(vtkSMProxyProperty* self)
    : Self(self)
  {
  }

  //====================================================================================
  // ** All methods return true if the "datastructure" was modified, false otherwise
  // ** The key thing to keep in mind is whenever the "checked" value is
  // ** changed, "unchecked" value must be cleared and the producer/consumer
  // ** links must be corrected.
  //====================================================================================
  // Add a proxy/port to the unchecked list. Returns true if value modified.
  bool AddUnchecked(vtkSMProxy* proxy, unsigned int port = 0)
  {
    this->UncheckedProxies.push_back(proxy);
    this->UncheckedPorts.push_back(port);
    return true;
  }
  //------------------------------------------------------------------------------------
  // Add a proxy/port to the list and clear any unchecked values. Returns true if value modified.
  bool Add(vtkSMProxy* proxy, unsigned int port = 0)
  {
    this->Proxies.push_back(proxy);
    this->Ports.push_back(port);
    return this->CheckedValueChanged();
  }
  //------------------------------------------------------------------------------------
  bool Remove(vtkSMProxy* proxy)
  {
    SmartVectorOfProxies::iterator iter = this->Proxies.begin();
    VectorOfUInts::iterator iter2 = this->Ports.begin();
    for (; iter != this->Proxies.end() && iter2 != this->Ports.end(); ++iter, ++iter2)
    {
      if (iter->GetPointer() == proxy)
      {
        this->Proxies.erase(iter);
        this->Ports.erase(iter2);
        return this->CheckedValueChanged();
      }
    }
    return false;
  }

  //------------------------------------------------------------------------------------
  bool Resize(unsigned int count)
  {
    if (this->Proxies.size() != count)
    {
      this->Proxies.resize(count);
      this->Ports.resize(count);
      return this->CheckedValueChanged();
    }
    return false;
  }
  //------------------------------------------------------------------------------------
  bool ResizeUnchecked(unsigned int count)
  {
    if (this->UncheckedProxies.size() != count)
    {
      this->UncheckedProxies.resize(count);
      this->UncheckedPorts.resize(count);
      return true;
    }
    return false;
  }
  //------------------------------------------------------------------------------------
  bool Clear()
  {
    if (this->Proxies.size() > 0)
    {
      this->Proxies.clear();
      this->Ports.clear();
      return this->CheckedValueChanged();
    }
    return false;
  }
  //------------------------------------------------------------------------------------
  bool ClearUnchecked()
  {
    if (this->UncheckedProxies.size() > 0)
    {
      this->UncheckedProxies.clear();
      this->UncheckedPorts.clear();
      return true;
    }
    return false;
  }

  //------------------------------------------------------------------------------------
  bool Set(unsigned int index, vtkSMProxy* proxy, unsigned int port = 0)
  {
    if (static_cast<unsigned int>(this->Proxies.size()) > index && this->Proxies[index] == proxy &&
      this->Ports[index] == port)
    {
      return false;
    }
    if (static_cast<unsigned int>(this->Proxies.size()) <= index)
    {
      this->Proxies.resize(index + 1);
      this->Ports.resize(index + 1);
    }
    this->Proxies[index] = proxy;
    this->Ports[index] = port;
    return this->CheckedValueChanged();
  }
  //------------------------------------------------------------------------------------
  bool SetUnchecked(unsigned int index, vtkSMProxy* proxy, unsigned int port = 0)
  {
    if (static_cast<unsigned int>(this->UncheckedProxies.size()) > index &&
      this->UncheckedProxies[index] == proxy && this->UncheckedPorts[index] == port)
    {
      return false;
    }
    if (static_cast<unsigned int>(this->UncheckedProxies.size()) <= index)
    {
      this->UncheckedProxies.resize(index + 1);
      this->UncheckedPorts.resize(index + 1);
    }
    this->UncheckedProxies[index] = proxy;
    this->UncheckedPorts[index] = port;
    return true;
  }

  //------------------------------------------------------------------------------------
  bool Set(unsigned int count, vtkSMProxy** proxies, const unsigned int* ports = nullptr)
  {
    SmartVectorOfProxies newValues(proxies, proxies + count);
    VectorOfUInts newPorts;
    if (ports)
    {
      newPorts.insert(newPorts.end(), ports, ports + count);
    }
    else
    {
      newPorts.resize(count);
    }
    if (this->Proxies != newValues || this->Ports != newPorts)
    {
      this->Proxies = newValues;
      this->Ports = newPorts;
      return this->CheckedValueChanged();
    }
    return false;
  }

  //------------------------------------------------------------------------------------
  bool SetProxies(const SmartVectorOfProxies& otherProxies, const VectorOfUInts& otherPorts)
  {
    if (this->Proxies != otherProxies || this->Ports != otherPorts)
    {
      this->Proxies = otherProxies;
      this->Ports = otherPorts;
      return this->CheckedValueChanged();
    }
    return false;
  }

  //------------------------------------------------------------------------------------
  bool SetUncheckedProxies(const WeakVectorOfProxies& otherProxies, const VectorOfUInts& otherPorts)
  {
    if (this->UncheckedProxies != otherProxies || this->UncheckedPorts != otherPorts)
    {
      this->UncheckedProxies = otherProxies;
      this->UncheckedPorts = otherPorts;
      return true;
    }
    return false;
  }

  //------------------------------------------------------------------------------------
  // Get methods.
  //------------------------------------------------------------------------------------
  bool IsAdded(vtkSMProxy* proxy)
  {
    SmartVectorOfProxies::iterator iter =
      std::find(this->Proxies.begin(), this->Proxies.end(), proxy);
    return (iter != this->Proxies.end());
  }
  //------------------------------------------------------------------------------------
  const SmartVectorOfProxies& GetProxies() const { return this->Proxies; }
  const WeakVectorOfProxies& GetUncheckedProxies() const { return this->UncheckedProxies; }
  const VectorOfUInts& GetPorts() const { return this->Ports; }
  const VectorOfUInts& GetUncheckedPorts() const { return this->UncheckedPorts; }

  //------------------------------------------------------------------------------------
  vtkSMProxy* Get(unsigned int index) const
  {
    return (index < static_cast<unsigned int>(this->Proxies.size())
        ? this->Proxies[index].GetPointer()
        : nullptr);
  }
  //------------------------------------------------------------------------------------
  vtkSMProxy* GetUnchecked(unsigned int index) const
  {
    if (this->UncheckedProxies.size() > 0)
    {
      return (index < static_cast<unsigned int>(this->UncheckedProxies.size())
          ? this->UncheckedProxies[index].GetPointer()
          : nullptr);
    }
    return this->Get(index);
  }

  unsigned int GetPort(unsigned int index) const
  {
    return (index < static_cast<unsigned int>(this->Ports.size()) ? this->Ports[index] : 0);
  }
  unsigned int GetUncheckedPort(unsigned int index) const
  {
    if (this->UncheckedPorts.size() > 0)
    {
      return (index < static_cast<unsigned int>(this->UncheckedPorts.size())
          ? this->UncheckedPorts[index]
          : 0);
    }
    return this->GetPort(index);
  }

  //------------------------------------------------------------------------------------
  // Serialization methods.
  //------------------------------------------------------------------------------------
  bool WriteTo(paraview_protobuf::Variant* variant) const
  {
    variant->set_type(Variant::INPUT); // or PROXY?
    for (SmartVectorOfProxies::const_iterator iter = this->Proxies.begin();
         iter != this->Proxies.end(); ++iter)
    {
      if (vtkSMProxy* proxy = iter->GetPointer())
      {
        proxy->CreateVTKObjects();
        variant->add_proxy_global_id(proxy->GetGlobalID());
      }
      else
      {
        variant->add_proxy_global_id(0);
      }
    }
    for (VectorOfUInts::const_iterator iter = this->Ports.begin(); iter != this->Ports.end();
         ++iter)
    {
      variant->add_port_number(*iter);
    }
    return true;
  }

  bool ReadFrom(const paraview_protobuf::Variant& variant, vtkSMProxyLocator* locator)
  {
    SmartVectorOfProxies proxies;
    VectorOfUInts ports;
    assert(variant.proxy_global_id_size() == variant.port_number_size());
    for (int cc = 0, max = variant.proxy_global_id_size(); cc < max; cc++)
    {
      vtkTypeUInt32 gid = variant.proxy_global_id(cc);
      unsigned int port = variant.port_number(cc);

      // either ask the locator for the proxy, or find an existing one.
      vtkSMProxy* proxy = nullptr;
      if (locator && vtkSMProxyProperty::CanCreateProxy())
      {
        proxy = locator->LocateProxy(gid);
      }
      else
      {
        proxy =
          vtkSMProxy::SafeDownCast(this->Self->GetParent()->GetSession()->GetRemoteObject(gid));
      }
      if (proxy != nullptr || gid == 0)
      {
        proxies.push_back(proxy);
        ports.push_back(port);
      }
    }
    return this->SetProxies(proxies, ports);
  }
};
#endif
// VTK-HeaderTest-Exclude: vtkSMProxyPropertyInternals.h
