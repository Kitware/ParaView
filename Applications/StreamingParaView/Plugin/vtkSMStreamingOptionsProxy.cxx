/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStreamingOptionsProxy.h"

#include "vtkStreamingFactory.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkStreamingOptions.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"

//-----------------------------------------------------------------------------
int vtkSMStreamingOptionsProxy::StreamingFactoryRegistered = 0;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMStreamingOptionsProxy);

//----------------------------------------------------------------------------
vtkSMStreamingOptionsProxy::vtkSMStreamingOptionsProxy()
{
  // Register the streaming object factory
  if (!vtkSMStreamingOptionsProxy::StreamingFactoryRegistered)
    {
    vtkStreamingFactory* sf = vtkStreamingFactory::New();
    vtkObjectFactory::RegisterFactory(sf);
    vtkSMStreamingOptionsProxy::StreamingFactoryRegistered = 1;
    sf->Delete();
    }

}

//----------------------------------------------------------------------------
vtkSMStreamingOptionsProxy::~vtkSMStreamingOptionsProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMStreamingOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
const char* vtkSMStreamingOptionsProxy::GetInstanceName()
{
  static const char* name = "StreamingOptionsInstance";
  return name;
}

//----------------------------------------------------------------------------
vtkSMStreamingOptionsProxy * vtkSMStreamingOptionsProxy::GetProxy()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMStreamingOptionsProxy * proxy = 
    vtkSMStreamingOptionsProxy::SafeDownCast(
      pxm->GetProxy("helpers", vtkSMStreamingOptionsProxy::GetInstanceName()));
  if (!proxy)
    {
    proxy = vtkSMStreamingOptionsProxy::SafeDownCast(
      pxm->NewProxy("helpers", "StreamingOptions")); 
    if (proxy)
      {
      proxy->SetConnectionID(vtkProcessModuleConnectionManager::GetAllConnectionsID());
      proxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
      pxm->RegisterProxy("helpers", vtkSMStreamingOptionsProxy::GetInstanceName(), proxy);
      proxy->Delete();
      }
    }
  return proxy;
}

/*
//----------------------------------------------------------------------------
vtkStreamingOptions * vtkSMStreamingOptionsProxy::GetOptions()
{
  return vtkStreamingOptions::GetOptions();
}
*/
