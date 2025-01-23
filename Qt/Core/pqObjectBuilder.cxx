// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqObjectBuilder.h"

#include "vtkDebugLeaksManager.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputProperty.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSmartPointer.h"

#include <QApplication>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QtDebug>

#include "pqAnimationCue.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqInterfaceTracker.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqProxyModifiedStateUndoElement.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMProxyManager.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <cassert>
#include <chrono>

namespace
{
bool ContinueWaiting = true;
bool processEvents()
{
  QApplication::processEvents();
  return ContinueWaiting;
}

//-----------------------------------------------------------------------------
QString recoverRegistrationName(vtkSMProxy* proxy)
{
  vtkSMProperty* nameProperty = proxy->GetProperty("RegistrationName");
  if (nameProperty)
  {
    proxy->UpdatePropertyInformation(nameProperty);
    std::string newName = vtkSMPropertyHelper(nameProperty).GetAsString();
    newName = vtkSMCoreUtilities::SanitizeName(newName);
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMSessionProxyManager* spxm = pxm->GetActiveSessionProxyManager();
    newName = spxm->GetUniqueProxyName(proxy->GetXMLGroup(), newName.c_str(), false);
    return QString::fromStdString(newName);
  }
  return QString();
}

//-----------------------------------------------------------------------------
bool preCreatePipelineProxy(vtkSMParaViewPipelineController* controller, vtkSMProxy* proxy)
{
  return controller->PreInitializeProxy(proxy);
}

//-----------------------------------------------------------------------------
pqPipelineSource* postCreatePipelineProxy(vtkSMParaViewPipelineController* controller,
  vtkSMProxy* proxy, pqServer* server, const QString& regName = QString())
{
  // since there are no properties to set, nothing to do here.
  controller->PostInitializeProxy(proxy);
  if (regName.isEmpty())
  {
    controller->RegisterPipelineProxy(proxy);
  }
  else
  {
    controller->RegisterPipelineProxy(proxy, regName.toUtf8().data());
  }

  pqPipelineSource* source =
    pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(proxy);
  source->setModifiedState(pqProxy::UNINITIALIZED);

  // Manage Modified state in Undo/Redo only if not a collaborative server
  if (!server->session()->IsMultiClients())
  {
    pqProxyModifiedStateUndoElement* elem = pqProxyModifiedStateUndoElement::New();
    elem->SetSession(server->session());
    elem->MadeUninitialized(source);
    ADD_UNDO_ELEM(elem);
    elem->Delete();
  }

  // Update animation scene.
  vtkSMAnimationSceneProxy::UpdateAnimationUsingDataTimeSteps(
    controller->GetAnimationScene(server->session()));

  return source;
}
}

//-----------------------------------------------------------------------------
pqObjectBuilder::pqObjectBuilder(QObject* _parent /*=0*/)
  : QObject(_parent)
  , ForceWaitingForConnection(false)
  , WaitingForConnection(false)
{
}

//-----------------------------------------------------------------------------
pqObjectBuilder::~pqObjectBuilder() = default;

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createSource(
  const QString& sm_group, const QString& sm_name, pqServer* server)
{
  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy(sm_group.toUtf8().data(), sm_name.toUtf8().data()));
  if (!::preCreatePipelineProxy(controller, proxy))
  {
    return nullptr;
  }

  pqPipelineSource* source =
    ::postCreatePipelineProxy(controller, proxy, server, ::recoverRegistrationName(proxy));
  Q_EMIT this->sourceCreated(source);
  Q_EMIT this->proxyCreated(source);
  return source;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createFilter(const QString& sm_group, const QString& sm_name,
  QMap<QString, QList<pqOutputPort*>> namedInputs, pqServer* server)
{
  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy(sm_group.toUtf8().data(), sm_name.toUtf8().data()));
  if (!::preCreatePipelineProxy(controller, proxy))
  {
    return nullptr;
  }

  // Now for every input port, connect the inputs.
  QMap<QString, QList<pqOutputPort*>>::iterator mapIter;
  for (mapIter = namedInputs.begin(); mapIter != namedInputs.end(); ++mapIter)
  {
    const QString& input_port_name = mapIter.key();
    QList<pqOutputPort*>& inputs = mapIter.value();

    vtkSMProperty* prop = proxy->GetProperty(input_port_name.toUtf8().data());
    if (!prop)
    {
      qCritical() << "Failed to locate input property " << input_port_name;
      continue;
    }

    vtkSMPropertyHelper helper(prop);
    Q_FOREACH (pqOutputPort* opPort, inputs)
    {
      helper.Add(opPort->getSource()->getProxy(), opPort->getPortNumber());
    }
  }

  pqPipelineSource* filter =
    ::postCreatePipelineProxy(controller, proxy, server, ::recoverRegistrationName(proxy));
  Q_EMIT this->filterCreated(filter);
  Q_EMIT this->proxyCreated(filter);
  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createFilter(
  const QString& group, const QString& name, pqPipelineSource* input, int output_port)
{
  QMap<QString, QList<pqOutputPort*>> namedInputs;
  QList<pqOutputPort*> inputs;
  inputs.push_back(input->getOutputPort(output_port));
  namedInputs["Input"] = inputs;

  return this->createFilter(group, name, namedInputs, input->getServer());
}

