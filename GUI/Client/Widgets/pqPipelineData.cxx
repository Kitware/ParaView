/*=========================================================================

   Program:   ParaQ
   Module:    pqPipelineData.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqPipelineData.h"

#include "pqMultiView.h"
#include "pqMultiViewFrame.h"
#include "pqNameCount.h"
#include "pqParts.h"
#include "pqPipelineModel.h"
#include "pqPipelineObject.h"
#include "pqPipelineServer.h"
#include "pqServer.h"
#include "pqSMMultiView.h"
#include "pqXMLUtil.h"

#include "QVTKWidget.h"
#include "vtkObject.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"

#include <QList>
#include <QMap>


class pqPipelineDataInternal
{
public:
  pqPipelineDataInternal();
  ~pqPipelineDataInternal() {}

  QMap<QVTKWidget *, vtkSMRenderModuleProxy *> ViewMap;
  QMap<vtkSMRenderModuleProxy *, QVTKWidget *> RenderMap;
  QMap<QString, QWidget *> RestoreMap;
};


pqPipelineDataInternal::pqPipelineDataInternal()
  : ViewMap(), RenderMap(), RestoreMap()
{
}


pqPipelineData *pqPipelineData::Instance = 0;

pqPipelineData::pqPipelineData(QObject* p)
  : QObject(p), CurrentProxy(NULL)
{
  this->Internal = new pqPipelineDataInternal();
  this->Model = new pqPipelineModel(this);
  this->Names = new pqNameCount();
  this->InCreateOrConnect = false;
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();

  if(!pqPipelineData::Instance)
    {
    pqPipelineData::Instance = this;
    }

  // Connect the pipeline model to the modification signals.
  if(this->Model)
    {
    QObject::connect(this, SIGNAL(serverAdded(pqServer *)), this->Model,
        SLOT(addServer(pqServer *)));
    QObject::connect(this, SIGNAL(removingServer(pqServer *)), this->Model,
        SLOT(removeServer(pqServer *)));
    QObject::connect(this,
        SIGNAL(sourceCreated(vtkSMProxy *, const QString &, pqServer *)),
        this->Model,
        SLOT(addSource(vtkSMProxy *, const QString &, pqServer *)));
    QObject::connect(this,
        SIGNAL(filterCreated(vtkSMProxy *, const QString &, pqServer *)),
        this->Model,
        SLOT(addFilter(vtkSMProxy *, const QString &, pqServer *)));
    QObject::connect(this,
        SIGNAL(bundleCreated(vtkSMProxy *, const QString &, pqServer *)),
        this->Model,
        SLOT(addBundle(vtkSMProxy *, const QString &, pqServer *)));
    QObject::connect(this, SIGNAL(removingProxy(vtkSMProxy *)), this->Model,
        SLOT(removeObject(vtkSMProxy *)));
    QObject::connect(this,
        SIGNAL(displayCreated(vtkSMDisplayProxy *, const QString &, vtkSMProxy *, vtkSMRenderModuleProxy *)),
        this->Model,
        SLOT(addDisplay(vtkSMDisplayProxy *, const QString &, vtkSMProxy *, vtkSMRenderModuleProxy *)));
    QObject::connect(this,
        SIGNAL(removingDisplay(vtkSMDisplayProxy *, vtkSMProxy *)),
        this->Model, SLOT(removeDisplay(vtkSMDisplayProxy *, vtkSMProxy *)));
    QObject::connect(this,
        SIGNAL(connectionCreated(vtkSMProxy *, vtkSMProxy *)), this->Model,
        SLOT(addConnection(vtkSMProxy *, vtkSMProxy *)));
    QObject::connect(this,
        SIGNAL(removingConnection(vtkSMProxy *, vtkSMProxy *)), this->Model,
        SLOT(removeConnection(vtkSMProxy *, vtkSMProxy *)));
    }
}

pqPipelineData::~pqPipelineData()
{
  // Clean up the pipelines before removing the instance so the objects
  // can be unregistered.
  if(this->Model)
    {
    QObject::disconnect(this, 0, this->Model, 0);
    this->Model->clearPipelines();
    }

  if(pqPipelineData::Instance == this)
    {
    pqPipelineData::Instance = 0;
    }

  if(this->Names)
    {
    delete this->Names;
    this->Names = 0;
    }

  if(this->Model)
    {
    delete this->Model;
    this->Model = 0;
    }

  if(this->Internal)
    {
    delete this->Internal;
    this->Internal = 0;
    }
}

pqPipelineData *pqPipelineData::instance()
{
  return pqPipelineData::Instance;
}

void pqPipelineData::addServer(pqServer *server)
{
  // Listen for pipeline events from the server.
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
  this->VTKConnect->Connect(proxyManager, vtkCommand::RegisterEvent, this,
      SLOT(proxyRegistered(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
      NULL, 1.0);

  // Signal that a new server was added to the pipeline data.
  emit this->serverAdded(server);
}

void pqPipelineData::removeServer(pqServer *server)
{
  if(server)
    {
    emit this->removingServer(server);
    }
}

void pqPipelineData::addViewMapping(QVTKWidget *widget,
    vtkSMRenderModuleProxy *module)
{
  if(this->Internal && widget && module)
    {
    this->Internal->ViewMap.insert(widget, module);
    this->Internal->RenderMap.insert(module, widget);
    }
}

vtkSMRenderModuleProxy *pqPipelineData::removeViewMapping(QVTKWidget *widget)
{
  vtkSMRenderModuleProxy *module = 0;
  if(this->Internal && widget)
    {
    QMap<QVTKWidget *, vtkSMRenderModuleProxy *>::Iterator iter =
        this->Internal->ViewMap.find(widget);
    if(iter != this->Internal->ViewMap.end())
      {
      module = *iter;
      this->Internal->ViewMap.erase(iter);
      }

    if(module)
      {
      QMap<vtkSMRenderModuleProxy *, QVTKWidget *>::Iterator jter =
          this->Internal->RenderMap.find(module);
      if(jter != this->Internal->RenderMap.end())
        {
        this->Internal->RenderMap.erase(jter);
        }
      }
    }

  return module;
}

vtkSMRenderModuleProxy *pqPipelineData::getRenderModule(
    QVTKWidget *widget) const
{
  if(this->Internal && widget)
    {
    QMap<QVTKWidget *, vtkSMRenderModuleProxy *>::Iterator iter =
        this->Internal->ViewMap.find(widget);
    if(iter != this->Internal->ViewMap.end())
      {
      return *iter;
      }
    }

  return 0;
}

QVTKWidget *pqPipelineData::getRenderWindow(
    vtkSMRenderModuleProxy *module) const
{
  if(this->Internal && module)
    {
    QMap<vtkSMRenderModuleProxy *, QVTKWidget *>::Iterator iter =
        this->Internal->RenderMap.find(module);
    if(iter != this->Internal->RenderMap.end())
      {
      return *iter;
      }
    }

  return 0;
}

void pqPipelineData::clearViewMapping()
{
  if(this->Internal)
    {
    this->Internal->ViewMap.clear();
    this->Internal->RenderMap.clear();
    }
}

vtkSMProxy *pqPipelineData::createAndRegisterSource(const char *proxyName,
    pqServer *server)
{
  if(!this->Names || !proxyName || !server)
    {
    return 0;
    }

  // Create a proxy object on the server.
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  vtkSMProxy *proxy = proxyManager->NewProxy("sources", proxyName);
  proxy->SetConnectionID(server->GetConnectionID());

  // Register the proxy with the server manager. Use a unique name
  // based on the class name and a count.
  QString name;
  name.setNum(this->Names->GetCountAndIncrement(proxyName));
  name.prepend(proxyName);
  this->InCreateOrConnect = true;
  proxyManager->RegisterProxy("sources", name.toAscii().data(), proxy);
  this->InCreateOrConnect = false;
  proxy->Delete();

  // Let other objects know a proxy was created.
  emit this->sourceCreated(proxy, name, server);

  this->setCurrentProxy(proxy);
  return proxy;
}

vtkSMProxy *pqPipelineData::createAndRegisterFilter(const char *proxyName,
    pqServer *server)
{
  if(!this->Names || !proxyName || !server)
    {
    return 0;
    }

  // Create a proxy object on the server.
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  vtkSMProxy *proxy = proxyManager->NewProxy("filters", proxyName);
  proxy->SetConnectionID(server->GetConnectionID());

  // Register the proxy with the server manager. Use a unique name
  // based on the class name and a count.
  QString name;
  name.setNum(this->Names->GetCountAndIncrement(proxyName));
  name.prepend(proxyName);
  this->InCreateOrConnect = true;
  proxyManager->RegisterProxy("sources", name.toAscii().data(), proxy);
  this->InCreateOrConnect = false;
  proxy->Delete();

  // Let other objects know a proxy was created.
  emit this->filterCreated(proxy, name, server);

  this->setCurrentProxy(proxy);
  return proxy;
}

vtkSMProxy *pqPipelineData::createAndRegisterBundle(const char *proxyName,
    pqServer *server)
{
  if(!this->Names || !proxyName || !server)
    {
    return 0;
    }

  // Create a proxy object on the server.
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  vtkSMCompoundProxy *proxy = proxyManager->NewCompoundProxy(proxyName);
  proxy->SetConnectionID(server->GetConnectionID());
  proxy->UpdateVTKObjects();

  // Register the proxy with the server manager. Use a unique name
  // based on the class name and a count.
  QString name;
  name.setNum(this->Names->GetCountAndIncrement(proxyName));
  name.prepend(proxyName);
  this->InCreateOrConnect = true;
  proxyManager->RegisterProxy("compound_proxies", name.toAscii().data(), proxy);
  this->InCreateOrConnect = false;
  proxy->Delete();

  // Let other objects know a proxy was created.
  emit this->bundleCreated(proxy, name, server);

  this->setCurrentProxy(proxy);
  return proxy;
}

void pqPipelineData::unregisterProxy(vtkSMProxy *proxy, const char *name)
{
  emit this->removingProxy(proxy);

  // Unregister the source proxy.
  if(name)
    {
    vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
    proxyManager->UnRegisterProxy("sources", name);
    }
}

vtkSMDisplayProxy *pqPipelineData::createAndRegisterDisplay(vtkSMProxy *proxy,
    vtkSMRenderModuleProxy *module)
{
  if(!module || !proxy)
    {
    return 0;
    }

  // TODO: How to decide which part of the compound proxy to display ????
  // For now just get the last one, and assuming it is a source proxy.
  vtkSMProxy *proxyToDisplay = 0;
  vtkSMCompoundProxy *bundle = vtkSMCompoundProxy::SafeDownCast(proxy);
  if(bundle)
    {
    int i = bundle->GetNumberOfProxies() - 1;
    for(; proxyToDisplay == 0 && i >= 0; i--)
      {
      proxyToDisplay = vtkSMSourceProxy::SafeDownCast(bundle->GetProxy(i));
      }
    }
  else
    {
    proxyToDisplay = proxy;
    }
  
  // Create the display proxy and register it.
  vtkSMDisplayProxy *display = pqPart::Add(module,
      vtkSMSourceProxy::SafeDownCast(proxyToDisplay));
  if(display)
    {
    QString name;
    name.setNum(this->Names->GetCountAndIncrement("Display"));
    name.prepend("Display");
    vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
    this->InCreateOrConnect = true;
    proxyManager->RegisterProxy("displays", name.toAscii().data(), display);
    this->InCreateOrConnect = false;
    display->Delete();

    // Notify other objects that a display was created.
    emit this->displayCreated(display, name, proxy, module);
    }

  return display;
}

void pqPipelineData::removeAndUnregisterDisplay(vtkSMDisplayProxy *display,
    const char *name, vtkSMRenderModuleProxy *module)
{
  // Get the proxy from the display input.
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  if(display)
    {
    vtkSMProxyProperty *property = vtkSMProxyProperty::SafeDownCast(
        display->GetProperty("Input"));
    if(property && property->GetNumberOfProxies())
      {
      // TODO: How to get the compound proxy from the input proxy.
      vtkSMProxy *proxy = property->GetProxy(0);
      emit this->removingDisplay(display, proxy);

      // Remove the input from the display.
      property->RemoveProxy(proxy);
      }

    // Remove the display proxy from the render module.
    if(module)
      {
      property = vtkSMProxyProperty::SafeDownCast(module->GetProperty(
          "Displays"));
      property->RemoveProxy(display);
      module->UpdateVTKObjects();
      }

    // Clean up the lookup table for the display proxy.
    property = vtkSMProxyProperty::SafeDownCast(display->GetProperty(
        "LookupTable"));
    if(property && property->GetNumberOfProxies())
      {
      vtkSMLookupTableProxy *lookup = vtkSMLookupTableProxy::SafeDownCast(
          property->GetProxy(0));
      proxyManager->UnRegisterProxy("lookup_tables",
          proxyManager->GetProxyName("lookup_tables", lookup));
      }
    }

  if(name)
    {
    // Unregister the display proxy.
    proxyManager->UnRegisterProxy("displays", name);
    }
}

void pqPipelineData::setVisible(vtkSMDisplayProxy *display, bool visible)
{
  if(display)
    {
    display->SetVisibilityCM(visible);
    }
}

// TODO: how to handle compound proxies better ????
void pqPipelineData::addInput(vtkSMProxy *proxy, vtkSMProxy *input)
{
  this->addConnection(input, proxy);
}

void pqPipelineData::removeInput(vtkSMProxy* proxy, vtkSMProxy* input)
{
  this->removeConnection(input, proxy);
}

void pqPipelineData::addConnection(vtkSMProxy *source, vtkSMProxy *sink)
{
  vtkSMCompoundProxy *bundle = vtkSMCompoundProxy::SafeDownCast(source);
  if(bundle)
    {
    // TODO: How to find the correct output proxy?
    source = 0; 
    for(int i = bundle->GetNumberOfProxies(); source == 0 && i > 0; i--)
      {
      source = vtkSMSourceProxy::SafeDownCast(bundle->GetProxy(i-1));
      }
    }

  bundle = vtkSMCompoundProxy::SafeDownCast(sink);
  if(bundle)
    {
    // TODO: How to find the correct input proxy?
    sink = bundle->GetMainProxy();
    }

  if(!source || !sink)
    {
    return;
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
      sink->GetProperty("Input"));
  if(inputProp)
    {
    // If the sink already has an input, the previous connection
    // needs to be broken if it doesn't support multiple inputs.
    if(!inputProp->GetMultipleInput() && inputProp->GetNumberOfProxies() > 0)
      {
      vtkSMProxy *other = inputProp->GetProxy(0);
      emit this->removingConnection(other, sink);
      this->InCreateOrConnect = true;
      inputProp->RemoveProxy(other);
      this->InCreateOrConnect = false;
      }

    // Add the input to the proxy in the server manager.
    inputProp->AddProxy(source);
    this->InCreateOrConnect = true;
    emit this->connectionCreated(source, sink);
    this->InCreateOrConnect = false;
    }
}

void pqPipelineData::removeConnection(vtkSMProxy *source, vtkSMProxy *sink)
{
  if(!source || !sink)
    {
    return;
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
      sink->GetProperty("Input"));
  if(inputProp)
    {
    emit this->removingConnection(source, sink);

    // Remove the input from the server manager.
    this->InCreateOrConnect = true;
    inputProp->RemoveProxy(source);
    this->InCreateOrConnect = false;
    }
}

void pqPipelineData::loadState(vtkPVXMLElement *root, pqMultiView *multiView)
{
  if(!root || !this->Internal)
    {
    return;
    }

  // Find the pipeline element in the xml.
  vtkPVXMLElement *pipeline = pqXMLUtil::FindNestedElementByName(root,
      "Pipeline");
  if(pipeline)
    {
    QString name;
    vtkPVXMLElement *serverManagerState = 0;

    // Create a pqServer for each server element in the xml.
    pqServer *server = 0;
    QStringList address;
    vtkPVXMLElement *serverElement = 0;
    unsigned int total = pipeline->GetNumberOfNestedElements();
    for(unsigned int i = 0; i < total; i++)
      {
      serverElement = pipeline->GetNestedElement(i);
      name = serverElement->GetName();
      if(name == "Server")
        {
        // Get the server's address from the attribute.
        address = QString(serverElement->GetAttribute("address")).split(":");
        if(address.isEmpty())
          {
          continue;
          }

        if(address[0] == "localhost")
          {
          server = pqServer::CreateStandalone();
          }
        else
          {
          if(address.size() < 2)
            {
            continue;
            }

          // Get the port number from the address.
          int port = address[1].toInt();
          server = pqServer::CreateConnection(address[0].toAscii().data(), port);
          }

        if(!server)
          {
          continue;
          }

        // Add the server to the pipeline data.
        this->addServer(server);

        // Loop through the elements in the server element.
        vtkPVXMLElement *element = 0;
        unsigned int count = serverElement->GetNumberOfNestedElements();
        for(unsigned int j = 0; j < count; j++)
          {
          element = serverElement->GetNestedElement(j);
          name = element->GetName();
          if(name == "ServerManagerState")
            {
            // The server manager state may be a sub-element of the
            // server element in demo state files.
            serverManagerState = element;
            }
          else if(multiView)
            {
            QList<int> list;
            if(name == "Window")
              {
              // Restore the window from the server.
              list = pqXMLUtil::GetIntListFromString(
                  element->GetAttribute("windowID"));
              pqMultiViewFrame *frame = qobject_cast<pqMultiViewFrame *>(
                  multiView->widgetOfIndex(list));
              if(frame)
                {
                // Make a new QVTKWidget in the frame.
                // TODO: Move this to a central location?
                ParaQ::AddQVTKWidget(frame, multiView, server);
                }
              }
            else if(name == "Display")
              {
              // Add the display/window pair to the restore map.
              list = pqXMLUtil::GetIntListFromString(
                  element->GetAttribute("windowID"));
              this->Internal->RestoreMap.insert(element->GetAttribute("name"),
                  multiView->widgetOfIndex(list));
              }
            }
          }
        }
      else if(name == "ServerManagerState")
        {
        serverManagerState = serverElement;
        }
      }

    // Restore the server manager state. The server manager should
    // signal the pipeline when new proxies are added.
    if(serverManagerState)
      {
      vtkSMObject::GetProxyManager()->LoadState(serverManagerState);
      }

    // Clean up the restore variables.
    this->Internal->RestoreMap.clear();
    }
}

void pqPipelineData::setCurrentProxy(vtkSMProxy* proxy)
{
  this->CurrentProxy = proxy;
  emit this->currentProxyChanged(this->CurrentProxy);
}

vtkSMProxy * pqPipelineData::currentProxy() const
{
  return this->CurrentProxy;
}

void pqPipelineData::proxyRegistered(vtkObject*, unsigned long, void*,
    void* callData, vtkCommand*)
{
  // Get the proxy information from the call data.
  vtkSMProxyManager::RegisteredProxyInformation *info =
    reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation *>(callData);
  if(!info || !this->Internal || this->InCreateOrConnect)
    {
    return;
    }

  if(strcmp(info->GroupName, "sources") == 0)
    {
    // TODO: Get the server from the proxy's connection ID.
    pqServer *server = this->Model->getServer(0)->GetServer();

    // TODO: Why doesn't this work?
    // Listen for input property change events.
    vtkSMProperty *prop = info->Proxy->GetProperty("Input");
    if(prop)
      {
      this->VTKConnect->Connect(prop, vtkCommand::ModifiedEvent, this,
          SLOT(inputChanged(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
          info->Proxy, 1.0);
      }

    // Use the xml grouping to determine the type of proxy.
    if(strcmp(info->Proxy->GetXMLGroup(), "compound_proxies") == 0)
      {
      emit this->bundleCreated(info->Proxy, info->ProxyName, server);
      }
    else if(strcmp(info->Proxy->GetXMLGroup(), "filters") == 0)
      {
      emit this->filterCreated(info->Proxy, info->ProxyName, server);
      }
    else if(strcmp(info->Proxy->GetXMLGroup(), "sources") == 0)
      {
      emit this->sourceCreated(info->Proxy, info->ProxyName, server);
      }
    }
  else if(strcmp(info->GroupName, "displays") == 0)
    {
    QMap<QString, QWidget *>::Iterator iter =
        this->Internal->RestoreMap.find(info->ProxyName);
    if(iter != this->Internal->RestoreMap.end())
      {
      // The widget in the restore map is the multi-view frame. Use
      // the frame's widget when getting the render module.
      QVTKWidget *widget = qobject_cast<QVTKWidget *>(
          qobject_cast<pqMultiViewFrame *>(*iter)->mainWidget());
      vtkSMRenderModuleProxy *module = this->getRenderModule(widget);
      vtkSMDisplayProxy *display = vtkSMDisplayProxy::SafeDownCast(
          info->Proxy);

      // Get the display proxy's input. If the input doesn't exist
      // yet, put the information on a list to be added to the
      // pipeline object later.
      vtkSMProxyProperty *property = vtkSMProxyProperty::SafeDownCast(
          display->GetProperty("Input"));
      if(property && property->GetNumberOfProxies() > 0)
        {
        emit this->displayCreated(display, info->ProxyName,
            property->GetProxy(0), module);
        }
      else
        {
        // TODO
        }
      }
    }
}

void pqPipelineData::inputChanged(vtkObject* object, unsigned long e,
    void* clientData, void* callData, vtkCommand*)
{
  // Get the property name from the callData and the sink proxy
  // from the clientData.
  const char *name = reinterpret_cast<const char *>(callData);
  vtkSMProxy *sink = reinterpret_cast<vtkSMProxy *>(clientData);
  vtkSMProxyProperty *prop = vtkSMProxyProperty::SafeDownCast(object);

  // TODO: The connection may be needed when the pipeline can
  // be modified outside of ParaQ. For now, it is only needed
  // when restoring a session.
  /*this->VTKConnect->Disconnect(object, e, this);
  if(prop && prop->GetNumberOfProxies() && sink && name &&
      strcmp(name, "Input") == 0)
    {
    // Get the source from the input property.
    vtkSMProxy *source = prop->GetProxy(0);

    // Get the objects from the list of servers.
    pqPipelineObject *sourceItem = 0;
    pqPipelineObject *sinkItem = 0;
    QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
    for( ; iter != this->Internal->Servers.end(); ++iter)
      {
      if(*iter)
        {
        sinkItem = (*iter)->GetObject(sink);
        if(sinkItem)
          {
          // Make sure the source is on the same server.
          sourceItem = (*iter)->GetObject(source);
          break;
          }
        }
      }

    if(sourceItem && sinkItem)
      {
      // Connect the internal objects together.
      sourceItem->AddOutput(sinkItem);
      sinkItem->AddInput(sourceItem);
      emit this->connectionCreated(sourceItem, sinkItem);
      }
    }*/
}


