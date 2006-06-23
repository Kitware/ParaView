/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerObserver.cxx

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

#include "pqServerManagerObserver.h"

#include "pqMultiView.h"
#include "pqMultiViewFrame.h"
#include "pqParts.h"
#include "pqPipelineModel.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqXMLUtil.h"

#include "QVTKWidget.h"
#include "vtkObject.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
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
#include <QtDebug>

//-----------------------------------------------------------------------------
class pqServerManagerObserverInternal
{
public:
  pqServerManagerObserverInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
  /// Used to listen to proxy manager events.
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqServerManagerObserver::pqServerManagerObserver(QObject* p) : QObject(p)
{
  this->Internal = new pqServerManagerObserverInternal();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
  
  /// Listen to interesting events from the proxy manager and process module.
  this->Internal->VTKConnect->Connect(proxyManager, vtkCommand::RegisterEvent, this,
    SLOT(proxyRegistered(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
    NULL, 1.0);

  this->Internal->VTKConnect->Connect(proxyManager, vtkCommand::UnRegisterEvent, this,
    SLOT(proxyUnRegistered(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
    NULL, 1.0);

  this->Internal->VTKConnect->Connect(pm, vtkCommand::ConnectionCreatedEvent,
    this, SLOT(connectionCreated(vtkObject*, unsigned long, void*, void*)));
  this->Internal->VTKConnect->Connect(pm, vtkCommand::ConnectionClosedEvent,
    this, SLOT(connectionClosed(vtkObject*, unsigned long, void*, void*)));
}

//-----------------------------------------------------------------------------
pqServerManagerObserver::~pqServerManagerObserver()
{
  delete this->Internal;
}


/*
//-----------------------------------------------------------------------------
void pqServerManagerObserver::addViewMapping(QVTKWidget *widget,
    vtkSMRenderModuleProxy *module)
{
  if(this->Internal && widget && module)
    {
    this->Internal->ViewMap.insert(widget, module);
    this->Internal->RenderMap.insert(module, widget);
    }
}

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy *pqServerManagerObserver::removeViewMapping(QVTKWidget *widget)
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

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy *pqServerManagerObserver::getRenderModule(
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

//-----------------------------------------------------------------------------
QVTKWidget *pqServerManagerObserver::getRenderWindow(
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

//-----------------------------------------------------------------------------
void pqServerManagerObserver::clearViewMapping()
{
  if(this->Internal)
    {
    this->Internal->ViewMap.clear();
    this->Internal->RenderMap.clear();
    }
}
*/
//-----------------------------------------------------------------------------
/*
vtkSMProxy *pqServerManagerObserver::createAndRegisterBundle(const char *proxyName,
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

  return proxy;
}
*/

/*
void pqServerManagerObserver::removeAndUnregisterDisplay(vtkSMDisplayProxy *display,
    const char *name, vtkSMRenderModuleProxy *module)
{
  // Get the proxy from the display input.
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  if(display)
    {
    vtkSMProxyProperty *prop = vtkSMProxyProperty::SafeDownCast(
        display->GetProperty("Input"));
    if(prop && prop->GetNumberOfProxies())
      {
      // TODO: How to get the compound proxy from the input proxy.
      vtkSMProxy *proxy = prop->GetProxy(0);
      emit this->removingDisplay(display, proxy);

      // Remove the input from the display.
      prop->RemoveProxy(proxy);
      }

    // Remove the display proxy from the render module.
    if(module)
      {
      prop = vtkSMProxyProperty::SafeDownCast(module->GetProperty(
          "Displays"));
      prop->RemoveProxy(display);
      module->UpdateVTKObjects();
      }

    // Clean up the lookup table for the display proxy.
    prop = vtkSMProxyProperty::SafeDownCast(display->GetProperty(
        "LookupTable"));
    if(prop && prop->GetNumberOfProxies())
      {
      vtkSMLookupTableProxy *lookup = vtkSMLookupTableProxy::SafeDownCast(
          prop->GetProxy(0));
      proxyManager->UnRegisterProxy("lookup_tables",
          proxyManager->GetProxyName("lookup_tables", lookup));
      }
    }

  if(name)
    {
    // Unregister the display proxy.
    proxyManager->UnRegisterProxy("displays", name);
    }
}*/
/*
//-----------------------------------------------------------------------------
// TODO: how to handle compound proxies better ????
void pqServerManagerObserver::addInput(vtkSMProxy *proxy, vtkSMProxy *input)
{
  this->addConnection(input, proxy);
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::removeInput(vtkSMProxy* proxy, vtkSMProxy* input)
{
  this->removeConnection(input, proxy);
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::addConnection(vtkSMProxy *source, vtkSMProxy *sink)
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
      // emit this->removingConnection(other, sink);
      this->InCreateOrConnect = true;
      inputProp->RemoveProxy(other);
      this->InCreateOrConnect = false;
      }

    // Add the input to the proxy in the server manager.
    inputProp->AddProxy(source);
    this->InCreateOrConnect = true;
    // emit this->connectionCreated(source, sink);
    this->InCreateOrConnect = false;
    }
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::removeConnection(vtkSMProxy *source, vtkSMProxy *sink)
{
  if(!source || !sink)
    {
    return;
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
      sink->GetProperty("Input"));
  if(inputProp)
    {
    // emit this->removingConnection(source, sink);

    // Remove the input from the server manager.
    this->InCreateOrConnect = true;
    inputProp->RemoveProxy(source);
    this->InCreateOrConnect = false;
    }
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::loadState(vtkPVXMLElement *root, pqMultiView *multiView)
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

        // Loop through the elements in the server state.
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
                ParaView::AddQVTKWidget(frame, multiView, server);
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

        // Restore the server manager state. The server manager should
        // signal the pipeline when new proxies are added.
        if(serverManagerState)
          {
          vtkSMObject::GetProxyManager()->LoadState(serverManagerState,
              server->GetConnectionID());
          }
        }
      }

    // Clean up the restore variables.
    this->Internal->RestoreMap.clear();
    }
}
*/
//-----------------------------------------------------------------------------
void pqServerManagerObserver::proxyRegistered(vtkObject*, unsigned long, void*,
    void* callData, vtkCommand*)
{
  // Get the proxy information from the call data.
  vtkSMProxyManager::RegisteredProxyInformation *info =
    reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation *>(callData);
  if(!info || !this->Internal)
    {
    return;
    }

  if(info->IsCompoundProxyDefinition)
    {
    emit this->compoundProxyDefinitionRegistered(info->ProxyName);
    }
  else if(strcmp(info->GroupName, "sources") == 0)
    {
    emit this->sourceRegistered(info->ProxyName, info->Proxy);
    }
  else if(strcmp(info->GroupName, "displays") == 0)
    {
    emit this->displayRegistered(info->ProxyName, info->Proxy);
    }
  else if (strcmp(info->GroupName, "render_modules")==0)
    {
    // A render module is registered. proxies in this group
    // are vtkSMRenderModuleProxies which are "alive" with a window and all,
    // and not the vtkSMMultiViewRenderModuleProxy.
    vtkSMRenderModuleProxy* rm = vtkSMRenderModuleProxy::SafeDownCast(
      info->Proxy);
    emit this->renderModuleRegistered(info->ProxyName, rm);
    }
  else
    {
    emit this->proxyRegistered(info->GroupName, info->ProxyName,
      info->Proxy);
    }
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::proxyUnRegistered(vtkObject*, unsigned long, void*,
    void* callData, vtkCommand*)
{
  // Get the proxy information from the call data.
  vtkSMProxyManager::RegisteredProxyInformation *info =
    reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation *>(callData);

  if(!info || !this->Internal)
    {
    return;
    }

  if(info->IsCompoundProxyDefinition)
    {
    emit this->compoundProxyDefinitionUnRegistered(info->ProxyName);
    }
  else if(strcmp(info->GroupName, "sources") == 0 )
    {
    emit this->sourceUnRegistered(info->Proxy);
    }
  else if (strcmp(info->GroupName, "displays") == 0)
    {
    emit this->displayUnRegistered(info->Proxy);
    }
  else if (strcmp(info->GroupName, "render_modules") == 0)
    {
    emit this->renderModuleUnRegistered(
      vtkSMRenderModuleProxy::SafeDownCast(info->Proxy));
    }
  else
    {
    emit this->proxyUnRegistered(info->GroupName, info->ProxyName,
      info->Proxy);
    }
}
//-----------------------------------------------------------------------------
void pqServerManagerObserver::connectionCreated(vtkObject*, unsigned long, void*, 
  void* callData)
{
  emit this->connectionCreated(*reinterpret_cast<vtkIdType*>(callData));
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::connectionClosed(vtkObject*, unsigned long, void*, 
  void* callData)
{
  emit this->connectionClosed(*reinterpret_cast<vtkIdType*>(callData));
}