//-----------------------------------------------------------------------------
namespace
{
inline QString pqObjectBuilderGetPath(const QString& filename, bool use_dir)
{
  if (use_dir)
  {
    return QFileInfo(filename).path();
  }
  return filename;
}
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createReader(
  const QString& sm_group, const QString& sm_name, const QStringList& files, pqServer* server)
{
  if (files.empty())
  {
    return nullptr;
  }

  std::vector<std::string> filesStd(files.size());
  std::transform(files.constBegin(), files.constEnd(), filesStd.begin(),
    [](const QString& str) -> std::string { return str.toStdString(); });
  QString fileRegName = QString(vtkSMCoreUtilities::FindLargestPrefix(filesStd).c_str());

  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy(sm_group.toUtf8().data(), sm_name.toUtf8().data()));
  if (!::preCreatePipelineProxy(controller, proxy))
  {
    return nullptr;
  }

  QString pname = this->getFileNamePropertyName(proxy);
  if (!pname.isEmpty())
  {
    vtkSMStringVectorProperty* prop =
      vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty(pname.toUtf8().data()));
    if (!prop)
    {
      return nullptr;
    }

    // If there's a hint on the property indicating that this property expects a
    // directory name, then, we will set the directory name on it.
    bool use_dir = false;
    if (prop->GetHints() && prop->GetHints()->FindNestedElementByName("UseDirectoryName"))
    {
      use_dir = true;
    }

    if (files.size() == 1 || !prop->GetRepeatCommand())
    {
      pqSMAdaptor::setElementProperty(prop, ::pqObjectBuilderGetPath(files[0], use_dir));
    }
    else
    {
      QList<QVariant> values;
      Q_FOREACH (QString file, files)
      {
        values.push_back(::pqObjectBuilderGetPath(file, use_dir));
      }
      pqSMAdaptor::setMultipleElementProperty(prop, values);
    }
    proxy->UpdateVTKObjects();
  }

  QString regName = ::recoverRegistrationName(proxy);
  if (regName.isEmpty())
  {
    regName = fileRegName;
  }

  pqPipelineSource* reader = ::postCreatePipelineProxy(controller, proxy, server, regName);
  Q_EMIT this->readerCreated(reader, files[0]);
  Q_EMIT this->readerCreated(reader, files);
  Q_EMIT this->sourceCreated(reader);
  Q_EMIT this->proxyCreated(reader);
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

  if (!source->getAllConsumers().empty())
  {
    qDebug() << "Cannot remove source with consumers.";
    return;
  }

  Q_EMIT this->destroying(source);

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->UnRegisterProxy(source->getProxy());
}

