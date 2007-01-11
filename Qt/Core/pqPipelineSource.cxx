/*=========================================================================

   Program: ParaView
   Module:    pqPipelineSource.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/

/// \file pqPipelineSource.cxx
/// \date 4/17/2006

#include "pqPipelineSource.h"

// ParaView Server Manager.
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPropertyLink.h"

// Qt
#include <QList>
#include <QMap>
#include <QtDebug>

// ParaView
#include "pqGenericViewModule.h"
#include "pqConsumerDisplay.h"
#include "pqPipelineFilter.h"
#include "pqXMLUtil.h"

//-----------------------------------------------------------------------------
class pqPipelineSourceInternal 
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;

  QString Name;
  QList<pqPipelineSource*> Consumers;
  QList<pqConsumerDisplay*> Displays;
  QList<vtkSmartPointer<vtkSMPropertyLink> > Links;
  QList<vtkSmartPointer<vtkSMProxy> > ProxyListDomainProxies;

  pqPipelineSourceInternal(QString name, vtkSMProxy* proxy)
    {
    this->Name = name;
    this->Proxy = proxy;
    }
};


//-----------------------------------------------------------------------------
pqPipelineSource::pqPipelineSource(const QString& name, vtkSMProxy* proxy,
  pqServer* server, QObject* _parent/*=NULL*/) 
: pqProxy("sources", name, proxy, server, _parent)
{
  this->Internal = new pqPipelineSourceInternal(name, proxy);
  
  // Create any internal proxies needed by any property
  // that has a vtkSMProxyListDomain.
  this->createProxiesForProxyListDomains();
}

//-----------------------------------------------------------------------------
pqPipelineSource::~pqPipelineSource()
{
  delete this->Internal;
}
//-----------------------------------------------------------------------------
void pqPipelineSource::createProxiesForProxyListDomains()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy = this->getProxy();
  vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyProperty* prop = vtkSMProxyProperty::SafeDownCast(
      iter->GetProperty());
    if (!prop)
      {
      continue;
      }
    vtkSMProxyListDomain* pld = vtkSMProxyListDomain::SafeDownCast(
      prop->GetDomain("proxy_list"));
    if (!pld)
      {
      continue;
      }

    QList<vtkSMProxy*> domainProxies;
    for (unsigned int cc=0; cc < pld->GetNumberOfProxies(); cc++)
      {
      domainProxies.push_back(pld->GetProxy(cc));
      }

    unsigned int max = pld->GetNumberOfProxyTypes();
    for (unsigned int cc=0; cc < max; cc++)
      {
      QString proxy_group = pld->GetProxyGroup(cc);
      QString proxy_type = pld->GetProxyName(cc);

      vtkSmartPointer<vtkSMProxy> new_proxy;

      // check if a proxy of the indicated type already exists in the domain,
      // we use it, if present.
      foreach(vtkSMProxy* temp_proxy, domainProxies)
        {
        if (temp_proxy && proxy_group == temp_proxy->GetXMLGroup() 
          && proxy_type == temp_proxy->GetXMLName())
          {
          new_proxy = temp_proxy;
          break;
          }
        }
      
      if (new_proxy.GetPointer() == NULL)
        {
        new_proxy.TakeReference(pxm->NewProxy(pld->GetProxyGroup(cc),
            pld->GetProxyName(cc)));
        if (!new_proxy.GetPointer())
          {
          qDebug() << "Could not create a proxy of type " 
            << proxy_group << "." << proxy_type <<
            " indicated the proxy list domain.";
          continue;
          }
        new_proxy->SetConnectionID(proxy->GetConnectionID());
        pld->AddProxy(new_proxy);
        domainProxies.push_back(new_proxy);
        this->processProxyListHints(new_proxy);
        this->Internal->ProxyListDomainProxies.push_back(new_proxy);
        }
      this->addInternalProxy(iter->GetKey(), new_proxy);
      }
    }
  iter->Delete();
}


