/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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
#include "pqPipelineObject.h"
#include "pqPipelineServer.h"
#include "pqPipelineWindow.h"
#include "pqServer.h"
#include "pqSMMultiView.h"
#include "pqXMLUtil.h"

#include "QVTKWidget.h"
#include "vtkObject.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMCompoundProxy.h"

#include <QList>
#include <QMap>


class pqDisplayRestore
{
public:
  pqDisplayRestore(vtkSMDisplayProxy *display, pqPipelineServer *server,
      QVTKWidget *window);
  ~pqDisplayRestore() {}

  vtkSMDisplayProxy *Display;
  pqPipelineServer *Server;
  QVTKWidget *Window;
};


class pqPipelineDataInternal
{
public:
  pqPipelineDataInternal();
  ~pqPipelineDataInternal() {}

  QList<pqPipelineServer *> Servers;
  QMap<QVTKWidget *, vtkSMRenderModuleProxy *> ViewMap;
  QMap<QString, QWidget *> RestoreMap;
  QList<pqDisplayRestore *> Displays;
  QList<pqPipelineObject *> Connections;
};


pqDisplayRestore::pqDisplayRestore(vtkSMDisplayProxy *display,
    pqPipelineServer *server, QVTKWidget *window)
{
  this->Display = display;
  this->Server = server;
  this->Window = window;
}


pqPipelineDataInternal::pqPipelineDataInternal()
  : Servers(), ViewMap(), RestoreMap(), Displays(), Connections()
{
}


pqPipelineData *pqPipelineData::Instance = 0;

pqPipelineData::pqPipelineData(QObject* p)
  : QObject(p), CurrentProxy(NULL)
{
  this->Internal = new pqPipelineDataInternal;
  this->Names = new pqNameCount();
  this->VTKConnect = vtkEventQtSlotConnect::New();

  if(!pqPipelineData::Instance)
    pqPipelineData::Instance = this;
}

pqPipelineData::~pqPipelineData()
{
  if(pqPipelineData::Instance == this)
    pqPipelineData::Instance = 0;

  if(this->VTKConnect)
    {
    this->VTKConnect->Delete();
    this->VTKConnect = 0;
    }

  if(this->Names)
    {
    delete this->Names;
    this->Names = 0;
    }

  if(this->Internal)
    {
    // Clean up the server objects, which will clean up all the
    // internal pipeline objects.
    QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
    for( ; iter != this->Internal->Servers.end(); ++iter)
      {
      if(*iter)
        {
        delete *iter;
        *iter = 0;
        }
      }

    delete this->Internal;
    this->Internal = 0;
    }
}

pqPipelineData *pqPipelineData::instance()
{
  return pqPipelineData::Instance;
}

void pqPipelineData::saveState(vtkPVXMLElement *root, pqMultiView *multiView)
{
  if(!root || !this->Internal)
    {
    return;
    }

  // Create an element to hold the pipeline state.
  vtkPVXMLElement *pipeline = vtkPVXMLElement::New();
  pipeline->SetName("Pipeline");

  // Save the state for each of the servers.
  QString address;
  pqServer *server = 0;
  vtkPVXMLElement *element = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    element = vtkPVXMLElement::New();
    element->SetName("Server");

    // Save the connection information.
    server = (*iter)->GetServer();
    address = server->getAddress();
    element->AddAttribute("address", address.toAscii().data());
    if(address != server->getFriendlyName())
      {
      element->AddAttribute("name", server->getFriendlyName().toAscii().data());
      }

    (*iter)->SaveState(element, multiView);
    pipeline->AddNestedElement(element);
    element->Delete();
    }

  // Add the pipeline element to the xml.
  root->AddNestedElement(pipeline);
  pipeline->Delete();
}

