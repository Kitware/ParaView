/*=========================================================================

   Program: ParaView
   Module:    pqHelperProxyStateLoader.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqHelperProxyStateLoader.h"

#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyLocator.h"

#include <QRegExp>

//-----------------------------------------------------------------------------
pqHelperProxyStateLoader::pqHelperProxyStateLoader(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
bool pqHelperProxyStateLoader::loadState(vtkPVXMLElement* root,
  vtkSMProxyLocator* locator)
{
  this->HelperProxyCollectionElements.clear();

  if (root->GetName() && 
    strcmp(root->GetName(),"ServerManagerState") != 0)
    {
    root = root->FindNestedElementByName("ServerManagerState");
    }
  else
    {
    root = NULL;
    }
  if (!root)
    {
    qCritical("Failed to locate <ServerManagerState /> element. "
      "Cannot load server manager state.");
    return false;
    }

  unsigned int numElems = root->GetNumberOfNestedElements();
  unsigned int i;
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name)
      {
      if (strcmp(name, "ProxyCollection") == 0)
        {
        if (!this->buildProxyCollectionInformation(currentElement))
          {
          return false;
          }
        }
      }
    }
  this->discoverHelperProxies(locator);
  this->HelperProxyCollectionElements.clear();
  return true;
}

//-----------------------------------------------------------------------------
void pqHelperProxyStateLoader::discoverHelperProxies(vtkSMProxyLocator* locator)
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QRegExp helper_group_rx ("pq_helper_proxies.(\\d+)");

  foreach(vtkPVXMLElement* proxyCollection, this->HelperProxyCollectionElements)
    {
    const char* groupname = proxyCollection->GetAttribute("name");
    if (helper_group_rx.indexIn(groupname) == -1)
      {
      continue;
      }
    int proxyid = helper_group_rx.cap(1).toInt();
    vtkSMProxy* proxy = locator->LocateProxy(proxyid);
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
      vtkSMProxy* helper = locator->LocateProxy(helperid);
      if (helper)
        {
        pq_proxy->addHelperProxy(name, helper);
        }
      }
    }
  // TODO: unregister helper proxies from their old names and groups.
}

//-----------------------------------------------------------------------------
int pqHelperProxyStateLoader::buildProxyCollectionInformation(
  vtkPVXMLElement* collectionElement)
{
  const char* groupName = collectionElement->GetAttribute("name");
  if (!groupName)
    {
    qCritical("Required attribute name is missing.");
    return 0;
    }

  QRegExp helper_group_rx ("pq_helper_proxies.(\\d+)");
  if (helper_group_rx.indexIn(groupName) != -1)
    {
    // The collection is a pq_helper_proxies collection.
    // We don't register these proxies directly again, instead
    // we add them as helper proxies which will get registered
    // while adding them as helper proxies to pqProxy objects.
    this->HelperProxyCollectionElements.push_back(collectionElement);
    }
  return 1;
}



