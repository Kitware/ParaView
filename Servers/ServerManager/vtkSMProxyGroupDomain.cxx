/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyGroupDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyGroupDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkSMProxyGroupDomain);
vtkCxxRevisionMacro(vtkSMProxyGroupDomain, "1.1");

//---------------------------------------------------------------------------
vtkSMProxyGroupDomain::vtkSMProxyGroupDomain()
{
  this->ProxyGroup = 0;
}

//---------------------------------------------------------------------------
vtkSMProxyGroupDomain::~vtkSMProxyGroupDomain()
{
  this->SetProxyGroup(0);
}

//---------------------------------------------------------------------------
int vtkSMProxyGroupDomain::IsInDomain(vtkSMProperty* property)
{
  if (!property)
    {
    return 0;
    }
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(property);
  if (!pp)
    {
    return 0;
    }
  return this->IsInDomain(pp->GetProxy());
}

//---------------------------------------------------------------------------
int vtkSMProxyGroupDomain::IsInDomain(vtkSMProxy* proxy)
{
  if (!proxy || !this->ProxyGroup)
    {
    return 0;
    }

  vtkSMProxyManager* pm = this->GetProxyManager();
  if (pm)
    {
    return pm->IsProxyInGroup(proxy, this->ProxyGroup);
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyGroupDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
