/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPQStateLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPQStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSmartPointer.h"
#include <vtkstd/list>
#include <vtkstd/algorithm>

vtkStandardNewMacro(vtkSMPQStateLoader);
vtkCxxRevisionMacro(vtkSMPQStateLoader, "1.16");
vtkCxxSetObjectMacro(vtkSMPQStateLoader, MultiViewRenderModuleProxy, 
  vtkSMMultiViewRenderModuleProxy);


struct vtkSMPQStateLoaderInternals
{
  vtkstd::list<vtkSmartPointer<vtkSMRenderModuleProxy> > PreferredRenderModules;
};


//-----------------------------------------------------------------------------
vtkSMPQStateLoader::vtkSMPQStateLoader()
{
  this->PQInternal = new vtkSMPQStateLoaderInternals;
  this->MultiViewRenderModuleProxy = 0;
}

//-----------------------------------------------------------------------------
vtkSMPQStateLoader::~vtkSMPQStateLoader()
{
  this->SetMultiViewRenderModuleProxy(0);
  delete this->PQInternal;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPQStateLoader::NewProxyInternal(
  const char* xml_group, const char* xml_name)
{
  // Check if the proxy requested is a render module.
  if (xml_group && xml_name && strcmp(xml_group, "rendermodules") == 0)
    {
    if (strcmp(xml_name, "MultiViewRenderModule") == 0)
      {
      if (this->MultiViewRenderModuleProxy)
        {
        this->MultiViewRenderModuleProxy->Register(this);
        return this->MultiViewRenderModuleProxy;
        }
      vtkWarningMacro("MultiViewRenderModuleProxy is not set. "
        "Creating MultiViewRenderModuleProxy from the state.");
      }
    else
      {
      // Create a rendermodule.
      if (this->MultiViewRenderModuleProxy)
        {
        if (!this->PQInternal->PreferredRenderModules.empty())
          {
          vtkSMRenderModuleProxy *renMod = this->PQInternal->PreferredRenderModules.front();
          unsigned int i=0;
          for(i=0; i<this->MultiViewRenderModuleProxy->GetNumberOfRenderModules(); i++)
            {
            if(this->MultiViewRenderModuleProxy->GetRenderModule(i) == renMod)
              {
              break;
              }
            }
          if(i<this->MultiViewRenderModuleProxy->GetNumberOfRenderModules())
            {
            renMod->Register(this);
            this->PQInternal->PreferredRenderModules.pop_front();
            return renMod;
            }
          this->PQInternal->PreferredRenderModules.pop_front();
          }
        // Can't use exiting module (none present, or all present are have
        // already been used, hence we allocate a new one.
        return this->MultiViewRenderModuleProxy->NewRenderModule();
        }
      vtkWarningMacro("MultiViewRenderModuleProxy is not set. "
        "Creating MultiViewRenderModuleProxy from the state.");
      }
    }
  else if (xml_group && xml_name && strcmp(xml_group, "displays")==0)
    {
    vtkSMProxy *display = this->Superclass::NewProxyInternal(xml_group, xml_name);
    if (vtkSMDataObjectDisplayProxy::SafeDownCast(display))
      {
      if (this->MultiViewRenderModuleProxy)
        {
        display->Delete();
        display = this->MultiViewRenderModuleProxy->CreateDisplayProxy();
        }
      }
    return display;
    }
  else if (xml_group && xml_name && strcmp(xml_group, "misc") == 0 
    && strcmp(xml_name, "TimeKeeper") == 0)
    {
    // There is only one time keeper per connection, simply
    // load the state on the timekeeper.
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMProxy* timekeeper = pxm->GetProxy("timekeeper", "TimeKeeper");
    if (timekeeper)
      {
      timekeeper->Register(this);
      return timekeeper;
      }
    }
  return this->Superclass::NewProxyInternal(xml_group, xml_name);
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::AddPreferredRenderModule(vtkSMRenderModuleProxy *renderModule)
{
  if(!renderModule)
    { 
    vtkWarningMacro("Could not add preffered render module.");
    return;
    }
  // Make sure it is not part of the list yet
  vtkstd::list<vtkSmartPointer<vtkSMRenderModuleProxy> >::iterator begin = this->PQInternal->PreferredRenderModules.begin();
  vtkstd::list<vtkSmartPointer<vtkSMRenderModuleProxy> >::iterator end = this->PQInternal->PreferredRenderModules.end();
  if(find(begin,end,renderModule) == end)
    {
    this->PQInternal->PreferredRenderModules.push_back(renderModule);
    }
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::RemovePreferredRenderModule(vtkSMRenderModuleProxy *renderModule)
{
  this->PQInternal->PreferredRenderModules.remove(renderModule);
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::ClearPreferredRenderModules()
{
  this->PQInternal->PreferredRenderModules.clear();
}


//---------------------------------------------------------------------------
void vtkSMPQStateLoader::RegisterProxyInternal(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  if (proxy->GetXMLGroup() 
    && strcmp(proxy->GetXMLGroup(), "rendermodules")==0)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    if (pxm->GetProxyName(group, proxy))
      {
      // render module is registered, don't re-register it.
      return;
      }
    }
  this->Superclass::RegisterProxyInternal(group, name, proxy);
}

//-----------------------------------------------------------------------------
void vtkSMPQStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultiViewRenderModuleProxy: " 
     << this->MultiViewRenderModuleProxy << endl;
}
