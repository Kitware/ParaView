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

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStateLocator.h"
#include "vtkSMSession.h"
#include "vtkStdString.h"

#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/iterator>

#include <assert.h>

vtkStandardNewMacro(vtkSMProxyProperty);

class vtkSMProxyProperty::vtkProxyPointer
{
public:
  vtkSmartPointer<vtkSMProxy> Value;
  vtkSMProxyProperty* Self;
  vtkProxyPointer()
    {
    this->Self = NULL;
    this->Value = NULL;
    }

  vtkProxyPointer(vtkSMProxyProperty* self, vtkSMProxy* value)
    {
    this->Self = self;
    this->Value = value;
    if (value && self)
      {
      self->AddProducer(this->Value);
      }
    }

  ~vtkProxyPointer()
    {
    if (this->Self && this->Value)
      {
      this->Self->RemoveProducer(this->Value);
      }
    }

  vtkProxyPointer(const vtkProxyPointer& other)
    {
    this->Self = other.Self;
    this->Value = other.Value;
    if (this->Self && this->Value)
      {
      this->Self->AddProducer(this->Value);
      }
    }

  vtkProxyPointer& operator=(const vtkProxyPointer& other)
    {
    if (this->Self && this->Value)
      {
      this->Self->RemoveProducer(this->Value);
      }
    this->Self = other.Self;
    this->Value = other.Value;
    if (this->Self && this->Value)
      {
      this->Self->AddProducer(this->Value);
      }
    return *this;
    }

  bool operator==(const vtkProxyPointer& other) const
    {
    return (this->Self == other.Self && this->Value == other.Value);
    }
};

struct vtkSMProxyPropertyInternals
{
  typedef vtkstd::vector<vtkSMProxyProperty::vtkProxyPointer> VectorOfProxies;

  void PrintProxy()
    {
    cout << "ProxyList: ";
    VectorOfProxies::iterator iter;
    for(iter = Proxies.begin(); iter != Proxies.end(); iter++)
      {
      cout << " " << (iter->Value->HasGlobalID() ? iter->Value->GetGlobalID() : 0);
      }
    cout << endl;
    }

  VectorOfProxies Proxies;
  vtkstd::vector<vtkSMProxy*> UncheckedProxies;
  vtkstd::map<void*, int> ProducerCounts;
};

