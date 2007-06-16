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
#include "vtkClientServerStream.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

// Qt
#include <QList>
#include <QMap>
#include <QtDebug>

// ParaView
#include "pqDataRepresentation.h"
#include "pqGenericViewModule.h"
#include "pqPipelineFilter.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqTimeKeeper.h"
#include "pqXMLUtil.h"

//-----------------------------------------------------------------------------
class pqPipelineSourceInternal 
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;

  QString Name;
  QList<pqPipelineSource*> Consumers;
  QList<pqDataRepresentation*> Representations;

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
  QObject::connect(this, SIGNAL(representationAdded(
        pqPipelineSource*, pqDataRepresentation*)),
    this, SIGNAL(visibilityChanged(pqPipelineSource*, pqDataRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(
        pqPipelineSource*, pqDataRepresentation*)),
    this, SIGNAL(visibilityChanged(pqPipelineSource*, pqDataRepresentation*)));
}

//-----------------------------------------------------------------------------
pqPipelineSource::~pqPipelineSource()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
// Overridden to add the proxies to the domain as well.
void pqPipelineSource::addHelperProxy(const QString& key, vtkSMProxy* helper)
{
  this->Superclass::addHelperProxy(key, helper);

  vtkSMProperty* prop = this->getProxy()->GetProperty(key.toAscii().data());
  if (prop)
    {
     vtkSMProxyListDomain* pld = vtkSMProxyListDomain::SafeDownCast(
      prop->GetDomain("proxy_list"));
    if (pld && !pld->HasProxy(helper))
      {
      pld->AddProxy(helper);
      }
    }
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

    QList<vtkSmartPointer<vtkSMProxy> > domainProxies;
    for (unsigned int cc=0; cc < pld->GetNumberOfProxies(); cc++)
      {
      domainProxies.push_back(pld->GetProxy(cc));
      }

    unsigned int max = pld->GetNumberOfProxyTypes();
    for (unsigned int cc=0; cc < max; cc++)
      {
      QString proxy_group = pld->GetProxyGroup(cc);
      QString proxy_type = pld->GetProxyName(cc);


      // check if a proxy of the indicated type already exists in the domain,
      // we use it, if present.
      foreach(vtkSMProxy* temp_proxy, domainProxies)
        {
        if (temp_proxy && proxy_group == temp_proxy->GetXMLGroup() 
          && proxy_type == temp_proxy->GetXMLName())
          {
          continue;
          }
        }

      vtkSmartPointer<vtkSMProxy> new_proxy;
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
      domainProxies.push_back(new_proxy);
      }

    foreach (vtkSMProxy* domainProxy, domainProxies)
      {
      this->addHelperProxy(iter->GetKey(), domainProxy);

      this->processProxyListHints(domainProxy);
      this->Internal->ProxyListDomainProxies.push_back(domainProxy);
      }
    // This ensures that the property is initialized using one of the proxies
    // in the domains.
    // NOTE: This method is called only in setDefaultPropertyValues()
    // ensure that we are not changing any user set values.
    prop->ResetToDefault();
    }
  iter->Delete();
}


//-----------------------------------------------------------------------------
// ProxyList hints are processed for any proxy, when it becomes a part of a 
// proxy list domain. It provides mechanism to link certain properties
// of the proxy (which is added in the proxy list domain) with properties of 
// the proxy which has the property with the proxy list domain.
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
void pqPipelineSource::setDefaultPropertyValues()
{
  // Create any internal proxies needed by any property
  // that has a vtkSMProxyListDomain.
  this->createProxiesForProxyListDomains();

  vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(this->getProxy());
  if (sp)
    {
    // this updates the information which may be required to determine
    // defaults of some property values (typically in case of readers).
    // For this call to not produce VTK pipeline errors, it is 
    // essential that necessary properties on the proxy are already
    // initialized correctly eg. FileName in case of readers,
    // all Input/Source properties in case filters etc.
    sp->UpdatePipelineInformation();
    }

  this->Superclass::setDefaultPropertyValues();

  // Now initialize the proxies in the proxy list domains as well. 
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
void pqPipelineSource::addRepresentation(pqDataRepresentation* repr)
{
  this->Internal->Representations.push_back(repr);
  QObject::connect(repr, SIGNAL(visibilityChanged(bool)),
    this, SLOT(onRepresentationVisibilityChanged()));
  emit this->representationAdded(this, repr);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::removeRepresentation(pqDataRepresentation* repr)
{
  this->Internal->Representations.removeAll(repr);
  QObject::disconnect(repr, 0, this, 0);
  emit this->representationRemoved(this, repr);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::onRepresentationVisibilityChanged()
{
  emit this->visibilityChanged(this, 
    qobject_cast<pqDataRepresentation*>(this->sender()));
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqPipelineSource::getRepresentation(pqView* view) const
{
  if (view)
    {
    foreach (pqDataRepresentation* repr, this->Internal->Representations)
      {
      if (repr && (!view || repr->getView() == view))
        {
        return repr;
        }
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
QList<pqDataRepresentation*> pqPipelineSource::getRepresentations(pqView* view) const
{
  QList<pqDataRepresentation*> list;
  foreach (pqDataRepresentation* repr, this->Internal->Representations)
    {
    if (repr && (!view || repr->getView() == view))
      {
      list.push_back(repr);
      }
    }

  return list;
}

//-----------------------------------------------------------------------------
QList<pqView*> pqPipelineSource::getViews() const
{
  QList<pqView*> views;
  foreach(pqDataRepresentation* repr, this->Internal->Representations)
    {
    if (repr)
      {
      pqView* view= repr->getView();
      if (view && !views.contains(view))
        {
        views.push_back(view);
        }
      }
    }

  return views; 
}

//-----------------------------------------------------------------------------
void pqPipelineSource::renderAllViews(bool force /*=false*/)
{
  foreach(pqDataRepresentation* disp, this->Internal->Representations)
    {
    if (disp)
      {
      disp->renderView(force);
      }
    }
}

//-----------------------------------------------------------------------------
int pqPipelineSource::replaceInput() const
{
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy)
    {
    return 1;
    }

  vtkPVXMLElement* hints = proxy->GetHints();
  if (!hints)
    {
    return 1;
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
    return replace_input;
    }
  return 1; // default value.
}


//-----------------------------------------------------------------------------
vtkPVDataInformation* pqPipelineSource::getDataInformation() const
{
  vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(
    this->getProxy());
  // If parts haven't been created, it implies that the
  // source has never been accepted. In that case
  // forcing a pipeline update can raise errors, hence we dont 
  // try to get any data information.
  if (!proxy || proxy->GetNumberOfParts() == 0)
    {
    return 0;
    }

  if (this->Internal->Representations.size() > 0 || proxy->GetDataInformationValid())
    {
    return proxy->GetDataInformation();
    }

  pqTimeKeeper* timekeeper = this->getServer()->getTimeKeeper();
  double time = timekeeper->getTime();
  proxy->UpdatePipeline(time);
  return proxy->GetDataInformation();
}
