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
#include "vtkSmartPointer.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

// Qt
#include <QList>
#include <QMap>
#include <QtDebug>

// ParaView
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"

//-----------------------------------------------------------------------------
class pqPipelineSourceInternal 
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;

  QString Name;
  QList<pqPipelineSource*> Consumers;
  QList<pqPipelineDisplay*> Displays;

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
  
  // Set the model item type.
  this->setType(pqPipelineModel::Source);
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
        }
      this->addInternalProxy(iter->GetKey(), new_proxy);
      }
    }
  iter->Delete();
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
void pqPipelineSource::addDisplay(pqPipelineDisplay* display)
{
  this->Internal->Displays.push_back(display);

  emit this->displayAdded(this, display);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::removeDisplay(pqPipelineDisplay* display)
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
pqPipelineDisplay* pqPipelineSource::getDisplay(int index) const
{
  if (index >= 0 && index < this->Internal->Displays.size())
    {
    return this->Internal->Displays[index];
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqPipelineDisplay* pqPipelineSource::getDisplay(
  pqRenderModule* renderModule) const
{
  foreach(pqPipelineDisplay* disp, this->Internal->Displays)
    {
    if (disp && disp->shownIn(renderModule))
      {
      return disp;
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqPipelineSource::renderAllViews(bool force /*=false*/)
{
  foreach(pqPipelineDisplay* disp, this->Internal->Displays)
    {
    if (disp)
      {
      disp->renderAllViews(force);
      }
    }
}
