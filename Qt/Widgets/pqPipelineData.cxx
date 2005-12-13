
#include "pqPipelineData.h"

#include "pqMultiViewFrame.h"
#include "pqNameCount.h"
#include "pqParts.h"
#include "pqPipelineObject.h"
#include "pqPipelineServer.h"
#include "pqServer.h"

#include "QVTKWidget.h"
#include "vtkSMInputProperty.h"
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

  QList<pqPipelineServer *> Servers;
  QMap<QVTKWidget *, vtkSMRenderModuleProxy *> ViewMap;
};


pqPipelineDataInternal::pqPipelineDataInternal()
  : Servers(), ViewMap()
{
}


pqPipelineData *pqPipelineData::Instance = 0;

pqPipelineData::pqPipelineData(QObject* parent)
  : QObject(parent), CurrentProxy(NULL)
{
  this->Internal = new pqPipelineDataInternal;
  this->Names = new pqNameCount();

  if(!pqPipelineData::Instance)
    pqPipelineData::Instance = this;
}

pqPipelineData::~pqPipelineData()
{
  if(pqPipelineData::Instance == this)
    pqPipelineData::Instance = 0;

  if(this->Names)
    delete this->Names;
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
    }
}

pqPipelineData *pqPipelineData::instance()
{
  return pqPipelineData::Instance;
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
  pqPipelineObject *object = serverObject->AddWindow(window);
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
      pqPipelineObject *object = (*iter)->GetWindow(window);
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

vtkSMProxy *pqPipelineData::createSource(const char *proxyName,
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
  vtkSMProxyManager *proxyManager = server->GetServer()->GetProxyManager();
  vtkSMProxy *proxy = proxyManager->NewProxy("sources", proxyName);

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

  this->setCurrentProxy(vtkSMSourceProxy::SafeDownCast(proxy));
  return proxy;
}

vtkSMProxy *pqPipelineData::createFilter(const char *proxyName,
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
  vtkSMProxyManager *proxyManager = server->GetServer()->GetProxyManager();
  vtkSMProxy *proxy = proxyManager->NewProxy("filters", proxyName);

  // Register the proxy with the server manager. Use a unique name
  // based on the class name and a count.
  QString name;
  name.setNum(this->Names->GetCountAndIncrement(proxyName));
  name.prepend(proxyName);
  proxyManager->RegisterProxy("filters", name.toAscii().data(), proxy);
  proxy->Delete();

  // Create an internal representation for the filter.
  pqPipelineObject *filter = server->AddFilter(proxy);
  if(filter)
    {
    filter->SetProxyName(name);
    filter->SetParent(server->GetWindow(window));
    emit this->filterCreated(filter);
    }

  this->setCurrentProxy(vtkSMSourceProxy::SafeDownCast(proxy));
  return proxy;
}

void pqPipelineData::setVisibility(vtkSMProxy *proxy, bool on)
{
  if(!this->Internal || !proxy)
    return;

  // Get the object from the list of servers.
  pqPipelineObject *object = 0;
  QList<pqPipelineServer *>::Iterator iter = this->Internal->Servers.begin();
  for( ; iter != this->Internal->Servers.end(); ++iter)
    {
    if(*iter)
      {
      object = (*iter)->GetObject(proxy);
      if(object)
        break;
      }
    }

  if(!object || !object->GetParent())
    return;

  QVTKWidget *window = qobject_cast<QVTKWidget *>(
      object->GetParent()->GetWidget());
  vtkSMRenderModuleProxy *module = this->getRenderModule(window);
  if(!module)
    return;

  vtkSMDisplayProxy *display = object->GetDisplayProxy();
  if(on)
    {
    if(!display)
      {
      display = pqAddPart(module, vtkSMSourceProxy::SafeDownCast(proxy));
      object->SetDisplayProxy(display);
      }
    }
  else if(display)
    {
    object->SetDisplayProxy(0);
    pqRemovePart(module, display);
    }
}

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

pqPipelineObject *pqPipelineData::getObjectFor(QVTKWidget *window) const
{
  if(!this->Internal || !window)
    return 0;

  // Get the pipeline object for the proxy.
  pqPipelineObject *object = 0;
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

void pqPipelineData::setCurrentProxy(vtkSMSourceProxy* proxy)
{
  this->CurrentProxy = proxy;
  emit this->currentProxyChanged(this->CurrentProxy);
}

vtkSMSourceProxy * pqPipelineData::currentProxy() const
{
  return this->CurrentProxy;
}


