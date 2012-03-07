/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMStateLocator.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <iterator>

#include <assert.h>

vtkStandardNewMacro(vtkSMInputProperty);


struct vtkSMInputPropertyInternals
{
  std::vector<unsigned int> OutputPorts;
  std::vector<unsigned int> UncheckedOutputPorts;
};

//---------------------------------------------------------------------------
vtkSMInputProperty::vtkSMInputProperty()
{
  this->MultipleInput = 0;
  this->PortIndex = 0;

  this->IPInternals = new vtkSMInputPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMInputProperty::~vtkSMInputProperty()
{
  delete this->IPInternals;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::WriteTo(vtkSMMessage* message)
{
  ProxyState_Property *prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant *var = prop->mutable_value();
  var->set_type(Variant::INPUT);
  for (unsigned int i=0; i<this->GetNumberOfProxies(); i++)
    {
    vtkSMProxy* argument = this->GetProxy(i);
    if (argument)
      {
      argument->CreateVTKObjects();
      var->add_proxy_global_id(argument->GetGlobalID());
      var->add_port_number(this->GetOutputPortForConnection(i));
      }
    else
      {
      var->add_proxy_global_id(0);
      var->add_port_number(0);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::ReadFrom(const vtkSMMessage* message, int msg_offset,
                                  vtkSMProxyLocator* locator)
{
  // --------------------------------------------------------------------------
  // WARNING: this method is REALLY close to its superclass: Please keep them
  //          in sync.
  // --------------------------------------------------------------------------
  const ProxyState_Property *prop =
      &message->GetExtension(ProxyState::property, msg_offset);
  if(strcmp(prop->name().c_str(), this->GetXMLName()) == 0)
    {
    const Variant *value = &prop->value();
    assert(value->proxy_global_id_size() == value->port_number_size());
    int nbProxies = value->proxy_global_id_size();
    std::set<vtkTypeUInt32> newProxyIdList;
    std::set<vtkTypeUInt32>::iterator proxyIdIter;
    std::map<vtkTypeUInt32,int> proxyIdPortMap;

    // Fill indexed proxy id list
    for(int i=0;i<nbProxies;i++)
      {
      proxyIdPortMap[value->proxy_global_id(i)] = value->port_number(i);
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
        newProxyIdList.erase(proxyIdIter);
        }
      }

    // Managed real new proxy
    for(proxyIdIter=newProxyIdList.begin();
        proxyIdIter != newProxyIdList.end();
        proxyIdIter++)
      {
      // Get the proxy from proxy manager
      vtkSMProxy* proxy;
      if(locator && vtkSMProxyProperty::CanCreateProxy())
        {
        proxy = locator->LocateProxy(*proxyIdIter);
        }
      else
        {
        proxy =
            vtkSMProxy::SafeDownCast(
              this->GetParent()->GetSession()->GetRemoteObject(*proxyIdIter));
        }

      if(proxy)
        {
        this->AddInputConnection(proxy, proxyIdPortMap[*proxyIdIter], true);
        }
      // We shouldn't ReNew an Input if that Input was not found
      //  else
      //    {
      //     // Recreate the proxy as it used to be
      //     proxy = pxm->ReNewProxy(*proxyIdIter, locator);
      //     if(proxy)
      //       {
      //       this->AddInputConnection(proxy, proxyIdPortMap[*proxyIdIter], true);
      //       proxy->Delete();
      //       }
      //     }
      }
    }
  else
    {
    vtkWarningMacro("Invalid offset property");
    }
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                          vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(parent, element);

  int multiple_input;
  int retVal = element->GetScalarAttribute("multiple_input", &multiple_input);
  if(retVal) 
    { 
    this->SetMultipleInput(multiple_input); 
    this->Repeatable = multiple_input;
    }

  int port_index;
  retVal = element->GetScalarAttribute("port_index", &port_index);
  if(retVal) 
    { 
    this->SetPortIndex(port_index); 
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultipleInput: " << this->MultipleInput << endl;
  os << indent << "PortIndex: " << this->PortIndex << endl;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetNumberOfProxies(unsigned int num)
{
  if (num != 0)
    {
    this->IPInternals->OutputPorts.resize(num);
    }
  else
    {
    this->IPInternals->OutputPorts.clear();
    }

  this->Superclass::SetNumberOfProxies(num);
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetNumberOfUncheckedProxies(unsigned int num)
{
  if (num != 0)
    {
    this->IPInternals->UncheckedOutputPorts.resize(num);
    }
  else
    {
    this->IPInternals->UncheckedOutputPorts.clear();
    }

  this->Superclass::SetNumberOfUncheckedProxies(num);
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetProxies(unsigned int numProxies,
  vtkSMProxy* proxies[], unsigned int outputports[])
{
  this->IPInternals->OutputPorts.clear();
  for (unsigned int cc=0; cc < numProxies; cc++)
    {
    this->IPInternals->OutputPorts.push_back(outputports[cc]);
    }

  this->Superclass::SetProxies(numProxies, proxies);
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::AddInputConnection(vtkSMProxy* proxy, 
                                           unsigned int outputPort,
                                           int modify)
{
  if (this->IPInternals->OutputPorts.size() != 
      this->GetNumberOfProxies())
    {
    this->IPInternals->OutputPorts.resize(
      this->GetNumberOfProxies());
    }
  this->IPInternals->OutputPorts.push_back(outputPort);

  int retval = this->AddProxy(proxy, modify);
  if (retval)
    {
    if (modify)
      {
      this->Modified();
      }
    return retval;
    }
  else
    {
    return 0;
    }
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::SetInputConnection(unsigned int idx, 
                                           vtkSMProxy* proxy, 
                                           unsigned int outputPort)
{
  if (idx >= this->IPInternals->OutputPorts.size())
    {
    this->IPInternals->OutputPorts.resize(idx+1);
    }
  this->IPInternals->OutputPorts[idx] = outputPort;

  int retval = this->SetProxy(idx, proxy);
  if (retval)
    {
    return retval;
    }
  else
    {
    return 0;
    }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AddUncheckedInputConnection(vtkSMProxy* proxy, 
                                                     unsigned int outputPort)
{
  if (this->IPInternals->UncheckedOutputPorts.size() != 
      this->GetNumberOfUncheckedProxies())
    {
    this->IPInternals->UncheckedOutputPorts.resize(
      this->GetNumberOfUncheckedProxies());
    }
  this->IPInternals->UncheckedOutputPorts.push_back(outputPort);

  this->AddUncheckedProxy(proxy);
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetUncheckedInputConnection(unsigned int idx, 
                                                    vtkSMProxy* proxy, 
                                                     unsigned int outputPort)
{
  if (idx >= this->IPInternals->UncheckedOutputPorts.size())
    {
    this->IPInternals->UncheckedOutputPorts.resize(idx+1);
    }
  this->IPInternals->UncheckedOutputPorts[idx] = outputPort;

  this->SetUncheckedProxy(idx, proxy);
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::RemoveProxy(vtkSMProxy* proxy, int modify)
{
  unsigned int idx =
    this->Superclass::RemoveProxy(proxy, modify);
  if (idx < this->IPInternals->OutputPorts.size())
    {
    this->IPInternals->OutputPorts.erase(
      this->IPInternals->OutputPorts.begin() + idx);
    }
  return idx;
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::RemoveUncheckedProxy(vtkSMProxy* proxy)
{
  unsigned int idx =
    this->Superclass::RemoveUncheckedProxy(proxy);
  if (idx < this->IPInternals->UncheckedOutputPorts.size())
    {
    this->IPInternals->UncheckedOutputPorts.erase(
      this->IPInternals->UncheckedOutputPorts.begin() + idx);
    }
  return idx;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::RemoveAllUncheckedProxies()
{
  this->IPInternals->UncheckedOutputPorts.clear();

  this->Superclass::RemoveAllUncheckedProxies();
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::ClearUncheckedProxies()
{
  this->IPInternals->UncheckedOutputPorts =
    this->IPInternals->OutputPorts;

  this->Superclass::ClearUncheckedProxies();
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::RemoveAllProxies(int modify)
{
  this->IPInternals->OutputPorts.clear();

  this->Superclass::RemoveAllProxies(modify);
  if (modify)
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetOutputPortForConnection(unsigned int idx)
{
  if (idx >= this->IPInternals->OutputPorts.size())
    {
    return 0;
    }
  return this->IPInternals->OutputPorts[idx];
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetUncheckedOutputPortForConnection(
  unsigned int idx)
{
  if (idx >= this->IPInternals->UncheckedOutputPorts.size())
    {
    return 0;
    }
  return this->IPInternals->UncheckedOutputPorts[idx];
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMInputProperty::AddProxyElementState(vtkPVXMLElement *prop,
                                                          unsigned int idx)
{
  vtkPVXMLElement* proxyElm = this->Superclass::AddProxyElementState(prop, idx);
  if (proxyElm)
    {
    proxyElm->AddAttribute("output_port",this->GetOutputPortForConnection(idx));
    }
  return proxyElm;
}
//---------------------------------------------------------------------------
int vtkSMInputProperty::LoadState(vtkPVXMLElement* element,
                                  vtkSMProxyLocator* loader)
{
  if (!loader)
    {
    // If no loader, leave state unchanged.
    return 1;
    }

  // NOTE: This method by-passes LoadState() of vtkSMProxyProperty and
  // re-implements a lot of it's functionality to add output ports.
  // Therefore, care must be taken to keep the two in sync.

  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  this->vtkSMProperty::LoadState(element, loader);

  // If "clear" is present and is 0, it implies that the proxy elements
  // currently in the property should not be cleared before loading
  // the new state.
  int clear=1;
  element->GetScalarAttribute("clear", &clear);
  if (clear)
    {
    this->RemoveAllProxies(0);
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
        int outputPort = 0;
        currentElement->GetScalarAttribute("output_port", &outputPort);
        if (id)
          {
          vtkSMProxy* proxy = loader->LocateProxy(id);
          if (proxy)
            {
            this->AddInputConnection(proxy, outputPort, 0);
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

//---------------------------------------------------------------------------
void vtkSMInputProperty::Copy(vtkSMProperty* src)
{
  int imUpdate = this->ImmediateUpdate;
  this->ImmediateUpdate = 0;

  this->Superclass::Copy(src);

  vtkSMInputProperty* dsrc = vtkSMInputProperty::SafeDownCast(src);
  if (dsrc)
    {
    this->IPInternals->OutputPorts = dsrc->IPInternals->OutputPorts;
    this->IPInternals->UncheckedOutputPorts =
      dsrc->IPInternals->UncheckedOutputPorts;
    }

  this->ImmediateUpdate = imUpdate;
  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::DeepCopy(vtkSMProperty* src, 
  const char* exceptionClass, int proxyPropertyCopyFlag)
{
  vtkSMInputProperty* dsrc = vtkSMInputProperty::SafeDownCast(src);
  int imUpdate = this->ImmediateUpdate;
  this->ImmediateUpdate = 0;

  this->Superclass::DeepCopy(src, exceptionClass, proxyPropertyCopyFlag);
  if (dsrc)
    {
    this->IPInternals->OutputPorts = dsrc->IPInternals->OutputPorts;
    this->IPInternals->UncheckedOutputPorts =
      dsrc->IPInternals->UncheckedOutputPorts;
    }

  this->ImmediateUpdate = imUpdate;
  if (this->ImmediateUpdate)
    {
    this->Modified();
    }
}