void pqPipelineData::loadState(vtkPVXMLElement *root, pqMultiView *multiView)
{
  if(!root || !this->Internal)
    {
    return;
    }

  // Find the pipeline element in the xml.
  vtkPVXMLElement *pipeline = ParaQ::FindNestedElementByName(root, "Pipeline");
  if(pipeline)
    {
    // Create a pqServer for each server element in the xml.
    pqServer *server = 0;
    QStringList address;
    QString name;
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

        // Add the server to the pipeline data.
        if(server)
          {
          this->addServer(server);
          vtkPVXMLElement *element = 0;

          // Restore the multi-view windows for the server. Set up the
          // display and object map.
          if(multiView)
            {
            QList<int> list;
            pqMultiViewFrame *frame = 0;
            unsigned int count = serverElement->GetNumberOfNestedElements();
            for(unsigned int j = 0; j < count; j++)
              {
              element = serverElement->GetNestedElement(j);
              name = element->GetName();
              if(name == "Window")
                {
                // Get the multi-view frame to put the window in.
                list = ParaQ::GetIntListFromString(
                    element->GetAttribute("windowID"));
                frame = qobject_cast<pqMultiViewFrame *>(
                    multiView->widgetOfIndex(list));
                if(frame)
                  {
                  // Make a new QVTKWidget in the frame.
                  ParaQ::AddQVTKWidget(frame, multiView, server);
                  }
                }
              else if(name == "Display")
                {
                list = ParaQ::GetIntListFromString(
                    element->GetAttribute("windowID"));
                frame = qobject_cast<pqMultiViewFrame *>(
                    multiView->widgetOfIndex(list));
                if(frame)
                  {
                  QWidget *widget = frame->mainWidget();
                  const char *proxyName = element->GetAttribute("name");
                  if(widget && proxyName)
                    {
                    this->Internal->RestoreMap.insert(QString(proxyName), widget);
                    }

                  proxyName = element->GetAttribute("proxyName");
                  if(widget && proxyName)
                    {
                    this->Internal->RestoreMap.insert(QString(proxyName), widget);
                    }
                  }
                }
              }
            }

          // Restore the server manager state. The server manager
          // should call back with register events.
          element = ParaQ::FindNestedElementByName(serverElement,
              "ServerManagerState");
          if(element)
            {
            vtkSMObject::GetProxyManager()->LoadState(element, 
              server->GetConnectionID());
            }

          // TEMP: Update the pipeline connection information.
          QList<pqPipelineObject *>::Iterator jter =
              this->Internal->Connections.begin();
          for( ; jter != this->Internal->Connections.end(); ++jter)
            {
            pqPipelineObject *source = 0;
            vtkSMProxyProperty *prop = vtkSMProxyProperty::SafeDownCast(
                (*jter)->GetProxy()->GetProperty("Input"));
            if(!prop)
              {
              continue;
              }

            for(unsigned int k = 0; k < prop->GetNumberOfProxies(); k++)
              {
              source = this->getObjectFor(prop->GetProxy(k));
              source->AddOutput(*jter);
              (*jter)->AddInput(source);
              emit this->connectionCreated(source, *jter);
              }
            }

          // Put all the display objects in the correct render module
          // and pipeline object.
          QList<pqDisplayRestore *>::Iterator iter =
              this->Internal->Displays.begin();
          for( ; iter != this->Internal->Displays.end(); ++iter)
            {
            if(*iter)
              {
              vtkSMRenderModuleProxy *module = this->getRenderModule(
                  (*iter)->Window);
              if(module)
                {
                (*iter)->Display->UpdateSelfAndAllInputs();
                vtkSMProxyProperty *prop = vtkSMProxyProperty::SafeDownCast(
                    module->GetProperty("Displays"));
                prop->AddProxy((*iter)->Display);
                module->UpdateVTKObjects();

                // Add the display proxy to the object. Use the display's
                // input to find the object.
                prop = vtkSMProxyProperty::SafeDownCast(
                    (*iter)->Display->GetProperty("Input"));
                pqPipelineObject *object = (*iter)->Server->GetObject(
                    prop->GetProxy(0));
                if(object)
                  {
                  object->SetDisplayProxy((*iter)->Display);
                  }
                }

              delete *iter;
              *iter = 0;
              }
            }

          // Clean up the restore variables.
          this->Internal->RestoreMap.clear();
          this->Internal->Displays.clear();
          }
        }
      }
    }
}