//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->PPInternals = new vtkSMProxyPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  // Ensure that this happens before we delete this->PPInternals. This is needed
  // because when items in this vector are being destroyed, they call
  // vtkSMProxyProperty::RemoveProducer(). That function uses this->PPInternals
  // to update producer counts.
  this->PPInternals->Proxies.clear();

  // We use this indirection to detect any other issues we may run into when
  // destroying the vtkSMProxyPropertyInternals instance more
  // cleanly (since we'll be accessing a NULL ptr).
  vtkSMProxyPropertyInternals* temp = this->PPInternals;
  this->PPInternals = NULL;
  delete temp;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddUncheckedProxy(vtkSMProxy* proxy)
{
  this->PPInternals->UncheckedProxies.push_back(proxy);
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::RemoveUncheckedProxy(vtkSMProxy* proxy)
{
  vtkstd::vector<vtkSMProxy* >::iterator it =
    this->PPInternals->UncheckedProxies.begin();
  unsigned int idx = 0;
  for (; 
       it != this->PPInternals->UncheckedProxies.end(); 
       it++, idx++)
    {
    if (*it == proxy)
      {
      this->PPInternals->UncheckedProxies.erase(it);
      break;
      }
    }
  return idx;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy)
{
  if (this->PPInternals->UncheckedProxies.size() <= idx)
    {
    this->PPInternals->UncheckedProxies.resize(idx+1);
    }
  this->PPInternals->UncheckedProxies[idx] = proxy;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllUncheckedProxies()
{
  this->PPInternals->UncheckedProxies.clear();
}

//---------------------------------------------------------------------------
bool vtkSMProxyProperty::IsProxyAdded(vtkSMProxy* proxy)
{
  vtkSMProxyPropertyInternals::VectorOfProxies::iterator iter =
    this->PPInternals->Proxies.begin();
  for ( ; iter != this->PPInternals->Proxies.end() ; ++iter)
    {
    if (iter->Value == proxy)
      {
      return true;
      }
    }
  return false;
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy, int modify)
{
  this->RemoveAllUncheckedProxies();

  this->PPInternals->Proxies.push_back(vtkProxyPointer(this, proxy));
  if (modify)
    {
    this->Modified();
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveProxy(vtkSMProxy* proxy)
{
  this->RemoveProxy(proxy, 1);
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::RemoveProxy(vtkSMProxy* proxy, int modify)
{
  vtkSMProxyPropertyInternals::VectorOfProxies::iterator iter =
    this->PPInternals->Proxies.begin();
  unsigned int idx = 0;
  for ( ; iter != this->PPInternals->Proxies.end() ; ++iter, idx++)
    {
    if (iter->Value == proxy)
      {
      this->PPInternals->Proxies.erase(iter);
      if (modify)
        {
        this->Modified();
        }
      break;
      }
    }
  return idx;
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::SetProxy(unsigned int idx, vtkSMProxy* proxy)
{
  if (this->PPInternals->Proxies.size() > idx &&
      proxy == this->PPInternals->Proxies[idx].Value)
    {
    return 1;
    }

  this->RemoveAllUncheckedProxies();
  if (this->PPInternals->Proxies.size() <= idx)
    {
    this->PPInternals->Proxies.resize(idx+1);
    }

  this->PPInternals->Proxies[idx] = vtkProxyPointer(this, proxy);
  this->Modified();

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetProxies(unsigned int numProxies,
  vtkSMProxy* proxies[])
{
  this->RemoveAllUncheckedProxies();

  this->PPInternals->Proxies.clear();
  for (unsigned int cc=0; cc < numProxies; cc++)
    {
    this->PPInternals->Proxies.push_back(vtkProxyPointer(this, proxies[cc]));
    }

  this->Modified();
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy)
{
  return this->AddProxy(proxy, 1);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllProxies(int modify)
{
  this->PPInternals->Proxies.clear();
  if (modify)
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetNumberOfProxies(unsigned int num)
{
  if (num != 0)
    {
    this->PPInternals->Proxies.resize(num);
    }
  else
    {
    this->PPInternals->Proxies.clear();
    }
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfProxies()
{
  return this->PPInternals->Proxies.size();
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfUncheckedProxies()
{
  return this->PPInternals->UncheckedProxies.size();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetProxy(unsigned int idx)
{
  return this->PPInternals->Proxies[idx].Value.GetPointer();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetUncheckedProxy(unsigned int idx)
{
  return this->PPInternals->UncheckedProxies[idx];
}


//---------------------------------------------------------------------------
void vtkSMProxyProperty::WriteTo(vtkSMMessage* message)
{
  ProxyState_Property *prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant *var = prop->mutable_value();
  var->set_type(Variant::PROXY);
  for (unsigned int i=0; i<this->GetNumberOfProxies(); i++)
    {
    vtkSMProxy* argument = this->GetProxy(i);
    if (argument)
      {
      argument->CreateVTKObjects();
      var->add_proxy_global_id(argument->GetGlobalID());
      }
    else
      {
      var->add_proxy_global_id(0);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::ReadFrom(const vtkSMMessage* message, int msg_offset)
{
  // FIXME this method is REALLY close to its vtkSMInputProperty subClass
  // Please keep them in sync
  const ProxyState_Property *prop = &message->GetExtension(ProxyState::property, msg_offset);
  if(strcmp(prop->name().c_str(), this->GetXMLName()) == 0)
    {
    const Variant *value = &prop->value();
    int nbProxies = value->proxy_global_id_size();
    vtkstd::set<vtkTypeUInt32> newProxyIdList;
    vtkstd::set<vtkTypeUInt32>::const_iterator proxyIdIter;

    // Fill indexed proxy id list
    for(int i=0; i < nbProxies; i++)
      {
      newProxyIdList.insert(value->proxy_global_id(i));
      }

    // Deal with existing proxy
    for(unsigned int i=0;i<this->GetNumberOfProxies();i++)
      {
      vtkSMProxy *proxy = this->GetProxy(i);
      vtkTypeUInt32 id = proxy->GetGlobalID();
      if((proxyIdIter=newProxyIdList.find(id)) == newProxyIdList.end())
        {
        // Not find => Need to be removed
        this->RemoveProxy(proxy, true);
        i--; // Make sure we don't skip a proxy in the analysis
        }
      else
        {
        // Already there, no need to add it twice
        newProxyIdList.erase(id);
        }
      }

    // Managed real new proxy
    for( proxyIdIter =  newProxyIdList.begin();
         proxyIdIter != newProxyIdList.end();
         proxyIdIter++ )
      {
      // Get the proxy from proxy manager
      vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
      vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(
          pxm->GetSession()->GetRemoteObject(*proxyIdIter));
      if(proxy)
        {
        this->AddProxy(proxy, true);
        }
      // We shouldn't ReNew a Proxy if that Input was not found
      // else
      //   {
      //    // Recreate the proxy as it used to be
      //    proxy = pxm->ReNewProxy(*proxyIdIter, locator);
      //    if(proxy)
      //      {
      //       this->AddProxy(proxy, true);
      //       proxy->Delete();
      //      }
      //    }
      }
    }
  else
    {
    vtkWarningMacro("Invalid offset property");
    }
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                          vtkPVXMLElement* element)
{
  return this->Superclass::ReadXMLAttributes(parent, element);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::DeepCopy(vtkSMProperty* src, 
  const char* exceptionClass, int proxyPropertyCopyFlag)
{
  vtkSMProxyManager* pxm = this->GetParent()->GetProxyManager();
  vtkSMProxyProperty* dsrc = vtkSMProxyProperty::SafeDownCast(src);

  this->RemoveAllProxies();
  this->RemoveAllUncheckedProxies();
  
  if (dsrc)
    {
    int imUpdate = this->ImmediateUpdate;
    this->ImmediateUpdate = 0;
    unsigned int i;
    unsigned int numElems = dsrc->GetNumberOfProxies();

    for(i=0; i<numElems; i++)
      {
      vtkSMProxy* psrc = dsrc->GetProxy(i);
      vtkSMProxy* pdest = pxm->NewProxy(psrc->GetXMLGroup(), 
        psrc->GetXMLName());
      pdest->SetSession(psrc->GetSession());
      pdest->Copy(psrc, exceptionClass, proxyPropertyCopyFlag);
      this->AddProxy(pdest);
      pdest->Delete();
      }
    
    numElems = dsrc->GetNumberOfUncheckedProxies();
    for(i=0; i<numElems; i++)
      {
      vtkSMProxy* psrc = dsrc->GetUncheckedProxy(i);
      vtkSMProxy* pdest = pxm->NewProxy(psrc->GetXMLGroup(), 
        psrc->GetXMLName());
      pdest->SetSession(psrc->GetSession());
      pdest->Copy(psrc, exceptionClass, proxyPropertyCopyFlag);
      this->AddUncheckedProxy(pdest);
      pdest->Delete();
      }
    this->ImmediateUpdate = imUpdate;
    }
  if (this->ImmediateUpdate)
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  this->RemoveAllProxies();
  this->RemoveAllUncheckedProxies();

  vtkSMProxyProperty* dsrc = vtkSMProxyProperty::SafeDownCast(
    src);
  if (dsrc)
    {
    int imUpdate = this->ImmediateUpdate;
    this->ImmediateUpdate = 0;
    unsigned int i;
    unsigned int numElems = dsrc->GetNumberOfProxies();
    for(i=0; i<numElems; i++)
      {
      this->AddProxy(dsrc->GetProxy(i));
      }
    numElems = dsrc->GetNumberOfUncheckedProxies();
    for(i=0; i<numElems; i++)
      {
      this->AddUncheckedProxy(dsrc->GetUncheckedProxy(i));
      }
    this->ImmediateUpdate = imUpdate;
    }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddProducer(vtkSMProxy* producer)
{
  if (producer && this->GetParent())
    {
    this->PPInternals->ProducerCounts[producer]++;
    if (this->PPInternals->ProducerCounts[producer] == 1)
      {
      producer->AddConsumer(this, this->GetParent());
      this->GetParent()->AddProducer(this, producer);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveProducer(vtkSMProxy* producer)
{
  if (producer && this->GetParent())
    {
    this->PPInternals->ProducerCounts[producer]--;
    assert(this->PPInternals->ProducerCounts[producer] >= 0);
    if (this->PPInternals->ProducerCounts[producer] == 0)
      {
      producer->RemoveConsumer(this, this->GetParent());
      this->GetParent()->RemoveProducer(this, producer);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Values: ";
  for (unsigned int i=0; i<this->GetNumberOfProxies(); i++)
    {
    os << this->GetProxy(i) << " ";
    }
  os << endl;
}
//---------------------------------------------------------------------------
void vtkSMProxyProperty::SaveStateValues(vtkPVXMLElement* propertyElement)
{
  unsigned int size = this->GetNumberOfProxies();
  if (size > 0)
    {
    propertyElement->AddAttribute("number_of_elements", size);
    }
  for (unsigned int i=0; i<size; i++)
    {
    this->AddProxyElementState(propertyElement, i);
    }
}
//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyProperty::AddProxyElementState(vtkPVXMLElement *prop,
                                                          unsigned int idx)
{
  vtkSMProxy* proxy = this->GetProxy(idx);
  vtkPVXMLElement* proxyElement = 0;
  if (proxy)
    {
    proxyElement = vtkPVXMLElement::New();
    proxyElement->SetName("Proxy");
    proxyElement->AddAttribute("value",
                               static_cast<unsigned int>(proxy->GetGlobalID()));
    prop->AddNestedElement(proxyElement);
    proxyElement->FastDelete();
    }
  return proxyElement;
}
//---------------------------------------------------------------------------
// NOTE: This method is duplicated in some way in vtkSMInputProperty
// Therefore, care must be taken to keep the two in sync.
int vtkSMProxyProperty::LoadState(vtkPVXMLElement* element,
                                  vtkSMProxyLocator* loader)
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
  int clear=1;
  element->GetScalarAttribute("clear", &clear);
  if (clear)
    {
    this->PPInternals->Proxies.clear();
    }

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() &&
        (strcmp(currentElement->GetName(), "Element") == 0 ||
         strcmp(currentElement->GetName(), "Proxy") == 0) )
      {
      int id;
      if (currentElement->GetScalarAttribute("value", &id))
        {
        if (id)
          {
          vtkSMProxy* proxy = loader->LocateProxy(id);
          if (proxy)
            {
            this->AddProxy(proxy, 0);
            }
          else
            {
            // It is not an error to have missing proxies in a proxy property.
            // We simply ignore such proxies.
            //vtkErrorMacro("Could not create proxy of id: " << id);
            //return 0;
            }
          }
        else
          {
          this->AddProxy(0, 0);
          }
        }
      }
    }

  // Do not immediately update. Leave it to the loader.
  this->Modified();
  this->ImmediateUpdate = prevImUpdate;
  return 1;
}
