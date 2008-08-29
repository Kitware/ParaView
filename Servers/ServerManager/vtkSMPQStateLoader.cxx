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
vtkCxxRevisionMacro(vtkSMPQStateLoader, "1.29");

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
vtkSMProxy* vtkSMPQStateLoader::NewProxyInternal(
  const char* xml_group, const char* xml_name)
{
  // Check if the proxy requested is a view module.
  if (xml_group && xml_name && strcmp(xml_group, "views") == 0)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMProxy* prototype = pxm->GetPrototypeProxy(xml_group, xml_name);
    if (prototype && prototype->IsA("vtkSMViewProxy"))
      {
      // Retrieve the view type that should be used/created
      const char* preferred_xml_name = 
        this->GetPreferredViewType(this->GetConnectionID(), xml_name);

      // Look for a view of this type among our preferred views, return the
      // first one found
      if (!this->PQInternal->PreferredViews.empty())
        {
        // Return a preferred render view if one exists of this type.
        vtkstd::list<vtkSmartPointer<vtkSMViewProxy> >::iterator iter = 
          this->PQInternal->PreferredViews.begin();
        while(iter != this->PQInternal->PreferredViews.end())
          {
          vtkSMViewProxy *viewProxy = *iter;
          if(strcmp(viewProxy->GetXMLName(), preferred_xml_name)==0)
            {
            viewProxy->Register(this);
            this->PQInternal->PreferredViews.erase(iter);
            return viewProxy;
            }
          iter++;
          }
        }

      // Can't use existing module (none present, none of the correct type, 
      // or all present are have already been used, hence we allocate a new one
      // of the preferred type.
      vtkSMProxy* preferred_prototype = 
        pxm->GetPrototypeProxy(xml_group, preferred_xml_name);
      if (preferred_prototype)
        {
        return this->Superclass::NewProxyInternal(xml_group, preferred_xml_name);
        }
      }
    }

  // If all else fails, let the superclass handle it:
  return this->Superclass::NewProxyInternal(xml_group, xml_name);
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
const char* vtkSMPQStateLoader::GetPreferredViewType (int connectionID,
  const char *xml_name)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMViewProxy* prototype = vtkSMViewProxy::SafeDownCast(
    pxm->GetPrototypeProxy("views", xml_name));
  if (prototype)
    {
    // Generally each view type is different class of view eg. bar char view, line
    // plot view etc. However in some cases a different view types are indeed the
    // same class of view the only different being that each one of them works in
    // a different configuration eg. "RenderView" in builin mode, 
    // "IceTDesktopRenderView" in remote render mode etc. This method is used to
    // determine what type of view needs to be created for the given class. 
    return prototype->GetSuggestedViewType(connectionID);
    }

  return xml_name;
}

//-----------------------------------------------------------------------------
void vtkSMPQStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
