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
#include "vtkSMProxyManager.h"
#include "vtkSMViewProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSmartPointer.h"
#include <vtkstd/list>
#include <vtkstd/algorithm>

vtkStandardNewMacro(vtkSMPQStateLoader);

struct vtkSMPQStateLoaderInternals
{
  vtkstd::list<vtkSmartPointer<vtkSMViewProxy> > PreferredViews;
};


//-----------------------------------------------------------------------------
vtkSMPQStateLoader::vtkSMPQStateLoader()
{
  this->PQInternal = new vtkSMPQStateLoaderInternals;
}

//-----------------------------------------------------------------------------
vtkSMPQStateLoader::~vtkSMPQStateLoader()
{
  delete this->PQInternal;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPQStateLoader::CreateProxy(
  const char* xml_group, const char* xml_name, vtkIdType cid)
{
  // Check if the proxy requested is a view module.
  if (xml_group && xml_name && strcmp(xml_group, "views") == 0)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMProxy* prototype = pxm->GetPrototypeProxy(xml_group, xml_name);
    if (prototype && prototype->IsA("vtkSMViewProxy"))
      {
      // Retrieve the view type that should be used/created
      const char* preferred_xml_name = this->GetViewXMLName(cid, xml_name);

      // Look for a view of this type among our preferred views, return the
      // first one found
      // Return a preferred render view if one exists of this type.
      vtkstd::list<vtkSmartPointer<vtkSMViewProxy> >::iterator iter = 
        this->PQInternal->PreferredViews.begin();
      while(iter != this->PQInternal->PreferredViews.end())
        {
        vtkSMViewProxy *viewProxy = *iter;
        if(viewProxy->GetConnectionID() == cid &&
          strcmp(viewProxy->GetXMLName(), preferred_xml_name) == 0)
          {
          viewProxy->Register(this);
          this->PQInternal->PreferredViews.erase(iter);
          return viewProxy;
          }
        iter++;
        }
      }
    }

  // If all else fails, let the superclass handle it:
  return this->Superclass::CreateProxy(xml_group, xml_name, cid);
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::AddPreferredView(vtkSMViewProxy *view)
{
  if(!view)
    { 
    vtkWarningMacro("Could not add preffered render module.");
    return;
    }
  // Make sure it is not part of the list yet
  vtkstd::list<vtkSmartPointer<vtkSMViewProxy> >::iterator begin = 
    this->PQInternal->PreferredViews.begin();
  vtkstd::list<vtkSmartPointer<vtkSMViewProxy> >::iterator end = 
    this->PQInternal->PreferredViews.end();
  if(vtkstd::find(begin,end,view) == end)
    {
    this->PQInternal->PreferredViews.push_back(view);
    }
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::RemovePreferredView(vtkSMViewProxy *view)
{
  this->PQInternal->PreferredViews.remove(view);
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::ClearPreferredViews()
{
  this->PQInternal->PreferredViews.clear();
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::RegisterProxyInternal(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  if (proxy->GetXMLGroup() 
    && strcmp(proxy->GetXMLGroup(), "views")==0)
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
}
