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
#include "vtkSMProxy.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMProxyProperty);
vtkCxxRevisionMacro(vtkSMProxyProperty, "1.5");

struct vtkSMProxyPropertyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > Proxies;
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
    vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command)
    {
    return;
    }

  unsigned int numProxies = this->GetNumberOfProxies();
  if (numProxies < 1)
    {
    return;
    }

  for (unsigned int idx=0; idx < numProxies; idx++)
    {
    *str << vtkClientServerStream::Invoke << objectId << this->Command;
    vtkSMProxy* proxy = this->GetProxy(idx);
    if (proxy)
      {
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
void vtkSMProxyProperty::AddProxy(vtkSMProxy* proxy, int modify)
{
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
    for (unsigned int i=0; i<this->GetNumberOfDomains(); i++)
      {
      vtkSMProxyGroupDomain* dom = vtkSMProxyGroupDomain::SafeDownCast(
        this->GetDomain(i));
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
}