//-----------------------------------------------------------------------------
void pqPipelineSource::processProxyListHints(vtkSMProxy *proxy_list_proxy)
{
  vtkPVXMLElement* proxy_list_hint = pqXMLUtil::FindNestedElementByName(
    proxy_list_proxy->GetHints(), "ProxyList");
  if (proxy_list_hint)
    {
    for (unsigned int cc=0; 
      cc < proxy_list_hint->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* child = proxy_list_hint->GetNestedElement(cc);
      if (child && QString("Link") == child->GetName())
        {
        const char* name = child->GetAttribute("name");
        const char* linked_with = child->GetAttribute("with_property");
        if (name && linked_with)
          {
          vtkSMPropertyLink* link = vtkSMPropertyLink::New();
          link->AddLinkedProperty(
            this->getProxy(), linked_with,
            vtkSMPropertyLink::INPUT);
          link->AddLinkedProperty(
            proxy_list_proxy, name, vtkSMPropertyLink::OUTPUT);
          this->Internal->Links.push_back(link);
          link->Delete();
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqPipelineSource::setDefaultValues()
{
  vtkSMProxy* proxy = this->getProxy();
  vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    iter->GetProperty()->ResetToDefault();
    }
  iter->Delete();

  foreach(vtkSMProxy* dproxy, this->Internal->ProxyListDomainProxies)
    {
    vtkSMPropertyIterator* diter = dproxy->NewPropertyIterator();
    for (diter->Begin(); !diter->IsAtEnd(); diter->Next())
      {
      diter->GetProperty()->UpdateDependentDomains();
      }
    for (diter->Begin(); !diter->IsAtEnd(); diter->Next())
      {
      diter->GetProperty()->ResetToDefault();
      }
    diter->Delete();
    }
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getNumberOfConsumers() const
{
  return this->Internal->Consumers.size();
}

//-----------------------------------------------------------------------------
pqPipelineSource *pqPipelineSource::getConsumer(int index) const
{
  if(index >= 0 && index < this->Internal->Consumers.size())
    {
    return this->Internal->Consumers[index];
    }

  qCritical() << "Index " << index << " out of bounds.";
  return 0;
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getConsumerIndexFor(pqPipelineSource *consumer) const
{
  int index = 0;
  if(consumer)
    {
    foreach(pqPipelineSource *current, this->Internal->Consumers)
      {
      if(current == consumer)
        {
        return index;
        }
      index++;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
bool pqPipelineSource::hasConsumer(pqPipelineSource *consumer) const
{
  return (this->getConsumerIndexFor(consumer) != -1);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::addConsumer(pqPipelineSource* cons)
{
  emit this->preConnectionAdded(this, cons);
  this->Internal->Consumers.push_back(cons);

  // raise signals to let the world know which connections were
  // broken and which ones were made.
  emit this->connectionAdded(this, cons);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::removeConsumer(pqPipelineSource* cons)
{
  int index = this->Internal->Consumers.indexOf(cons);
  if (index != -1)
    {
    emit this->preConnectionRemoved(this, cons);
    this->Internal->Consumers.removeAt(index);
    // raise signals to let the world know which connections were
    // broken and which ones were made.
    emit this->connectionRemoved(this, cons);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineSource::addDisplay(pqConsumerDisplay* display)
{
  this->Internal->Displays.push_back(display);

  emit this->displayAdded(this, display);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::removeDisplay(pqConsumerDisplay* display)
{
  int index = this->Internal->Displays.indexOf(display);
  if (index != -1)
    {
    this->Internal->Displays.removeAt(index);
    }
  emit this->displayRemoved(this, display);
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getDisplayCount() const
{
  return this->Internal->Displays.size();
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqPipelineSource::getDisplay(int index) const
{
  if (index >= 0 && index < this->Internal->Displays.size())
    {
    return this->Internal->Displays[index];
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqPipelineSource::getDisplay(
  pqGenericViewModule* renderModule) const
{
  foreach(pqConsumerDisplay* disp, this->Internal->Displays)
    {
    if (disp && disp->shownIn(renderModule))
      {
      return disp;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
QList<pqGenericViewModule*> pqPipelineSource::getViewModules() const
{
  QList<pqGenericViewModule*> renModules;
 foreach(pqConsumerDisplay* disp, this->Internal->Displays)
    {
    if (disp)
      {
      unsigned int max = disp->getNumberOfViewModules();
      for (unsigned int cc=0; cc < max; ++cc)
        {
        pqGenericViewModule* ren = disp->getViewModule(cc);
        if (ren && !renModules.contains(ren))
          {
          renModules.push_back(ren);
          }
        }
      }
    }
  return renModules; 
}

//-----------------------------------------------------------------------------
void pqPipelineSource::renderAllViews(bool force /*=false*/)
{
  foreach(pqConsumerDisplay* disp, this->Internal->Displays)
    {
    if (disp)
      {
      disp->renderAllViews(force);
      }
    }
}
//-----------------------------------------------------------------------------
bool pqPipelineSource::replaceInput() const
{
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy)
    {
    return true;
    }

  vtkPVXMLElement* hints = proxy->GetHints();
  if (!hints)
    {
    return true;
    }
  for (unsigned int cc=0; cc < hints->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* child = hints->GetNestedElement(cc);
    if (!child || !child->GetName() || 
      strcmp(child->GetName(), "Visibility") != 0)
      {
      continue;
      }
    int replace_input = 1;
    if (!child->GetScalarAttribute("replace_input", &replace_input))
      {
      continue;
      }
    return (replace_input==1);
    }
  return true; // default value.
}
