/*=========================================================================

   Program: ParaView
   Module:    pqObjectBuilder.cxx

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

========================================================================*/
#include "pqObjectBuilder.h"

#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputProperty.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"

#include <QtDebug>
#include <QFileInfo>

#include "pqApplicationCore.h"
#include "pqConsumerDisplay.h"
#include "pqDisplayPolicy.h"
#include "pqNameCount.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPluginManager.h"
#include "pqRenderViewModule.h"
#include "pqScalarBarDisplay.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqViewModuleInterface.h"

//-----------------------------------------------------------------------------
pqObjectBuilder::pqObjectBuilder(QObject* _parent/*=0*/) :QObject(_parent)
{
  this->NameGenerator = new pqNameCount();
}

//-----------------------------------------------------------------------------
pqObjectBuilder::~pqObjectBuilder()
{
  delete this->NameGenerator;
}


//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createSource(const QString& sm_group,
    const QString& sm_name, pqServer* server)
{
  vtkSMProxy* proxy = 
    this->createProxyInternal(sm_group, sm_name, server, "sources");
  if (proxy)
    {
    pqPipelineSource* source = pqApplicationCore::instance()->
      getServerManagerModel()->getPQSource(proxy);

    // initialize the source.
    source->setDefaultPropertyValues();

    emit this->sourceCreated(source);
    emit this->proxyCreated(source);
    return source;
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createFilter(const QString& sm_group,
    const QString& sm_name, const QList<pqPipelineSource*> &inputs)
{
  pqPipelineSource* src0 = inputs[0];
  this->blockSignals(true);
  pqPipelineSource* filter = this->createFilter(sm_group, sm_name, src0);
  this->blockSignals(false);
  if (filter)
    {
    vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
      filter->getProxy()->GetProperty("Input"));
    if (inputProp->GetMultipleInput())
      {
      for (int cc=1; cc< inputs.size(); cc++)
        {
        this->addConnection(inputs[cc], filter);
        }
      }
    emit this->filterCreated(filter);
    emit this->proxyCreated(filter);
    }
  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createFilter(const QString& sm_group,
    const QString& sm_name, pqPipelineSource* input)
{
  if (!input)
    {
    qWarning() << "Cannot create filter with no input.";
    return 0;
    }
 
  vtkSMProxy* proxy = 
    this->createProxyInternal(sm_group, sm_name, input->getServer(), "sources");
  if (!proxy)
    {
    return 0;
    }

  pqPipelineSource* filter = qobject_cast<pqPipelineSource*>(
    pqApplicationCore::instance()->getServerManagerModel()->getPQSource(proxy));
  if (!filter)
    {
    qDebug() << "Failed to locate pqPipelineSource for the created proxy "
      << sm_group << ", " << sm_name;
    return 0;
    }
  vtkSMProperty* inputProperty = proxy->GetProperty("Input");
  if (inputProperty)
    {
    pqSMAdaptor::setProxyProperty(inputProperty, input->getProxy());
    proxy->UpdateVTKObjects();
    inputProperty->UpdateDependentDomains();
    }

  // Set default property values.
  filter->setDefaultPropertyValues();

  emit this->filterCreated(filter);
  emit this->proxyCreated(filter);
  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createCustomFilter(const QString& sm_name,
    pqServer* server, pqPipelineSource* input/*=0*/)
{
  vtkSMProxy* proxy = 
    this->createProxyInternal(QString(), sm_name, server, "sources");
  if (!proxy)
    {
    return 0;
    }

  pqPipelineSource* filter = qobject_cast<pqPipelineSource*>(
    pqApplicationCore::instance()->getServerManagerModel()->getPQSource(proxy));
  if (!filter)
    {
    qDebug() << "Failed to locate pqPipelineSource for the created custom filter proxy "
      << sm_name;
    return 0;
    }

  vtkSMProperty* inputProperty = proxy->GetProperty("Input");
  if (inputProperty && input)
    {
    pqSMAdaptor::setProxyProperty(inputProperty, input->getProxy());
    proxy->UpdateVTKObjects();
    inputProperty->UpdateDependentDomains();
    }

  // Set default property values.
  filter->setDefaultPropertyValues();
  emit this->customFilterCreated(filter);
  emit this->proxyCreated(filter);
  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createReader(const QString& sm_group,
    const QString& sm_name, const QString& filename, pqServer* server)
{
  QFileInfo fileInfo(filename);

  vtkSMProxy* proxy = 
    this->createProxyInternal(sm_group, sm_name, server, "sources", 
      fileInfo.fileName());
  if (!proxy)
    {
    return 0;
    }

  pqPipelineSource* reader = qobject_cast<pqPipelineSource*>(
    pqApplicationCore::instance()->getServerManagerModel()->getPQSource(proxy));
  if (!reader)
    {
    qDebug() << "Failed to locate pqPipelineSource for the created proxy "
      << sm_group << ", " << sm_name;
    return 0;
    }

  QString pname = this->getFileNamePropertyName(proxy);
  if (!pname.isEmpty())
    {
    vtkSMProperty* prop = proxy->GetProperty(pname.toAscii().data());
    pqSMAdaptor::setElementProperty(prop, filename);
    proxy->UpdateVTKObjects();
    prop->UpdateDependentDomains();
    }
  reader->setDefaultPropertyValues();

  emit this->readerCreated(reader, filename);
  emit this->proxyCreated(reader);
  return reader;
}
//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqPipelineSource* source)
{
  if (!source)
    {
    qDebug() << "Cannot remove null source.";
    return;
    }

  if (source->getNumberOfConsumers())
    {
    qDebug() << "Cannot remove source with consumers.";
    return;
    }

  emit this->destroying(source);

  // * remove inputs.
  // TODO: this step should not be necessary, but it currently
  // is :(. Needs some looking into.
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    source->getProxy()->GetProperty("Input"));
  if (pp)
    {
    pp->RemoveAllProxies();
    }

  // * remove all displays.
  while (source->getDisplayCount())
    {
    this->destroy(source->getDisplay(0));
    }

  // * Unregister proxy.
  this->destroyProxyInternal(source);
}

//-----------------------------------------------------------------------------
pqGenericViewModule* pqObjectBuilder::createView(const QString& type,
  pqServer* server)
{
  if (!server)
    {
    qDebug() << "Cannot create view without server.";
    return NULL;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy= 0;
  if(type == pqRenderViewModule::renderViewType())
    {
    proxy = server->newRenderModule();
    }
  else
    {
    QObjectList ifaces =
      pqApplicationCore::instance()->getPluginManager()->interfaces();
    foreach(QObject* iface, ifaces)
      {
      pqViewModuleInterface* vmi = qobject_cast<pqViewModuleInterface*>(iface);
      if(vmi && vmi->viewTypes().contains(type))
        {
        proxy = vmi->createViewProxy(type);
        break;
        }
      }
    }

  if (!proxy)
    {
    qDebug() << "Failed to create a proxy for the requested view type.";
    return NULL;
    }

  proxy->SetConnectionID(server->GetConnectionID());

  QString name = QString("%1%2").arg(proxy->GetXMLName()).arg(
    this->NameGenerator->GetCountAndIncrement(proxy->GetXMLName()));
  pxm->RegisterProxy("view_modules", name.toAscii().data(), proxy);
  proxy->Delete();

  if (type == pqRenderViewModule::renderViewType())
    {
    pqSMAdaptor::addProxyProperty(
      server->GetRenderModule()->GetProperty("RenderModules"),
      proxy);
    server->GetRenderModule()->UpdateProperty("RenderModules");
    }

  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqGenericViewModule* view = qobject_cast<pqGenericViewModule*>(
    model->getPQProxy(proxy));
  if (view)
    {
    view->setDefaultPropertyValues();
    emit this->viewCreated(view);
    emit this->proxyCreated(view);
    }
  else
    {
    qDebug() << "Cannot locate the pqGenericViewModule for the " 
      << "view module proxy.";
    }

  return view;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqGenericViewModule* view)
{
  if (!view)
    {
    return;
    }

  emit this->destroying(view);

  // Get a list of all displays belonging to this render module. We delete
  // all the displays that belong only to this render module.
  QList<pqDisplay*> displays = view->getDisplays();

  // Unregister the proxy....the rest of the GUI will(rather should) manage itself!
  QString name = view->getSMName();
  vtkSMProxy* proxy = view->getProxy();

  if (qobject_cast<pqRenderViewModule*>(view))
    {
    // remove the render module from the multiview proxy.
    vtkSMMultiViewRenderModuleProxy* multiRM = view->getServer()->GetRenderModule();  
    pqSMAdaptor::removeProxyProperty(multiRM->GetProperty("RenderModules"), proxy);
    multiRM->UpdateProperty("RenderModules");
    }

  this->destroyProxyInternal(view);

  // Now clean up any orphan displays.
  foreach (pqDisplay* disp, displays)
    {
    if (disp && disp->getNumberOfViewModules() == 0)
      {
      this->destroyProxyInternal(disp);      
      }
    }
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqObjectBuilder::createDataDisplay(pqPipelineSource* source,
  pqGenericViewModule* view)
{
  if (!source|| !view)
    {
    qCritical() <<"Missing required attribute.";
    return NULL;
    }

  // FIXME: This class should not refer to any policies.
  pqApplicationCore* core= pqApplicationCore::instance();
  pqDisplayPolicy* policy = core->getDisplayPolicy();
  vtkSMProxy* displayProxy = policy? policy->newDisplayProxy(source, view) : 0; 
  if (!displayProxy)
    {
    return NULL;
    }

  
  // (of undo/redo to work).
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  QString name = QString("DataDisplay%1").arg(
    this->NameGenerator->GetCountAndIncrement("DataDisplay"));
  pxm->RegisterProxy("displays", name.toAscii().data(), displayProxy);
  displayProxy->Delete();

  vtkSMProxy* viewModuleProxy = view->getProxy();

  // Set the displayProxy's input.
  pqSMAdaptor::setProxyProperty(
    displayProxy->GetProperty("Input"), source->getProxy());
  displayProxy->UpdateVTKObjects();

  // Add the displayProxy to render module.
  pqSMAdaptor::addProxyProperty(
    viewModuleProxy->GetProperty("Displays"), displayProxy);
  viewModuleProxy->UpdateVTKObjects();

  pqConsumerDisplay* display = core->getServerManagerModel()->getPQDisplay(
    displayProxy);
  if (display)
    {
    display->setDefaultPropertyValues();

    emit this->dataDisplayCreated(display);
    emit this->proxyCreated(display);
    }
  return display;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqDisplay* display)
{
  if (!display)
    {
    return;
    }

  emit this->destroying(display);

  vtkSMProxy* displayProxy = display->getProxy();

  // Remove display from the render module.
  unsigned int num_views = display->getNumberOfViewModules();
  for(unsigned int i=0; i < num_views ; i++)
    {
    pqGenericViewModule* view = display->getViewModule(i);
    if (!view)
      {
      continue;
      }
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      view->getProxy()->GetProperty("Displays"));
    pp->RemoveProxy(displayProxy);
    view->getProxy()->UpdateVTKObjects();
    }

  // If this display has a lookuptable, we hide all scalar bars for that
  // lookup table unless there is some other display that's using it.
  pqScalarsToColors* stc =0;
  if (pqConsumerDisplay* cd = qobject_cast<pqConsumerDisplay*>(display))
    {
    stc = cd->getLookupTable();
    }

  this->destroyProxyInternal(display);

  if (stc)
    {
    // this hides scalar bars only if the LUT is not used by
    // any other display. This must happen after the display has 
    // been deleted.
    stc->hideUnusedScalarBars();
    }
}

//-----------------------------------------------------------------------------
pqScalarBarDisplay* pqObjectBuilder::createScalarBarDisplay(
    pqScalarsToColors* lookupTable, pqGenericViewModule* view)
{
  if (!lookupTable || !view)
    {
    return 0;
    }

  if (lookupTable->getServer() != view->getServer())
    {
    qCritical() << "LUT and View are on different servers!";
    return 0;
    }

  pqServer* server = view->getServer();
  vtkSMProxy* scalarBarProxy = this->createProxyInternal(
    "displays", "ScalarBarWidget", server, "scalar_bars");

  if (!scalarBarProxy)
    {
    return 0;
    }
  pqScalarBarDisplay* scalarBar = qobject_cast<pqScalarBarDisplay*>(
    pqServerManagerModel::instance()->getPQProxy(scalarBarProxy));
  pqSMAdaptor::setProxyProperty(scalarBarProxy->GetProperty("LookupTable"),
    lookupTable->getProxy());
  scalarBarProxy->UpdateVTKObjects();

  pqSMAdaptor::addProxyProperty(view->getProxy()->GetProperty("Displays"),
    scalarBarProxy);
  view->getProxy()->UpdateVTKObjects();
  scalarBar->setDefaultPropertyValues();

  emit this->scalarBarDisplayCreated(scalarBar);
  emit this->proxyCreated(scalarBar);
  return scalarBar;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::addConnection(pqPipelineSource* source, 
  pqPipelineSource* sink)
{
  if(!source || !sink)
    {
    qCritical() << "Cannot addConnection. source or sink missing.";
    return;
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
    sink->getProxy()->GetProperty("Input"));
  if(inputProp)
    {
    // If the sink already has an input, the previous connection
    // needs to be broken if it doesn't support multiple inputs.
    if(!inputProp->GetMultipleInput() && inputProp->GetNumberOfProxies() > 0)
      {
      inputProp->RemoveAllProxies();
      }

    // Add the input to the proxy in the server manager.
    inputProp->AddProxy(source->getProxy());
    }
  else
    {
    qCritical() << "Failed to locate property Input on proxy:" 
      << source->getProxy()->GetXMLGroup() << ", " << source->getProxy()->GetXMLName();
    }
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::removeConnection(pqPipelineSource* pqsource,
  pqPipelineSource* pqsink)
{
  vtkSMCompoundProxy *compoundProxy =
    vtkSMCompoundProxy::SafeDownCast(pqsource->getProxy());

  vtkSMProxy* source = pqsource->getProxy();
  vtkSMProxy* sink = pqsink->getProxy();

  if(compoundProxy)
    {
    // TODO: How to find the correct output proxy?
    source = 0;
    for(int i = compoundProxy->GetNumberOfProxies(); source == 0 && i > 0; i--)
      {
      source = vtkSMSourceProxy::SafeDownCast(compoundProxy->GetProxy(i-1));
      }
    }

  compoundProxy = vtkSMCompoundProxy::SafeDownCast(sink);
  if(compoundProxy)
    {
    // TODO: How to find the correct input proxy?
    sink = compoundProxy->GetMainProxy();
    }

  if(!source || !sink)
    {
    qCritical() << "Cannot removeConnection. source or sink missing.";
    return;
    }

  vtkSMProxyProperty* inputProp = vtkSMProxyProperty::SafeDownCast(
    sink->GetProperty("Input"));
  if(inputProp)
    {
    // Remove the input from the server manager.
    inputProp->RemoveProxy(source);
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqObjectBuilder::createProxy(const QString& sm_group, 
    const QString& sm_name, pqServer* server, 
    const QString& reg_group, const QString& reg_name/*=QString()*/)
{
  vtkSMProxy* proxy = this->createProxyInternal(
    sm_group, sm_name, server, reg_group, reg_name);
  if (proxy)
    {
    emit this->proxyCreated(proxy);
    }
  return proxy;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqProxy* proxy)
{
  emit this->destroying(proxy);

  this->destroyProxyInternal(proxy);
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroySources(pqServer* server)
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqObjectBuilder* builder =
    pqApplicationCore::instance()->getObjectBuilder();

  QList<pqPipelineSource*> sources = model->getSources(server);
  while(!sources.isEmpty())
    {
    for(int i=0; i<sources.size(); i++)
      {
      if(sources[i]->getNumberOfConsumers() == 0)
        {
        builder->destroy(sources[i]);
        sources[i] = NULL;
        }
      }
    sources.removeAll(NULL);
    }
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroySources()
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  QList<pqServer*> servers = model->getServers();
  foreach(pqServer* server, servers)
    {
    this->destroySources(server);
    }
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyAllProxies(pqServer* server)
{
  if (!server)
    {
    qDebug() << "Server cannot be NULL.";
    return;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UnRegisterProxies(server->GetConnectionID());
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqObjectBuilder::createProxyInternal(const QString& sm_group, 
  const QString& sm_name, pqServer* server, 
  const QString& reg_group, const QString& reg_name/*=QString()*/)
{
  if (!server)
    {
    qDebug() << "server cannot be null";
    return 0;
    }

  QString actual_regname = reg_name;
  if (reg_name.isEmpty())
    {
    actual_regname = QString("%1%2").arg(sm_name).arg(
      this->NameGenerator->GetCountAndIncrement(sm_name));
    }

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  if (sm_group.isEmpty())
    {
    proxy.TakeReference(pxm->NewCompoundProxy(sm_name.toAscii().data()));
    }
  else
    {
    proxy.TakeReference(
      pxm->NewProxy(sm_group.toAscii().data(), sm_name.toAscii().data()));
    }

  if (!proxy.GetPointer())
    {
    qCritical() << "Failed to create proxy: " << sm_group << ", " << sm_name;
    return 0;
    }
  proxy->SetConnectionID(server->GetConnectionID());

  pxm->RegisterProxy(reg_group.toAscii().data(), 
    actual_regname.toAscii().data(), proxy);
  return proxy;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyProxyInternal(pqProxy* proxy)
{
  if (proxy)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    pxm->UnRegisterProxy(proxy->getSMGroup().toAscii().data(), 
      proxy->getSMName().toAscii().data(), proxy->getProxy());
    }
}

//-----------------------------------------------------------------------------
QString pqObjectBuilder::getFileNamePropertyName(vtkSMProxy* proxy) const
{
  // Find the first property that has a vtkSMFileListDomain. Assume that
  // it is the property used to set the filename.
  vtkSmartPointer<vtkSMPropertyIterator> piter;
  piter.TakeReference(proxy->NewPropertyIterator());
  piter->Begin();
  while(!piter->IsAtEnd())
    {
    vtkSMProperty* prop = piter->GetProperty();
    if (prop->IsA("vtkSMStringVectorProperty"))
      {
      vtkSmartPointer<vtkSMDomainIterator> diter;
      diter.TakeReference(prop->NewDomainIterator());
      diter->Begin();
      while(!diter->IsAtEnd())
        {
        if (diter->GetDomain()->IsA("vtkSMFileListDomain"))
          {
          return piter->GetKey();
          }
        diter->Next();
        }
      if (!diter->IsAtEnd())
        {
        break;
        }
      }
    piter->Next();
    }

  return QString::Null();
}