void pqPipelineData::clearPipeline()
{
  if(this->Internal)
    {
    // Clean up the server objects, which will clean up all the internal
    // pipeline objects.
    emit this->clearingPipeline();
    QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
    for( ; iter != this->Internal->Servers.end(); ++iter)
      {
      if(*iter)
        {
        delete *iter;
        *iter = 0;
        }
      }
    }
}

void pqPipelineData::addServer(pqServer *server)
{
  if(!this->Internal || !server)
    return;

  // Make sure the server doesn't already exist.
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter && (*iter)->GetServer() == server)
      return;
    }

  // Create a new internal representation for the server.
  pqPipelineServer *object = new pqPipelineServer();
  if(object)
    {
    object->SetServer(server);
    this->Internal->Servers.append(object);

    // Listen for pipeline events from the server.
    vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
    this->VTKConnect->Connect(proxyManager, vtkCommand::RegisterEvent, this,
        SLOT(proxyRegistered(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
        NULL, 1.0);

    // Signal that a new server was added to the pipeline data.
    emit this->serverAdded(object);
    }
}

void pqPipelineData::removeServer(pqServer *server)
{
  if(!this->Internal || !server)
    return;

  // Find the server in the list.
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter && (*iter)->GetServer() == server)
      {
      emit this->removingServer(*iter);
      delete *iter;
      this->Internal->Servers.erase(iter);
      break;
      }
    }
}

int pqPipelineData::getServerCount() const
{
  if(this->Internal)
    {
    return this->Internal->Servers.size();
    }

  return 0;
}

pqPipelineServer *pqPipelineData::getServer(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Servers.size())
    {
    return this->Internal->Servers[index];
    }

  return 0;
}

pqPipelineServer *pqPipelineData::getServerFor(pqServer *server)
{
  if(this->Internal)
    {
    QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
    for( ; iter != this->Internal->Servers.end(); ++iter)
      {
      if((*iter)->GetServer() == server)
        {
        return *iter;
        }
      }
    }

  return 0;
}

void pqPipelineData::addWindow(QVTKWidget *window, pqServer *server)
{
  if(!this->Internal || !window || !server)
    return;

  // Find the server in the list.
  pqPipelineServer *serverObject = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter && (*iter)->GetServer() == server)
      {
      serverObject = *iter;
      break;
      }
    }

  if(!serverObject)
    return;

  // Create a new window representation.
  pqPipelineWindow *object = serverObject->AddWindow(window);
  if(object)
    {
    // Give the window a title.
    QString name;
    name.setNum(this->Names->GetCountAndIncrement("Window"));
    name.prepend("Window");
    window->setWindowTitle(name);
    pqMultiViewFrame *frame = qobject_cast<pqMultiViewFrame *>(window->parent());
    if(frame)
      {
      frame->setTitle(name);
      }

    object->SetServer(serverObject);
    emit this->windowAdded(object);
    }
}

void pqPipelineData::removeWindow(QVTKWidget *window)
{
  if(!this->Internal || !window)
    return;

  // Find the server in the list.
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter)
      {
      pqPipelineWindow *object = (*iter)->GetWindow(window);
      if(object)
        {
        emit this->removingWindow(object);
        (*iter)->RemoveWindow(window);
        break;
        }
      }
    }
}

void pqPipelineData::addViewMapping(QVTKWidget *widget,
    vtkSMRenderModuleProxy *module)
{
  if(this->Internal && widget && module)
    this->Internal->ViewMap.insert(widget, module);
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
      return *iter;
    }

  return 0;
}

void pqPipelineData::clearViewMapping()
{
  if(this->Internal)
    this->Internal->ViewMap.clear();
}