//-----------------------------------------------------------------------------
pqView* pqObjectBuilder::createView(const QString& type, pqServer* server)
{
  if (!server)
  {
    qDebug() << "Cannot create view without server.";
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("views", type.toUtf8().data()));
  if (!proxy)
  {
    qDebug() << "Failed to create a proxy for the requested view type:" << type;
    return nullptr;
  }

  // notify the world that we may create a new view. applications may handle
  // this by setting up layouts, etc.
  Q_EMIT this->aboutToCreateView(server);

  QString regName = ::recoverRegistrationName(proxy);

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(proxy);
  controller->PostInitializeProxy(proxy);
  controller->RegisterViewProxy(proxy, regName.toUtf8().data());

  pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
  pqView* view = model->findItem<pqView*>(proxy);
  if (view)
  {
    Q_EMIT this->viewCreated(view);
    Q_EMIT this->proxyCreated(view);
  }
  else
  {
    qDebug() << "Cannot locate the pqView for the view proxy of type" << type;
  }
  return view;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqView* view)
{
  if (!view)
  {
    return;
  }

  Q_EMIT this->destroying(view);
  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->UnRegisterProxy(view->getProxy());
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::addToLayout(pqView* view, pqProxy* layout)
{
  if (view)
  {
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    controller->AssignViewToLayout(view->getViewProxy(),
      vtkSMViewLayoutProxy::SafeDownCast(layout ? layout->getProxy() : nullptr));
  }
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqObjectBuilder::createDataRepresentation(
  pqOutputPort* opPort, pqView* view, const QString& representationType)
{
  if (!opPort || !view)
  {
    qCritical() << "Missing required attribute.";
    return nullptr;
  }

  if (!view->canDisplay(opPort))
  {
    // View cannot display this source, nothing to do here.
    return nullptr;
  }

  vtkSmartPointer<vtkSMProxy> reprProxy;

  pqPipelineSource* source = opPort->getSource();
  vtkSMSessionProxyManager* pxm = source->proxyManager();

  // HACK to create correct representation for text sources/filters.
  QString srcProxyName = source->getProxy()->GetXMLName();
  if (representationType != "")
  {
    reprProxy.TakeReference(pxm->NewProxy("representations", representationType.toUtf8().data()));
  }
  else
  {
    reprProxy.TakeReference(view->getViewProxy()->CreateDefaultRepresentation(
      source->getProxy(), opPort->getPortNumber()));
  }

  // Could not determine representation proxy to create.
  if (!reprProxy)
  {
    return nullptr;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(reprProxy);

  QString regName = ::recoverRegistrationName(reprProxy);

  // Set the reprProxy's input.
  pqSMAdaptor::setInputProperty(
    reprProxy->GetProperty("Input"), source->getProxy(), opPort->getPortNumber());
  controller->PostInitializeProxy(reprProxy);
  controller->RegisterRepresentationProxy(reprProxy, regName.toUtf8().data());

  // Add the reprProxy to render module.
  vtkSMProxy* viewModuleProxy = view->getProxy();
  vtkSMPropertyHelper(viewModuleProxy, "Representations").Add(reprProxy);
  viewModuleProxy->UpdateVTKObjects();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqDataRepresentation* repr =
    core->getServerManagerModel()->findItem<pqDataRepresentation*>(reprProxy);
  if (repr)
  {
    Q_EMIT this->dataRepresentationCreated(repr);
    Q_EMIT this->proxyCreated(repr);
  }
  return repr;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqObjectBuilder::createProxy(
  const QString& sm_group, const QString& sm_name, pqServer* server, const QString& reg_group)
{
  if (!server)
  {
    qDebug() << "server cannot be null";
    return nullptr;
  }
  if (sm_group.isEmpty() || sm_name.isEmpty())
  {
    qCritical() << "Group name and proxy name must be non empty.";
    return nullptr;
  }

  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy(sm_group.toUtf8().data(), sm_name.toUtf8().data()));
  if (!proxy.GetPointer())
  {
    qCritical() << "Failed to create proxy: " << sm_group << ", " << sm_name;
    return nullptr;
  }
  else if (reg_group.contains("prototypes"))
  {
    // Mark as prototype to prevent them from being saved in undo stack and
    // managed through the state
    proxy->SetPrototype(true);
  }

  pxm->RegisterProxy(
    reg_group.toUtf8().data(), ::recoverRegistrationName(proxy).toUtf8().data(), proxy);
  return proxy;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqRepresentation* repr)
{
  if (!repr)
  {
    return;
  }

  Q_EMIT this->destroying(repr);

  // Remove repr from the view module.
  pqView* view = repr->getView();
  if (view)
  {
    vtkSMProxyProperty* pp =
      vtkSMProxyProperty::SafeDownCast(view->getProxy()->GetProperty("Representations"));
    pp->RemoveProxy(repr->getProxy());
    view->getProxy()->UpdateVTKObjects();
  }

  // If this repr has a lookuptable, we hide all scalar bars for that
  // lookup table unless there is some other repr that's using it.
  pqScalarsToColors* stc = nullptr;
  if (pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr))
  {
    stc = dataRepr->getLookupTable();
  }

  this->destroyProxyInternal(repr);

  if (stc)
  {
    // this hides scalar bars only if the LUT is not used by
    // any other repr. This must happen after the repr has
    // been deleted.
    stc->hideUnusedScalarBars();
  }
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqProxy* proxy)
{
  Q_EMIT this->destroying(proxy);

  this->destroyProxyInternal(proxy);
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroySources(pqServer* server)
{
  pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  QList<pqPipelineSource*> sources = model->findItems<pqPipelineSource*>(server);
  while (!sources.isEmpty())
  {
    for (int i = 0; i < sources.size(); i++)
    {
      if (sources[i]->getAllConsumers().empty())
      {
        builder->destroy(sources[i]);
        sources[i] = nullptr;
      }
    }
    sources.removeAll(nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyLookupTables(pqServer* server)
{
  pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  QList<pqScalarsToColors*> luts = model->findItems<pqScalarsToColors*>(server);
  Q_FOREACH (pqScalarsToColors* lut, luts)
  {
    builder->destroy(lut);
  }

  QList<pqScalarBarRepresentation*> scalarbars =
    model->findItems<pqScalarBarRepresentation*>(server);
  Q_FOREACH (pqScalarBarRepresentation* sb, scalarbars)
  {
    builder->destroy(sb);
  }
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyPipelineProxies(pqServer* server)
{
  this->destroySources(server);
  this->destroyLookupTables(server);
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyAllProxies(pqServer* server)
{
  if (!server)
  {
    qDebug() << "Server cannot be NULL.";
    return;
  }

  server->proxyManager()->UnRegisterProxies();
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyProxyInternal(pqProxy* proxy)
{
  if (proxy)
  {
    vtkSMSessionProxyManager* pxm = proxy->proxyManager();
    pxm->UnRegisterProxy(
      proxy->getSMGroup().toUtf8().data(), proxy->getSMName().toUtf8().data(), proxy->getProxy());
  }
}

//-----------------------------------------------------------------------------
QString pqObjectBuilder::getFileNamePropertyName(vtkSMProxy* proxy)
{
  const char* fname = vtkSMCoreUtilities::GetFileNameProperty(proxy);
  return fname ? QString(fname) : QString();
}

//-----------------------------------------------------------------------------
bool pqObjectBuilder::forceWaitingForConnection(bool force)
{
  std::swap(force, this->ForceWaitingForConnection);
  return force;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::abortPendingConnections()
{
  ::ContinueWaiting = false;
}

//-----------------------------------------------------------------------------
pqServer* pqObjectBuilder::createServer(const pqServerResource& resource, int connectionTimeout,
  vtkNetworkAccessManager::ConnectionResult& result)
{
  if (this->WaitingForConnection)
  {
    qCritical() << "createServer called while waiting for previous connection "
                   "to be established.";
    return nullptr;
  }

  SCOPED_UNDO_EXCLUDE();

  ::ContinueWaiting = true;

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();

  if (!vtkProcessModule::GetProcessModule()->GetMultipleSessionsSupport())
  {
    // If multiple connections are not supported, then we only connect to the
    // new server if no already connected and ensure that any previously
    // connected servers are disconnected.
    // determine if we're already connected to this server.
    pqServer* server = nullptr;

    // If a server name is set, use it to find the server, if not, use the schemehostsports instead
    if (resource.serverName().isEmpty())
    {
      // Create a modified version of the resource that only contains server information
      const pqServerResource server_resource = resource.schemeHostsPorts();
      server = smModel->findServer(server_resource);
    }
    else
    {
      server = smModel->findServer(resource.serverName());
    }

    if (server)
    {
      return server;
    }

    if (smModel->getNumberOfItems<pqServer*>() > 0)
    {
      this->removeServer(smModel->getItemAtIndex<pqServer*>(0));
    }
  }

  this->WaitingForConnection = true;

  // Let the pqServerManagerModel know the resource to use for the connection
  // to be created.
  smModel->setActiveResource(resource);

  // Based on the server resource, create the correct type of server ...
  vtkIdType id = 0;
  if (resource.scheme() == "builtin")
  {
    id = vtkSMSession::ConnectToSelf();
    result = vtkNetworkAccessManager::ConnectionResult::CONNECTION_SUCCESS;
  }
  else if (resource.scheme() == "cs")
  {
    id = vtkSMSession::ConnectToRemote(resource.host().toUtf8().data(), resource.port(11111),
      connectionTimeout, &::processEvents, result);
  }
  else if (resource.scheme() == "csrc")
  {
    id = vtkSMSession::ReverseConnectToRemote(
      resource.port(11111), connectionTimeout, &::processEvents, result);
  }
  else if (resource.scheme() == "cdsrs")
  {
    id = vtkSMSession::ConnectToRemote(resource.dataServerHost().toUtf8().data(),
      resource.dataServerPort(11111), resource.renderServerHost().toUtf8().data(),
      resource.renderServerPort(22221), connectionTimeout, &::processEvents, result);
  }
  else if (resource.scheme() == "cdsrsrc")
  {
    id = vtkSMSession::ReverseConnectToRemote(resource.dataServerPort(11111),
      resource.renderServerPort(22221), connectionTimeout, &::processEvents, result);
  }
  else if (resource.scheme() == "catalyst")
  {
    id = vtkSMSession::ConnectToCatalyst();
    result = vtkNetworkAccessManager::ConnectionResult::CONNECTION_SUCCESS;
  }
  else
  {
    qCritical() << "Unknown server type: " << resource.scheme() << "\n";
    result = vtkNetworkAccessManager::ConnectionResult::CONNECTION_FAILURE;
  }

  pqServer* server = nullptr;
  if (id != 0)
  {
    server = smModel->findServer(id);
    Q_EMIT this->finishedAddingServer(server);
  }
  this->WaitingForConnection = false;
  return server;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::removeServer(pqServer* server)
{
  SCOPED_UNDO_EXCLUDE();
  if (!server)
  {
    qDebug() << "No server to remove.";
    return;
  }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* sModel = core->getServerManagerModel();
  sModel->beginRemoveServer(server);
  vtkSMSession::Disconnect(server->sessionId());
  sModel->endRemoveServer();
}

//-----------------------------------------------------------------------------
pqServer* pqObjectBuilder::resetServer(pqServer* server)
{
  assert(server);

  // save the current remaining lifetime to restore it
  // when we recreate the server following reset.
  const int remainingLifetime = server->getRemainingLifeTime();
  auto startTime = std::chrono::system_clock::now();

  pqServerResource resource = server->getResource();
  vtkSmartPointer<vtkSMSession> session = server->session();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // simulate disconnect.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();
  smModel->beginRemoveServer(server);

  // Unregister all proxies.
  this->destroyAllProxies(server);

  // Unregister session.
  pm->UnRegisterSession(session);
  smModel->endRemoveServer();

  server = nullptr;

  // Let the pqServerManagerModel know the resource to use for the connection
  // to be created.
  smModel->setActiveResource(resource);

  // re-register session.
  vtkIdType id = pm->RegisterSession(session);

  pqServer* newServer = nullptr;
  if (id != 0)
  {
    newServer = smModel->findServer(id);

    auto endTime = std::chrono::system_clock::now();
    auto deltaMinutes = std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime);
    newServer->setRemainingLifeTime(
      remainingLifetime > 0 ? (remainingLifetime - deltaMinutes.count()) : remainingLifetime);
    Q_EMIT this->finishedAddingServer(newServer);
  }
  return newServer;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqAnimationCue* cue)
{
  if (!cue)
  {
    return;
  }

  vtkSMSessionProxyManager* pxm = cue->proxyManager();

  QList<vtkSMProxy*> keyframes = cue->getKeyFrames();
  // unregister all the keyframes.
  Q_FOREACH (vtkSMProxy* kf, keyframes)
  {
    pxm->UnRegisterProxy("animation", pxm->GetProxyName("animation", kf), kf);
  }
  this->destroy(static_cast<pqProxy*>(cue));
}
