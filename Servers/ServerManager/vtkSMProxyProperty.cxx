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

#include <vtkstd/vector>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMProxyProperty);
vtkCxxRevisionMacro(vtkSMProxyProperty, "1.8");

struct vtkSMProxyPropertyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > Proxies;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > PreviousProxies;
};

//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->PPInternals = new vtkSMProxyPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  delete this->PPInternals;
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
  if (!this->Command || this->IsReadOnly)
    {
    return;
    }

  unsigned int numProxies = this->GetNumberOfProxies();
  if (numProxies < 1)
    {
    return;
    }

  this->RemoveConsumers(cons);
  this->ClearPreviousProxies();
  for (unsigned int idx=0; idx < numProxies; idx++)
    {
    *str << vtkClientServerStream::Invoke << objectId << this->Command;
    vtkSMProxy* proxy = this->GetProxy(idx);
    if (proxy)
      {
      this->AddPreviousProxy(proxy);
      proxy->AddConsumer(this, cons);
      if (this->UpdateSelf)
        {
        *str << proxy;
        }
      else
        {
        int numIDs = proxy->GetNumberOfIDs();
        for(int i=0; i<numIDs; i++)
          {
          *str << proxy->GetID(i);
          }
        }
      }
    else
      {
      vtkClientServerID nullID = { 0 };
      *str << nullID;
      }
    *str << vtkClientServerStream::End;
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::ClearPreviousProxies()
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
void vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy, int modify)
{
  if (this->IsReadOnly)
    {
    return;
    }
  this->PPInternals->Proxies.push_back(proxy);
  if (modify)
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy)
{
  this->AddProxy(proxy, 1);
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
vtkSMProxy* vtkSMProxyProperty::GetProxy(unsigned int idx)
{
  return this->PPInternals->Proxies[idx];
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::SaveState(
  const char* name,  ofstream* file, vtkIndent indent)
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
  *file << indent << "</Property>" << endl;
          
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
