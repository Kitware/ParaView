/*=========================================================================

   Program: ParaView
   Module:    pqPipelineSource.cxx

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

=========================================================================*/

/// \file pqPipelineSource.cxx
/// \date 4/17/2006

#include "pqPipelineSource.h"

// ParaView Server Manager.
#include "vtkClientServerStream.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
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
#include "pqHelperProxyRegisterUndoElement.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "pqXMLUtil.h"

//-----------------------------------------------------------------------------
class pqPipelineSourceInternal 
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;

  QString Name;
  QList<pqOutputPort*> OutputPorts;

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
  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(this->getProxy());
  if (source)
    {
    // Obtain information about number of output ports.
    // Number of output ports is valid even if the output port proxies haven't
    // been created (so long as CreateVTKObjects() has been called on the
    // proxy).
    source->UpdateVTKObjects(); // causes CreateVTKObjects() to be called.
    int numports = source->GetNumberOfOutputPorts();

    for (int cc=0; cc < numports; cc++)
      {
      pqOutputPort* op = new pqOutputPort(this, cc);

      // Relay all signals fired by the output ports
      QObject::connect(
        op, SIGNAL(connectionAdded(pqOutputPort*, pqPipelineSource*)),
        this, SLOT(portConnectionAdded(pqOutputPort*, pqPipelineSource*)));
      QObject::connect(
        op, SIGNAL(preConnectionAdded(pqOutputPort*, pqPipelineSource*)),
        this, SLOT(prePortConnectionAdded(pqOutputPort*, pqPipelineSource*)));
      QObject::connect(
        op, SIGNAL(connectionRemoved(pqOutputPort*, pqPipelineSource*)),
        this, SLOT(portConnectionRemoved(pqOutputPort*, pqPipelineSource*)));
      QObject::connect(
        op, SIGNAL(preConnectionRemoved(pqOutputPort*, pqPipelineSource*)),
        this, SLOT(prePortConnectionRemoved(pqOutputPort*, pqPipelineSource*)));

      QObject::connect(
        op, SIGNAL(representationAdded(pqOutputPort*, pqDataRepresentation*)),
        this, SLOT(portRepresentationAdded(pqOutputPort*, pqDataRepresentation*)));
      QObject::connect(
        op, SIGNAL(representationRemoved(pqOutputPort*, pqDataRepresentation*)),
        this, SLOT(portRepresentationRemoved(pqOutputPort*, pqDataRepresentation*)));
      QObject::connect(
        op, SIGNAL(visibilityChanged(pqOutputPort*, pqDataRepresentation*)),
        this, SLOT(portVisibilityChanged(pqOutputPort*, pqDataRepresentation*)));

      this->Internal->OutputPorts.push_back(op);
      }
    this->getConnector()->Connect(source, vtkCommand::UpdateDataEvent,
        this, SLOT(dataUpdated()));
    }
}

