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
#include "vtkSMCommunicationModule.h"
#include "vtkSMProcessModule.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMObject);
vtkCxxRevisionMacro(vtkSMObject, "1.2");

vtkSMCommunicationModule* vtkSMObject::CommunicationModule = 0;
vtkSMProcessModule* vtkSMObject::ProcessModule = 0;
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
vtkSMCommunicationModule* vtkSMObject::GetCommunicationModule()
{
  return vtkSMObject::CommunicationModule;
}

//---------------------------------------------------------------------------
void vtkSMObject::SetCommunicationModule(vtkSMCommunicationModule* cm)
{
  if (vtkSMObject::CommunicationModule == cm)
    {
    return;
    }
  if (vtkSMObject::CommunicationModule)
    {
    vtkSMObject::CommunicationModule->UnRegister(0);
    }
  if (cm)
    {
    cm->Register(0);
    }
  vtkSMObject::CommunicationModule = cm;
}

//---------------------------------------------------------------------------
vtkSMProcessModule* vtkSMObject::GetProcessModule()
{
  return vtkSMObject::ProcessModule;
}

//---------------------------------------------------------------------------
void vtkSMObject::SetProcessModule(vtkSMProcessModule* pm)
{
  if (vtkSMObject::ProcessModule == pm)
    {
    return;
    }
  if (vtkSMObject::ProcessModule)
    {
    vtkSMObject::ProcessModule->UnRegister(0);
    }
  if (pm)
    {
    pm->Register(0);
    }
  vtkSMObject::ProcessModule = pm;
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
    vtkSMObject::ProxyManager->UnRegister(0);
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

  os << indent << "Communication module: " << vtkSMObject::CommunicationModule
     << endl;
  os << indent << "Process module: " << vtkSMObject::ProcessModule << endl;
  os << indent << "Proxy manager: " << vtkSMObject::ProxyManager << endl;
}
