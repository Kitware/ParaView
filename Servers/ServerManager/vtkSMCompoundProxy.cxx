/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompoundProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompoundProxy.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMCompoundProxy);
vtkCxxRevisionMacro(vtkSMCompoundProxy, "1.2");

vtkCxxSetObjectMacro(vtkSMCompoundProxy, MainProxy, vtkSMProxy);

//----------------------------------------------------------------------------
vtkSMCompoundProxy::vtkSMCompoundProxy()
{
  this->MainProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMCompoundProxy::~vtkSMCompoundProxy()
{
  if (this->MainProxy)
    {
    this->MainProxy->Delete();
    }
  this->MainProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMCompoundProxy::GetProxy(const char* name)
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetSubProxy(name);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMCompoundProxy::GetProxy(unsigned int index)
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetSubProxy(index);
}

//----------------------------------------------------------------------------
void vtkSMCompoundProxy::AddProxy(const char* name, vtkSMProxy* proxy)
{
  if (!this->MainProxy)
    {
    this->MainProxy = vtkSMProxy::New();
    }
  this->MainProxy->AddSubProxy(name, proxy);
}

//----------------------------------------------------------------------------
void vtkSMCompoundProxy::RemoveProxy(const char* name)
{
  if (!this->MainProxy)
    {
    return;
    }

  this->MainProxy->RemoveSubProxy(name);
}

//----------------------------------------------------------------------------
const char* vtkSMCompoundProxy::GetProxyName(unsigned int index)
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetSubProxyName(index);
}

//----------------------------------------------------------------------------
unsigned int vtkSMCompoundProxy::GetNumberOfProxies()
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetNumberOfSubProxies();
}

//----------------------------------------------------------------------------
void vtkSMCompoundProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}