//-----------------------------------------------------------------------------
pqPipelineSource::~pqPipelineSource()
{
  foreach (pqOutputPort* opport, this->Internal->OutputPorts)
    {
    delete opport;
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPipelineSource::updatePipeline()
{
  pqTimeKeeper* timekeeper = this->getServer()->getTimeKeeper();
  double time = timekeeper->getTime();
  vtkSMSourceProxy::SafeDownCast(this->getProxy())->UpdatePipeline(time);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::dataUpdated()
{
  emit this->dataUpdated(this);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::prePortConnectionAdded(
  pqOutputPort* op, pqPipelineSource* cons)
{
  emit this->preConnectionAdded(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::portConnectionAdded(
  pqOutputPort* op, pqPipelineSource* cons)
{
  emit this->connectionAdded(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::prePortConnectionRemoved(
  pqOutputPort* op, pqPipelineSource* cons)
{
  emit this->preConnectionRemoved(this, cons, op->getPortNumber());
}
//-----------------------------------------------------------------------------
void pqPipelineSource::portConnectionRemoved(
  pqOutputPort* op, pqPipelineSource* cons)
{
  emit this->connectionRemoved(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::portRepresentationAdded(
  pqOutputPort* op, pqDataRepresentation* cons)
{
  emit this->representationAdded(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::portRepresentationRemoved(
  pqOutputPort* op, pqDataRepresentation* cons)
{
  emit this->representationRemoved(this, cons, op->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqPipelineSource::portVisibilityChanged(
  pqOutputPort* vtkNotUsed(op), pqDataRepresentation* cons)
{
  emit this->visibilityChanged(this, cons);
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getNumberOfOutputPorts() const
{
  return this->Internal->OutputPorts.size();
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
  vtkSMProxy* proxy = this->getProxy();
  vtkSMProxyManager* pxm = proxy->GetProxyManager();
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
  vtkSMProxy* proxy = this->getProxy();

  if (proxy)
    {
    // if any properties were changed e.g. by
    // createProxiesForProxyListDomains(), this will ensure that they are pushed
    // correctly.
    proxy->UpdateVTKObjects();
    }

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

    sp->CreateOutputPorts();
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

  this->createAnimationHelpersIfNeeded();

  // This is sort-of-a-hack to ensure that when this operation is redo, all the
  // helper proxies are discovered correctly. This needs to happen only after
  // all helper proxies have been created.
  pqHelperProxyRegisterUndoElement* elem = 
    pqHelperProxyRegisterUndoElement::New();
  elem->SetOperationTypeToRedo(); // Redo creation
  elem->RegisterHelperProxies(this);
  ADD_UNDO_ELEM(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void pqPipelineSource::createAnimationHelpersIfNeeded()
{
  QList<vtkSMProxy*> helpers = this->getHelperProxies("RepresentationAnimationHelper");
  if (helpers.size() == 0)
    {
    // Create animation helper which assists in animating display properties.
    vtkSMProxyManager* pxm = this->getProxy()->GetProxyManager();
    int numPorts = this->getNumberOfOutputPorts();
    for (int cc=0; cc < numPorts; cc++)
      {
      vtkSMProxy* helper = pxm->NewProxy("misc", "RepresentationAnimationHelper");
      vtkSMPropertyHelper(helper, "Source").Add(this->getProxy());
      helper->UpdateVTKObjects();
      this->addHelperProxy("RepresentationAnimationHelper", helper);
      helper->Delete();
      }
    }
}

//-----------------------------------------------------------------------------
QList<pqOutputPort*> pqPipelineSource::getOutputPorts() const
{
  QList<pqOutputPort*> ports;
  foreach (pqOutputPort* port, this->Internal->OutputPorts)
    {
    ports << port;
    }
  return ports;
}

//-----------------------------------------------------------------------------
pqOutputPort* pqPipelineSource::getOutputPort(int outputport) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
    {
    qCritical() << "Invalid output port : " << outputport
      << ". Available number of output ports: " 
      << this->Internal->OutputPorts.size();
    return NULL;
    }
  return this->Internal->OutputPorts[outputport];
}

//-----------------------------------------------------------------------------
pqOutputPort* pqPipelineSource::getOutputPort(const QString& name) const
{
  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(this->getProxy());
  unsigned int index = source->GetOutputPortIndex(name.toAscii().data());
  if (index != VTK_UNSIGNED_INT_MAX)
    {
    return this->getOutputPort(static_cast<int>(index));
    }

  return 0;
}

//-----------------------------------------------------------------------------
int pqPipelineSource::getNumberOfConsumers(int outputport) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
    {
    return 0;
    }

  return this->Internal->OutputPorts[outputport]->getNumberOfConsumers();
}

//-----------------------------------------------------------------------------
pqPipelineSource *pqPipelineSource::getConsumer(int outputport, int index) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
    {
    qCritical() << "Invalid output port : " << outputport
      << ". Available number of output ports: " 
      << this->Internal->OutputPorts.size();
    return NULL;
    }

  return this->Internal->OutputPorts[outputport]->getConsumer(index);
}

//-----------------------------------------------------------------------------
QList<pqPipelineSource*> pqPipelineSource::getAllConsumers() const
{
  QList<pqPipelineSource*> consumers;
  foreach (pqOutputPort* port, this->Internal->OutputPorts)
    {
    QList<pqPipelineSource*> portConsumers = port->getConsumers();
    for (int cc=0; cc < portConsumers.size(); cc++)
      {
      if (!consumers.contains(portConsumers[cc]))
        {
        consumers.push_back(portConsumers[cc]);
        }
      }
    }
  return consumers;
}

//-----------------------------------------------------------------------------
void pqPipelineSource::onRepresentationVisibilityChanged()
{
  emit this->visibilityChanged(this, 
    qobject_cast<pqDataRepresentation*>(this->sender()));
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqPipelineSource::getRepresentation(
  int outputport, pqView* view) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
    {
    qCritical() << "Invalid output port : " << outputport
      << ". Available number of output ports: " 
      << this->Internal->OutputPorts.size();
    return 0;
    }
  return this->Internal->OutputPorts[outputport]->getRepresentation(view);
}

//-----------------------------------------------------------------------------
QList<pqDataRepresentation*> pqPipelineSource::getRepresentations(
  int outputport, pqView* view) const
{
  if (outputport < 0 || outputport >= this->Internal->OutputPorts.size())
    {
    qCritical() << "Invalid output port : " << outputport
      << ". Available number of output ports: " 
      << this->Internal->OutputPorts.size();
    return QList<pqDataRepresentation*>();
    }

  return this->Internal->OutputPorts[outputport]->getRepresentations(view);
}

//-----------------------------------------------------------------------------
QList<pqView*> pqPipelineSource::getViews() const
{
  QSet<pqView*> views;

  foreach (pqOutputPort* opPort, this->Internal->OutputPorts)
    {
    views.unite(QSet<pqView*>::fromList(
        opPort->getViews()));
    }

  return QList<pqView*>::fromSet(views);
}

//-----------------------------------------------------------------------------
void pqPipelineSource::renderAllViews(bool force /*=false*/)
{
  foreach (pqOutputPort* opPort, this->Internal->OutputPorts)
    {
    opPort->renderAllViews(force);
    }
}

