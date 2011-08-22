/*=========================================================================

  Program:   ParaView
  Module:    vtkSMObject.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMObject.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMObject);
vtkSMProxyManager* vtkSMObject::ProxyManager = 0;
//---------------------------------------------------------------------------
vtkSMObject::vtkSMObject()
{
}

//---------------------------------------------------------------------------
vtkSMObject::~vtkSMObject()
{
}

//---------------------------------------------------------------------------
vtkSMProxyManager* vtkSMObject::GetProxyManager()
{
  return vtkSMObject::ProxyManager;
}

//---------------------------------------------------------------------------
void vtkSMObject::SetProxyManager(vtkSMProxyManager* pm)
{
  if (vtkSMObject::ProxyManager == pm)
    {
    return;
    }
  if (vtkSMObject::ProxyManager)
    {
    // this indirect cleanup ensures that we don't accidentally end up calling
    // the destructor on the vtkSMObject::ProxyManager twice.
    vtkSMProxyManager* temp = vtkSMObject::ProxyManager;
    vtkSMObject::ProxyManager = NULL;
    temp->UnRegister(0);
    }
  if (pm)
    {
    pm->Register(0);
    }
  vtkSMObject::ProxyManager = pm;
}

//---------------------------------------------------------------------------
void vtkSMObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