vtkSMSourceProxy *pqPipelineData::createSource(const char *proxyName,
    QVTKWidget *window)
{
  if(!this->Internal || !proxyName || !window)
    return 0;

  // Get the server from the window.
  pqPipelineServer *server = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter && (*iter)->GetWindow(window))
      {
      server = *iter;
      break;
      }
    }

  if(!server)
    return 0;

  // Create a proxy object on the server.
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  vtkSMSourceProxy *proxy = 
    vtkSMSourceProxy::SafeDownCast(proxyManager->NewProxy("sources", proxyName));
  proxy->SetConnectionID(server->GetServer()->GetConnectionID());

  // Register the proxy with the server manager. Use a unique name
  // based on the class name and a count.
  QString name;
  name.setNum(this->Names->GetCountAndIncrement(proxyName));
  name.prepend(proxyName);
  proxyManager->RegisterProxy("sources", name.toAscii().data(), proxy);
  proxy->Delete();

  // Create an internal representation for the source.
  pqPipelineObject *source = server->AddSource(proxy);
  if(source)
    {
    source->SetProxyName(name);
    source->SetParent(server->GetWindow(window));
    emit this->sourceCreated(source);
    }

  this->setCurrentProxy(proxy);
  return proxy;
}

vtkSMSourceProxy *pqPipelineData::createFilter(const char *proxyName,
    QVTKWidget *window)
{
  if(!this->Internal || !proxyName || !window)
    return 0;

  // Get the server from the window.
  pqPipelineServer *server = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter && (*iter)->GetWindow(window))
      {
      server = *iter;
      break;
      }
    }

  if(!server)
    return 0;

  // Create a proxy object on the server.
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  vtkSMSourceProxy *proxy = 
    vtkSMSourceProxy::SafeDownCast(proxyManager->NewProxy("filters", proxyName));
  proxy->SetConnectionID(server->GetServer()->GetConnectionID());

  // Register the proxy with the server manager. Use a unique name
  // based on the class name and a count.
  QString name;
  name.setNum(this->Names->GetCountAndIncrement(proxyName));
  name.prepend(proxyName);
  proxyManager->RegisterProxy("sources", name.toAscii().data(), proxy);
  proxy->Delete();

  // Create an internal representation for the filter.
  pqPipelineObject *filter = server->AddFilter(proxy);
  if(filter)
    {
    filter->SetProxyName(name);
    filter->SetParent(server->GetWindow(window));
    emit this->filterCreated(filter);
    }

  this->setCurrentProxy(proxy);
  return proxy;
}

vtkSMCompoundProxy *pqPipelineData::createCompoundProxy(const char *proxyName,
    QVTKWidget *window)
{
  if(!this->Internal || !proxyName || !window)
    return 0;

  // Get the server from the window.
  pqPipelineServer *server = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter && (*iter)->GetWindow(window))
      {
      server = *iter;
      break;
      }
    }

  if(!server)
    return 0;

  // Create a proxy object on the server.
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  vtkSMCompoundProxy *proxy = vtkSMCompoundProxy::SafeDownCast(
    proxyManager->NewCompoundProxy(proxyName));
  proxy->SetConnectionID(server->GetServer()->GetConnectionID());
  proxy->UpdateVTKObjects();

  // Register the proxy with the server manager. Use a unique name
  // based on the class name and a count.
  QString name;
  name.setNum(this->Names->GetCountAndIncrement(proxyName));
  name.prepend(proxyName);
  proxyManager->RegisterProxy("compound_proxies", name.toAscii().data(), proxy);
  proxy->Delete();

  // Create an internal representation for the filter.
  pqPipelineObject *filter = server->AddCompoundProxy(proxy);
  if(filter)
    {
    filter->SetProxyName(name);
    filter->SetParent(server->GetWindow(window));
    emit this->filterCreated(filter);
    }

  this->setCurrentProxy(proxy);
  return proxy;
}

void pqPipelineData::deleteProxy(vtkSMProxy *proxy)
{
  if(!this->Internal || !proxy)
    {
    return;
    }

  pqPipelineServer *server = 0;
  pqPipelineObject *object = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter)
      {
      object = (*iter)->GetObject(proxy);
      if(object)
        {
        server = *iter;
        break;
        }
      }
    }

  if(!server || !object)
    {
    return;
    }

  // Disconnect the object's outputs.
  pqPipelineObject *sink = 0;
  vtkSMInputProperty *inputProperty = 0;
  for(int i = 0; i < object->GetOutputCount(); i++)
    {
    sink = object->GetOutput(i);
    inputProperty = vtkSMInputProperty::SafeDownCast(
        sink->GetProxy()->GetProperty("Input"));
    if(inputProperty)
      {
      emit this->removingConnection(object, sink);
      sink->RemoveInput(object);
      inputProperty->RemoveProxy(proxy);
      }
    }

  // Clean up the object.
  emit this->removingObject(object);
  server->RemoveObject(proxy);
}

