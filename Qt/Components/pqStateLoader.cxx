/*=========================================================================

   Program: ParaView
   Module:    pqStateLoader.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"

#include <QPointer>
#include <QRegExp>

#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqMainWindowCore.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "pqViewManager.h"

//-----------------------------------------------------------------------------
class pqStateLoaderInternal
{
public:
  QPointer<pqMainWindowCore> MainWindowCore;
  QList<vtkSmartPointer<vtkPVXMLElement> > HelperProxyCollectionElements;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(pqStateLoader);
vtkCxxRevisionMacro(pqStateLoader, "1.18");
//-----------------------------------------------------------------------------
pqStateLoader::pqStateLoader()
{
  this->Internal = new pqStateLoaderInternal;
}

//-----------------------------------------------------------------------------
pqStateLoader::~pqStateLoader()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqStateLoader::SetMainWindowCore(pqMainWindowCore* core)
{
  this->Internal->MainWindowCore = core;
}

//-----------------------------------------------------------------------------
int pqStateLoader::LoadStateInternal(vtkPVXMLElement* root)
{
  this->Internal->HelperProxyCollectionElements.clear();

  // Load the <ServerManagerState /> element first.
  if (!this->Superclass::LoadStateInternal(root))
    {
    return 0;
    }

  vtkPVXMLElement *viewManagerXML =
    root->FindNestedElementByName("ViewManager");
  if (viewManagerXML)
    {
    if (!this->Internal->MainWindowCore->multiViewManager().loadState(
        viewManagerXML, this->ProxyLocator)) 
      {
      return 0;
      }
    }

  // After having loaded all state,
  // try to discover helper proxies for all pqProxies.
  this->DiscoverHelperProxies();
  this->Internal->HelperProxyCollectionElements.clear();
  return 1;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqStateLoader::CreateProxy(
  const char* xml_group, const char* xml_name, vtkIdType cid)
{
  if (xml_group && xml_name && strcmp(xml_group, "animation")==0
    && strcmp(xml_name, "AnimationScene")==0)
    {
    // If an animation scene already exists, we use that.
    pqAnimationScene* scene = 
      this->Internal->MainWindowCore->getAnimationManager()->getActiveScene();
    if (scene)
      {
      vtkSMProxy* proxy = scene->getProxy();
      proxy->Register(this);
      return proxy;
      }
    }
  else if (xml_group && xml_name && strcmp(xml_group, "misc") == 0 
    && strcmp(xml_name, "TimeKeeper") == 0)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    // There is only one time keeper per connection, simply
    // load the state on the timekeeper.
    vtkSMProxy* timekeeper = pxm->GetProxy("timekeeper", "TimeKeeper");
    if (timekeeper)
      {
      timekeeper->Register(this);
      return timekeeper;
      }
    }

  return this->Superclass::CreateProxy(xml_group, xml_name, cid);
}

//---------------------------------------------------------------------------
void pqStateLoader::RegisterProxyInternal(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  // Don't re-register a proxy in the same group.
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm->GetProxyName(group, proxy))
    {
    // scene is registered, don't re-register it.
    return;
    }

  this->Superclass::RegisterProxyInternal(group, name, proxy);
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqStateLoader::LocateProxyElement(int id)
{
  vtkPVXMLElement* proxyElement = this->Superclass::LocateProxyElement(id);
  if (!proxyElement || !proxyElement->GetAttribute("group") ||
    !proxyElement->GetAttribute("type"))
    {
    return proxyElement;
    }

  const char* xml_group = proxyElement->GetAttribute("group");
  const char* xml_name = proxyElement->GetAttribute("type");
  if (strcmp(xml_group, "views") == 0)
    {
    unsigned int max = proxyElement->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < max; ++cc)
      {
      vtkPVXMLElement* element = proxyElement->GetNestedElement(cc);
      if (element->GetName() == QString("Property") &&
        element->GetAttribute("name") == QString("ViewSize"))
        {
        proxyElement->RemoveNestedElement(element);
        break;
        }
      }
    }
  else if (strcmp(xml_group, "misc")==0 && strcmp(xml_name, "TimeKeeper") == 0)
    {
    // FIXME: This needs to be fixed by making the vtkSMTimeKeeper internally
    // manage the views and timestep values so that on never manually has to set
    // it up.
    unsigned int max = proxyElement->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < max; ++cc)
      {
      // Views are not loaded from state, since the pqTimeKeeper 
      // automatically updates the property appropriately.
      vtkPVXMLElement* element = proxyElement->GetNestedElement(cc);
      if (element->GetName() == QString("Property") &&
        element->GetAttribute("name") == QString("Views"))
        {
        proxyElement->RemoveNestedElement(element);
        cc--;
        max--;
        continue;
        }
      // We don't want to upload the values from "TimestepValues" property
      // either since that's populated by the GUI. 
      if (element->GetName() == QString("Property") &&
        element->GetAttribute("name") == QString("TimestepValues"))
        {
        proxyElement->RemoveNestedElement(element);
        cc--;
        max--;
        continue;
        }
      }
    }

  return proxyElement;
}

//-----------------------------------------------------------------------------
int pqStateLoader::BuildProxyCollectionInformation(
  vtkPVXMLElement* collectionElement)
{
  const char* groupName = collectionElement->GetAttribute("name");
  if (!groupName)
    {
    vtkErrorMacro("Required attribute name is missing.");
    return 0;
    }

  QRegExp helper_group_rx ("pq_helper_proxies.(\\d+)");
  if (helper_group_rx.indexIn(groupName) == -1)
    {
    return this->Superclass::BuildProxyCollectionInformation(
      collectionElement);
    }

  // The collection is a pq_helper_proxies collection.
  // We don't register these proxies directly again, instead
  // we add them as helper proxies which will get registered
  // while adding them as helper proxies to pqProxy objects.
  this->Internal->HelperProxyCollectionElements.push_back(collectionElement);
  return 1;
}

//-----------------------------------------------------------------------------
void pqStateLoader::DiscoverHelperProxies()
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QRegExp helper_group_rx ("pq_helper_proxies.(\\d+)");

  foreach(vtkPVXMLElement* proxyCollection, 
    this->Internal->HelperProxyCollectionElements)
    {
    const char* groupname = proxyCollection->GetAttribute("name");
    if (helper_group_rx.indexIn(groupname) == -1)
      {
      continue;
      }
    int proxyid = helper_group_rx.cap(1).toInt();
    vtkSMProxy* proxy = this->ProxyLocator->LocateProxy(proxyid);
    pqProxy *pq_proxy = smmodel->findItem<pqProxy*>(proxy);
    if (!pq_proxy)
      {
      continue;
      }
    unsigned int num_children = proxyCollection->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < num_children; cc++)
      {
      vtkPVXMLElement* child = proxyCollection->GetNestedElement(cc);
      if (child->GetName() != QString("Item"))
        {
        continue;
        }
      const char* name = child->GetAttribute("name");
      int helperid;
      if (!name || !child->GetScalarAttribute("id", &helperid))
        {
        continue;
        }
      vtkSMProxy* helper = this->ProxyLocator->LocateProxy(helperid);
      if (helper)
        {
        pq_proxy->addHelperProxy(name, helper);
        }
      }
    }
}


//-----------------------------------------------------------------------------
void pqStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
