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

vtkStandardNewMacro(vtkSMProxyProperty);
vtkCxxRevisionMacro(vtkSMProxyProperty, "1.4");

vtkCxxSetObjectMacro(vtkSMProxyProperty, Proxy, vtkSMProxy);

//---------------------------------------------------------------------------
vtkSMProxyProperty::vtkSMProxyProperty()
{
  this->Proxy = 0;
}

//---------------------------------------------------------------------------
vtkSMProxyProperty::~vtkSMProxyProperty()
{
  if (this->Proxy)
    {
    this->Proxy->Delete();
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

  *str << vtkClientServerStream::Invoke << objectId << this->Command;
  if (this->Proxy)
    {
    int numIDs = this->Proxy->GetNumberOfIDs();
    for(int i=0; i<numIDs; i++)
      {
      *str << this->Proxy->GetID(i);
      }
    }
  else
    {
    vtkClientServerID nullID = { 0 };
    *str << nullID;
    }
  *str << vtkClientServerStream::End;
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
          this->Proxy, dom->GetGroup(j));
        if (proxyname)
          {
          *file << indent 
                << name 
                << " : " <<  proxyname
                << " : " << dom->GetGroup(j)
                << endl;
          return;
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Proxy: " << this->Proxy << endl;
}