vtkSMDisplayProxy* pqPipelineData::createDisplay(vtkSMSourceProxy* proxy, vtkSMProxy* rep)
{
  if(!this->Internal || !proxy)
    {
    return NULL;
    }

  if(!rep)
    {
    rep = proxy;
    }
  
  // Get the object from the list of servers.
  pqPipelineServer *server = 0;
  pqPipelineObject *object = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter)
      {
      object = (*iter)->GetObject(rep);
      if(object)
        {
        server = *iter;
        break;
        }
      }
    }

  if(!object || !object->GetParent())
    return NULL;

  QVTKWidget *window = qobject_cast<QVTKWidget *>(
      object->GetParent()->GetWidget());
  vtkSMRenderModuleProxy *module = this->getRenderModule(window);
  if(!module)
    return NULL;

  // Create the display proxy and register it.
  vtkSMDisplayProxy* display = pqPart::Add(module, proxy);
  if(display)
    {
    QString name;
    name.setNum(this->Names->GetCountAndIncrement("Display"));
    name.prepend("Display");
    vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
    proxyManager->RegisterProxy("displays", name.toAscii().data(), display);
    display->Delete();
    }

  // Save the display proxy in the object as well.
  object->SetDisplayProxy(display);

  return display;
}

void pqPipelineData::setVisibility(vtkSMDisplayProxy* display, bool on)
{
  if(!this->Internal || !display)
    {
    return;
    }

  display->SetVisibilityCM(on);
}

// TODO: how to handle compound proxies better ????
void pqPipelineData::addInput(vtkSMProxy *proxy, vtkSMProxy *input)
{
  if(!this->Internal || !proxy || !input)
    return;

  // Get the object from the list of servers.
  pqPipelineObject *source = 0;
  pqPipelineObject *sink = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter)
      {
      sink = (*iter)->GetObject(proxy);
      if(sink)
        {
        // Make sure the source is on the same server.
        source = (*iter)->GetObject(input);
        break;
        }
      }
    }

  if(!source || !sink)
    return;

  // Make sure the two objects are not already connected.
  if(sink->HasInput(source))
    return;

  // TODO  hardwired compound proxies
  vtkSMCompoundProxy* compoundProxy = vtkSMCompoundProxy::SafeDownCast(proxy);
  if(compoundProxy)
    {
    proxy = compoundProxy->GetMainProxy();
    }
  
  // if we are connecting the output of a compound proxy to the input of another filter
  compoundProxy = vtkSMCompoundProxy::SafeDownCast(input);
  if(compoundProxy)
    {
    input = NULL; 
    for(int i=compoundProxy->GetNumberOfProxies(); input == NULL && i>0; i--)
      {
      input = vtkSMSourceProxy::SafeDownCast(compoundProxy->GetProxy(i-1));
      }
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
      proxy->GetProperty("Input"));
  if(inputProp)
    {
    // If the sink already has an input, the previous connection
    // needs to be broken if it doesn't support multiple inputs.
    if(!inputProp->GetMultipleInput() && sink->GetInputCount() > 0)
      {
      pqPipelineObject *other = sink->GetInput(0);
      emit this->removingConnection(other, sink);

      other->RemoveOutput(sink);
      sink->RemoveInput(other);
      inputProp->RemoveProxy(other->GetProxy());
      }

    // Connect the internal objects together.
    source->AddOutput(sink);
    sink->AddInput(source);

    // Add the input to the proxy in the server manager.
    inputProp->AddProxy(input);
    emit this->connectionCreated(source, sink);
    }
}

