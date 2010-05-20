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
#include "vtkSMDomainIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyLocator.h"
#include "vtkSmartPointer.h"

#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/iterator>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMProxyProperty);

struct vtkSMProxyPropertyInternals
{
  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > VectorOfProxies;
  
  VectorOfProxies Proxies;
  VectorOfProxies PreviousProxies;
  vtkstd::vector<vtkSMProxy*> UncheckedProxies;
};

//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->PPInternals = new vtkSMProxyPropertyInternals;
  this->CleanCommand = 0;
  this->RepeatCommand = 0;
  this->RemoveCommand = 0;
  this->IsInternal = 0;
  this->NullOnEmpty = 0;
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  delete this->PPInternals;
  this->SetCleanCommand(0);
  this->SetRemoveCommand(0);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::UpdateAllInputs()
{
  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int idx=0; idx < numProxies; idx++)
    {
    vtkSMProxy* proxy = this->GetProxy(idx);
    if (proxy)
      {
      proxy->UpdateSelfAndAllInputs();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AppendCommandToStreamWithRemoveCommand(
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->RemoveCommand || this->InformationOnly)
    {
    return;
    }

  vtkstd::set<vtkSmartPointer<vtkSMProxy> > prevProxies(
    this->PPInternals->PreviousProxies.begin(),
    this->PPInternals->PreviousProxies.end());
  vtkstd::set<vtkSmartPointer<vtkSMProxy> > curProxies(
    this->PPInternals->Proxies.begin(),
    this->PPInternals->Proxies.end());

  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > proxiesToRemove;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > proxiesToAdd;
 
  // Determine the proxies in the PreviousProxies but not in Proxies.
  // These are the proxies to remove.
  vtkstd::back_insert_iterator<
    vtkstd::vector<vtkSmartPointer<vtkSMProxy> > > ii_remove(proxiesToRemove);
  vtkstd::set_difference(prevProxies.begin(),
                         prevProxies.end(),
                         curProxies.begin(),
                         curProxies.end(),
                         ii_remove);
  
  // Determine the proxies in the Proxies but not in PreviousProxies.
  // These are the proxies to add.
  vtkstd::back_insert_iterator<
    vtkstd::vector<vtkSmartPointer<vtkSMProxy> > > ii_add(proxiesToAdd);
  vtkstd::set_difference(curProxies.begin(),
                         curProxies.end(),
                         prevProxies.begin(),
                         prevProxies.end(),
                         ii_add   );

  // Remove the proxies to remove.
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator iter1;
  for (iter1 = proxiesToRemove.begin(); iter1 != proxiesToRemove.end(); ++iter1)
    {
    vtkSMProxy* toAppend = iter1->GetPointer();
    this->AppendProxyToStream(toAppend, str, objectId, 1);
    toAppend->RemoveConsumer(this, cons);
    cons->RemoveProducer(this, toAppend);
    }

  // Add the proxies to add.
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator iter;
  iter  = proxiesToAdd.begin();
  for ( ; iter != proxiesToAdd.end(); ++iter)
    {
    vtkSMProxy *toAppend = iter->GetPointer();
    // Keep track of all proxies that point to this as a
    // consumer so that we can remove this from the consumer
    // list later if necessary.
    toAppend->AddConsumer(this, cons);
    cons->AddProducer(this, toAppend);
    this->AppendProxyToStream(toAppend, str, objectId, 0);
    }
 
  // Set PreviousProxies to match the current Proxies.
  // (which is same as PreviousProxies - proxiesToRemove + proxiesToAdd).
  this->PPInternals->PreviousProxies.clear();
  this->PPInternals->PreviousProxies.insert(
    this->PPInternals->PreviousProxies.begin(),
    this->PPInternals->Proxies.begin(), this->PPInternals->Proxies.end());
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AppendCommandToStream(
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->InformationOnly)
    {
    return;
    }

  if (this->RemoveCommand)
    {
    this->AppendCommandToStreamWithRemoveCommand(
      cons, str, objectId);
    }
  else // no remove command.
    {
    if (this->CleanCommand)
      {
      *str << vtkClientServerStream::Invoke
        << objectId << this->CleanCommand
        << vtkClientServerStream::End;
      }

    // Remove all consumers (using previous proxies)
    this->RemoveConsumerFromPreviousProxies(cons);
    // Remove all previous proxies before adding new ones.
    this->RemoveAllPreviousProxies();

    unsigned int numProxies = this->GetNumberOfProxies();
    for (unsigned int idx=0; idx < numProxies; idx++)
      {
      vtkSMProxy* proxy = this->GetProxy(idx);
      // Keep track of all proxies that point to this as a
      // consumer so that we can remove this from the consumer
      // list later if necessary
      this->AddPreviousProxy(proxy);
      if (proxy)
        {
        proxy->AddConsumer(this, cons);
        cons->AddProducer(this, proxy);
        }
      this->AppendProxyToStream(proxy, str, objectId, 0);
      }

    // When no proxies are present in the property, we call the command on
    // the server side object with NULL argument.
    if (numProxies == 0 && !this->CleanCommand && this->NullOnEmpty)
      {
      this->AppendProxyToStream(NULL, str, objectId, 0);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AppendProxyToStream(vtkSMProxy* toAppend, 
                                             vtkClientServerStream* str, 
                                             vtkClientServerID objectId,
                                             int remove /*=0*/)
{
  const char* command = (remove)? this->RemoveCommand : this->Command;
  if (!command)
    {
    vtkErrorMacro("Command not specified!"); //sanity check.
    return;
    }

  if (!toAppend)
    {
    vtkClientServerID nullID;
    *str << vtkClientServerStream::Invoke << objectId << command 
      << nullID << vtkClientServerStream::End;
    return;
    }

  if (this->UpdateSelf)
    {
    *str << vtkClientServerStream::Invoke << objectId << command 
      << toAppend << vtkClientServerStream::End;
    return;
    }

  toAppend->CreateVTKObjects();
  
  *str << vtkClientServerStream::Invoke << objectId << command 
       << toAppend->GetID()
       << vtkClientServerStream::End;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllPreviousProxies()
{
  this->PPInternals->PreviousProxies.clear();
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddPreviousProxy(vtkSMProxy* proxy)
{
  this->PPInternals->PreviousProxies.push_back(proxy);
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyProperty::GetNumberOfPreviousProxies()
{
  return this->PPInternals->PreviousProxies.size();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetPreviousProxy(unsigned int idx)
{
  if (idx >= this->PPInternals->PreviousProxies.size())
    {
    return 0;
    }
  return this->PPInternals->PreviousProxies[idx];
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveConsumerFromPreviousProxies(vtkSMProxy* cons)
{
  vtkSMProxyPropertyInternals::VectorOfProxies& prevProxies =
    this->PPInternals->PreviousProxies;

  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator it =
    prevProxies.begin();
  for(; it != prevProxies.end(); it++)
    {
    if (it->GetPointer())
      {
      it->GetPointer()->RemoveConsumer(this, cons);
      cons->RemoveProducer(this, it->GetPointer());
      }
    }
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
  this->PPInternals->UncheckedProxies.erase(
    this->PPInternals->UncheckedProxies.begin(),
    this->PPInternals->UncheckedProxies.end());
}

//---------------------------------------------------------------------------
bool vtkSMProxyProperty::IsProxyAdded(vtkSMProxy* proxy)
{
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator iter =   
    this->PPInternals->Proxies.begin();
  for ( ; iter != this->PPInternals->Proxies.end() ; ++iter)
    {
    if (*iter == proxy)
      {
      return true;
      }
    }
  return false;
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy, int modify)
{
  if ( vtkSMProperty::GetCheckDomains() )
    {
    this->RemoveAllUncheckedProxies();
    this->AddUncheckedProxy(proxy);
    
    if (!this->IsInDomains())
      {
      this->RemoveAllUncheckedProxies();
      return 0;
      }
    }
  this->RemoveAllUncheckedProxies();

  this->PPInternals->Proxies.push_back(proxy);
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
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator iter =   
    this->PPInternals->Proxies.begin();
  unsigned int idx = 0;
  for ( ; iter != this->PPInternals->Proxies.end() ; ++iter, idx++)
    {
    if (*iter == proxy)
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
      proxy == this->PPInternals->Proxies[idx])
    {
    return 1;
    }

  if ( vtkSMProperty::GetCheckDomains() )
    {
    this->SetUncheckedProxy(idx, proxy);
    
    if (!this->IsInDomains())
      {
      this->RemoveAllUncheckedProxies();
      return 0;
      }
    }
  this->RemoveAllUncheckedProxies();
  if (this->PPInternals->Proxies.size() <= idx)
    {
    this->PPInternals->Proxies.resize(idx+1);
    }

  this->PPInternals->Proxies[idx] = proxy;
  this->Modified();

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetProxies(unsigned int numProxies,
  vtkSMProxy* proxies[])
{
  if ( vtkSMProperty::GetCheckDomains() )
    {
    this->RemoveAllUncheckedProxies();
    for (unsigned int cc=0; cc < numProxies; cc++)
      {
      this->PPInternals->UncheckedProxies.push_back(proxies[cc]);
      }
    
    if (!this->IsInDomains())
      {
      this->RemoveAllUncheckedProxies();
      return;
      }
    }
  this->RemoveAllUncheckedProxies();

  this->PPInternals->Proxies.clear();
  for (unsigned int cc=0; cc < numProxies; cc++)
    {
    this->PPInternals->Proxies.push_back(proxies[cc]);
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
  return this->PPInternals->Proxies[idx].GetPointer();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyProperty::GetUncheckedProxy(unsigned int idx)
{
  return this->PPInternals->UncheckedProxies[idx];
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                          vtkPVXMLElement* element)
{
  int ret = this->Superclass::ReadXMLAttributes(parent, element);
  
  const char* clean_command = element->GetAttribute("clean_command");
  if(clean_command) 
    { 
    this->SetCleanCommand(clean_command); 
    }

  int repeat_command;
  int retVal = element->GetScalarAttribute("repeat_command", &repeat_command);
  if(retVal) 
    { 
    this->SetRepeatCommand(repeat_command);
    this->Repeatable = repeat_command;
    }

  const char* remove_command = element->GetAttribute("remove_command");
  if (remove_command)
    {
    this->SetRemoveCommand(remove_command);
    }

  int null_on_empty;
  if (element->GetScalarAttribute("null_on_empty", &null_on_empty))
    {
    this->SetNullOnEmpty(null_on_empty);
    }


  //if (this->RemoveCommand && !this->GetUpdateSelf())
  //  {
  //  vtkErrorMacro("Due to reference counting issue, remove command can "
  //    "only be used for update self properties. " << endl <<
  //    "Offending property: " << this->XMLName);
  //  }
  return ret;
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::LoadState(vtkPVXMLElement* element,
  vtkSMProxyLocator* loader, int loadLastPushedValues/*=0*/)
{
  if (!loader)
    {
    // no loader specified, state remains unchanged.
    return 1;
    }

  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  this->Superclass::LoadState(element, loader, loadLastPushedValues);

  // If "clear" is present and is 0, it implies that the proxy elements
  // currently in the property should not be cleared before loading 
  // the new state.
  int clear=1;
  element->GetScalarAttribute("clear", &clear);
  if (clear)
    {
    this->PPInternals->Proxies.clear();
    }

  if (loadLastPushedValues)
    {
    element = element->FindNestedElementByName("LastPushedValues");
    if (!element)
      {
      vtkErrorMacro("Failed to locate LastPushedValues.");
      this->ImmediateUpdate = prevImUpdate;
      return 0;
      }
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

//---------------------------------------------------------------------------
void vtkSMProxyProperty::ChildSaveState(vtkPVXMLElement* propertyElement,
  int saveLastPushedValues)
{
  this->Superclass::ChildSaveState(propertyElement, saveLastPushedValues);

  unsigned int numProxies = this->GetNumberOfProxies();
  propertyElement->AddAttribute("number_of_elements", numProxies);
  for (unsigned int idx=0; idx<numProxies; idx++)
    {
    vtkPVXMLElement* proxyElement = this->SaveProxyElementState(idx, false);
    if (proxyElement)
      {
      propertyElement->AddNestedElement(proxyElement);
      proxyElement->Delete();
      }
    }

  if (saveLastPushedValues)
    {
    numProxies = this->PPInternals->PreviousProxies.size();
    vtkPVXMLElement* element = vtkPVXMLElement::New();
    element->SetName("LastPushedValues");
    element->AddAttribute("number_of_elements", numProxies);
    for (unsigned int cc=0; cc < numProxies; cc++)
      {
      vtkPVXMLElement* proxyElement = this->SaveProxyElementState(cc, true);
      if (proxyElement)
        {
        element->AddNestedElement(proxyElement);
        proxyElement->Delete();
        }
      }
    propertyElement->AddNestedElement(element);
    element->Delete();
    }
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyProperty::SaveProxyElementState(
  unsigned int idx, bool use_previous_proxies)
{
  // We can assume idx is valid since it's called by ChildSaveState().
  vtkSMProxy* proxy = use_previous_proxies? 
    this->PPInternals->PreviousProxies[idx].GetPointer():
    this->GetProxy(idx);
  vtkPVXMLElement* proxyElement = 0;
  if (proxy)
    {
    proxyElement = vtkPVXMLElement::New();
    proxyElement->SetName("Proxy");
    proxyElement->AddAttribute("value", proxy->GetSelfIDAsString());
    }
  return proxyElement;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::DeepCopy(vtkSMProperty* src, 
  const char* exceptionClass, int proxyPropertyCopyFlag)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxyProperty* dsrc = vtkSMProxyProperty::SafeDownCast(
    src);

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
      pdest->SetConnectionID(psrc->GetConnectionID());
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
      pdest->SetConnectionID(psrc->GetConnectionID());
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
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Values: ";
  for (unsigned int i=0; i<this->GetNumberOfProxies(); i++)
    {
    os << this->GetProxy(i) << " ";
    }
  os << endl;
  os << indent 
     << "CleanCommand: "
     << (this->CleanCommand ? this->CleanCommand : "(none)") 
     << endl;
}
