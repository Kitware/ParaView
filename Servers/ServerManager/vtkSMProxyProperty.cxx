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
#include "vtkSMDomainIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkPVXMLElement.h"

#include <vtkstd/vector>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMProxyProperty);
vtkCxxRevisionMacro(vtkSMProxyProperty, "1.14.6.4");

struct vtkSMProxyPropertyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > Proxies;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > PreviousProxies;
  vtkstd::vector<vtkSMProxy*> UncheckedProxies;
};

//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->PPInternals = new vtkSMProxyPropertyInternals;
  this->CleanCommand = 0;
  this->SetSaveable(1);
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  delete this->PPInternals;
  this->SetCleanCommand(0);
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
void vtkSMProxyProperty::AppendCommandToStream(
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->InformationOnly)
    {
    return;
    }

  if (this->CleanCommand)
    {
    *str << vtkClientServerStream::Invoke
      << objectId << this->CleanCommand
      << vtkClientServerStream::End;
    }

  unsigned int numProxies = this->GetNumberOfProxies();
  if (numProxies < 1)
    {
    return;
    }

  // Remove all consumers (using previous proxies)
  this->RemoveConsumers(cons);
  // Remove all previous proxies before adding new ones.
  this->RemoveAllPreviousProxies();

  unsigned int numConsIDs = cons->GetNumberOfIDs();
  
  for (unsigned int idx=0; idx < numProxies; idx++)
    {
    vtkSMProxy* proxy = this->GetProxy(idx);
    if (!proxy)
      {
      vtkClientServerID nullID = { 0 };
      *str << vtkClientServerStream::Invoke << objectId << this->Command
        << nullID << vtkClientServerStream::End;
      continue;
      }
  
    // Keep track of all proxies that point to this as a
    // consumer so that we can remove this from the consumer
    // list later if necessary
    this->AddPreviousProxy(proxy);
    proxy->AddConsumer(this, cons);

    if (this->UpdateSelf)
      {
      *str << vtkClientServerStream::Invoke << objectId << this->Command
        << proxy << vtkClientServerStream::End;
      continue;
      }

    unsigned int numIDs = proxy->GetNumberOfIDs();
    // Determine now the IDs are added.
    if (numConsIDs == numIDs)
      {
      // One to One Mapping between the IDs.
      for (unsigned int i = 0; i < numIDs; i++)
        {
        if (cons->GetID(i) == objectId)
          {
          // This check is essential since AppendCommandToStream is called
          // for all IDs in cons.
          *str << vtkClientServerStream::Invoke << objectId << this->Command
            << proxy->GetID(i)
            << vtkClientServerStream::End;
          }
        }
      }
    else if (numConsIDs == 1)
      {
      // One to Many Mapping.
      for (unsigned int i=0 ; i < numIDs; i++)
        {
        *str << vtkClientServerStream::Invoke << objectId << this->Command
          << proxy->GetID(i) << vtkClientServerStream::End;
        }
      }
    else if (numIDs == 1)
      {
      // Many to One Mapping.
      // No need to loop since AppendCommandToStream is called
      // for all IDs in cons.
      *str << vtkClientServerStream::Invoke << objectId << this->Command
        << proxy->GetID(0)
        << vtkClientServerStream::End;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllPreviousProxies()
{
  this->PPInternals->PreviousProxies.erase(
    this->PPInternals->PreviousProxies.begin(),
    this->PPInternals->PreviousProxies.end());
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddPreviousProxy(vtkSMProxy* proxy)
{
  this->PPInternals->PreviousProxies.push_back(proxy);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveConsumers(vtkSMProxy* proxy)
{
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator it =
    this->PPInternals->PreviousProxies.begin();
  for(; it != this->PPInternals->PreviousProxies.end(); it++)
    {
    it->GetPointer()->RemoveConsumer(this, proxy);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddUncheckedProxy(vtkSMProxy* proxy)
{
  this->PPInternals->UncheckedProxies.push_back(proxy);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy)
{
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
void vtkSMProxyProperty::RemoveProxy(vtkSMProxy* proxy, int modify)
{
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> >::iterator iter =   
    this->PPInternals->Proxies.begin();
  for ( ; iter != this->PPInternals->Proxies.end() ; ++iter)
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
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::SetProxy(unsigned int idx, vtkSMProxy* proxy)
{
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

  this->PPInternals->Proxies[idx] = proxy;
  this->Modified();

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy)
{
  return this->AddProxy(proxy, 1);
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::RemoveAllProxies()
{
  this->PPInternals->Proxies.clear();
  this->Modified();
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
  return ret;
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SaveState(
  const char* name,  ostream* file, vtkIndent indent)
{
  vtkSMProxyManager* pm = this->GetProxyManager();
  if (!pm)
    {
    return;
    }
  
  *file << indent << "<Property name=\"" << (this->XMLName?this->XMLName:"")
        << "\" id=\"" << name << "\" ";
  vtkstd::vector<vtkStdString> proxies;
  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int idx=0; idx<numProxies; idx++)
    {
    this->DomainIterator->Begin();
    while (!this->DomainIterator->IsAtEnd())
      {
      vtkSMProxyGroupDomain* dom = vtkSMProxyGroupDomain::SafeDownCast(
        this->DomainIterator->GetDomain());
      if (dom)
        {
        unsigned int numGroups = dom->GetNumberOfGroups();
        for (unsigned int j=0; j<numGroups; j++)
          {
          const char* proxyname = pm->IsProxyInGroup(
            this->GetProxy(idx), dom->GetGroup(j));
          if (proxyname)
            {
            proxies.push_back(proxyname);
            }
          }
        }
      this->DomainIterator->Next();
      }
    }
  unsigned int numFoundProxies = proxies.size();
  if (numFoundProxies > 0)
    {
    *file << "number_of_elements=\"" << numFoundProxies << "\">" << endl;
    for(unsigned int i=0; i<numFoundProxies; i++)
      {
      *file << indent.GetNextIndent() << "<Element index=\""
            << i << "\" " << "value=\"" << proxies[i].c_str() << "\"/>"
            << endl;
      }
    }
  else
    {
    *file << ">" << endl;
    }
  this->Superclass::SaveState(name, file, indent);
  *file << indent << "</Property>" << endl;
          
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::DeepCopy(vtkSMProperty* src)
{
  this->Superclass::DeepCopy(src);

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

  if (this->ImmediateUpdate)
    {
    this->Modified();
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
  os << indent 
     << "CleanCommand: "
     << (this->CleanCommand ? this->CleanCommand : "(none)") 
     << endl;
}