void pqPipelineData::removeInput(vtkSMProxy* proxy, vtkSMProxy* input)
{
  if(!this->Internal || !proxy || !input)
    return;

  // Get the object from the list of servers.
  pqPipelineObject *source = 0;
  pqPipelineObject *sink = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter)
      {
      sink = (*iter)->GetObject(proxy);
      if(sink)
        {
        // Make sure the source is on the same server.
        source = (*iter)->GetObject(input);
        break;
        }
      }
    }

  if(!source || !sink)
    return;

  // Make sure the two objects are connected.
  if(!sink->HasInput(source))
    return;

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
      proxy->GetProperty("Input"));
  if(inputProp)
    {
    emit this->removingConnection(source, sink);

    // Disconnect the internal objects.
    sink->RemoveInput(source);
    source->RemoveOutput(sink);

    // Remove the input from the server manager.
    inputProp->RemoveProxy(input);
    }
}

QVTKWidget *pqPipelineData::getWindowFor(vtkSMProxy *proxy) const
{
  // Get the pipeline object for the proxy.
  pqPipelineObject *object = getObjectFor(proxy);
  if(object && object->GetParent())
    return qobject_cast<QVTKWidget *>(object->GetParent()->GetWidget());

  return 0;
}

pqPipelineObject *pqPipelineData::getObjectFor(vtkSMProxy *proxy) const
{
  if(!this->Internal || !proxy)
    return 0;

  // Get the pipeline object for the proxy.
  pqPipelineObject *object = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter)
      {
      object = (*iter)->GetObject(proxy);
      if(object)
        return object;
      }
    }

  return 0;
}

pqPipelineWindow *pqPipelineData::getObjectFor(QVTKWidget *window) const
{
  if(!this->Internal || !window)
    return 0;

  // Get the pipeline object for the proxy.
  pqPipelineWindow *object = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter)
      {
      object = (*iter)->GetWindow(window);
      if(object)
        return object;
      }
    }

  return 0;
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
  if(info && this->Internal)
    {
    bool isSource = strcmp(info->GroupName, "sources") == 0;
    bool isDisplay = strcmp(info->GroupName, "displays") == 0;
    if(isSource || isDisplay)
      {
      QString name = info->ProxyName;
      QMap<QString, QWidget *>::Iterator iter =
          this->Internal->RestoreMap.find(name);
      if(iter != this->Internal->RestoreMap.end())
        {
        // Get the server from the window.
        QWidget *window = *iter;
        pqPipelineServer *server = 0;
        QList<pqPipelineServer *>::Iterator jter = this->Internal->Servers.begin();
        for( ; jter != this->Internal->Servers.end(); ++jter)
          {
          if(*jter && (*jter)->GetWindow(window))
            {
            server = *jter;
            break;
            }
          }

        if(server)
          {
          if(isSource)
            {
            info->Proxy->UpdateVTKObjects();

            // Determine the proxy's type. Use the type to create the
            // appropriate internal representation for the proxy.
            pqPipelineObject *object = 0;
            if(vtkSMCompoundProxy::SafeDownCast(info->Proxy))
              {
              object = server->AddCompoundProxy(info->Proxy);
              }
            else if(vtkSMSourceProxy::SafeDownCast(info->Proxy))
              {
              if(strcmp(info->Proxy->GetXMLGroup(), "filters") == 0)
                {
                object = server->AddFilter(info->Proxy);
                }
              else
                {
                object = server->AddSource(info->Proxy);
                }
              }

            if(object)
              {
              object->SetProxyName(name);
              object->SetParent(server->GetWindow(window));
              if(object->GetType() == pqPipelineObject::Source)
                {
                emit this->sourceCreated(object);
                }
              else
                {
                this->Internal->Connections.append(object); // TEMP
                // TODO: Why doesn't this work?
                // Listen for input property change events.
                //vtkSMProperty *prop = info->Proxy->GetProperty("Input");
                //this->VTKConnect->Connect(prop, vtkCommand::ModifiedEvent,
                //    this, SLOT(inputChanged(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
                //    info->Proxy, 1.0);
                emit this->filterCreated(object);
                }
              }
            }
          else if(isDisplay)
            {
            // Add the display proxy to the list for later.
            this->Internal->Displays.append(new pqDisplayRestore(
                vtkSMDisplayProxy::SafeDownCast(info->Proxy), server,
                qobject_cast<QVTKWidget *>(window)));
            }
          }
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
  this->VTKConnect->Disconnect(object, e, this);
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
    }
}


