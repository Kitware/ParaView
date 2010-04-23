/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAdaptiveOptionsProxy.h"

#include "vtkAdaptiveFactory.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkAdaptiveOptions.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"

//-----------------------------------------------------------------------------
int vtkSMAdaptiveOptionsProxy::AdaptiveFactoryRegistered = 0;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMAdaptiveOptionsProxy);

//----------------------------------------------------------------------------
vtkSMAdaptiveOptionsProxy::vtkSMAdaptiveOptionsProxy()
{
  // Register the streaming object factory
  if (!vtkSMAdaptiveOptionsProxy::AdaptiveFactoryRegistered)
    {
    vtkAdaptiveFactory* sf = vtkAdaptiveFactory::New();
    vtkObjectFactory::RegisterFactory(sf);
    vtkSMAdaptiveOptionsProxy::AdaptiveFactoryRegistered = 1;
    sf->Delete();
    }

}

//----------------------------------------------------------------------------
vtkSMAdaptiveOptionsProxy::~vtkSMAdaptiveOptionsProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
const char* vtkSMAdaptiveOptionsProxy::GetInstanceName()
{
  static const char* name = "AdaptiveOptionsInstance";
  return name;
}

//----------------------------------------------------------------------------
vtkSMAdaptiveOptionsProxy * vtkSMAdaptiveOptionsProxy::GetProxy()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMAdaptiveOptionsProxy * proxy = 
    vtkSMAdaptiveOptionsProxy::SafeDownCast(
      pxm->GetProxy("helpers", vtkSMAdaptiveOptionsProxy::GetInstanceName()));
  if (!proxy)
    {
    proxy = vtkSMAdaptiveOptionsProxy::SafeDownCast(
      pxm->NewProxy("helpers", "AdaptiveOptions")); 
    if (proxy)
      {
      proxy->SetConnectionID(vtkProcessModuleConnectionManager::GetAllConnectionsID());
      proxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
      pxm->RegisterProxy("helpers", vtkSMAdaptiveOptionsProxy::GetInstanceName(), proxy);
      proxy->Delete();
      }
    }
  return proxy;
}
