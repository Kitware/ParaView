// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnimationManager.h"
#include "ui_pqAbortAnimation.h"

#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMAnimationSceneGeometryWriter.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QFileInfo>
#include <QIntValidator>
#include <QMap>
#include <QMessageBox>
#include <QPointer>
#include <QSize>
#include <QtDebug>

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEventDispatcher.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqProgressManager.h"
#include "pqProxy.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include <sstream>

//-----------------------------------------------------------------------------
class pqAnimationManager::pqInternals
{
public:
  pqInternals()
    : AnimationPlaying(false)
  {
  }

  QPointer<pqServer> ActiveServer;
  typedef QMap<pqServer*, QPointer<pqAnimationScene>> SceneMap;
  SceneMap Scenes;

  QSize OldMaxSize;
  QSize OldSize;

  double AspectRatio;
  bool AnimationPlaying;
  int OldNumberOfFrames;
};

//-----------------------------------------------------------------------------
pqAnimationManager::pqAnimationManager(QObject* _parent /*=0*/)
  : QObject(_parent)
{
  this->Internals = new pqAnimationManager::pqInternals();
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(proxyAdded(pqProxy*)), this, SLOT(onProxyAdded(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)), this, SLOT(onProxyRemoved(pqProxy*)));

  QObject::connect(this,
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::beginPlay), this,
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::onBeginPlay));
  QObject::connect(this,
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::endPlay), this,
    QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::onEndPlay));
}

//-----------------------------------------------------------------------------
pqAnimationManager::~pqAnimationManager()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onProxyAdded(pqProxy* proxy)
{
  pqAnimationScene* scene = qobject_cast<pqAnimationScene*>(proxy);
  if (scene && !this->Internals->Scenes.contains(scene->getServer()))
  {
    this->Internals->Scenes[scene->getServer()] = scene;
    if (this->Internals->ActiveServer == scene->getServer())
    {
      Q_EMIT this->activeSceneChanged(this->getActiveScene());
    }
  }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onProxyRemoved(pqProxy* proxy)
{
  pqAnimationScene* scene = qobject_cast<pqAnimationScene*>(proxy);
  if (scene)
  {
    this->Internals->Scenes.remove(scene->getServer());
    if (this->Internals->ActiveServer == scene->getServer())
    {
      Q_EMIT this->activeSceneChanged(this->getActiveScene());
    }
  }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onActiveServerChanged(pqServer* server)
{
  // In case of multi-server that method can be called when we disconnect
  // from one or our connected server.
  // Check if the server is going to be deleted and if so just skip creation
  if (!server || !server->session() || server->session()->GetReferenceCount() == 1)
  {
    return;
  }

  pqAnimationScene* activeScene = this->getActiveScene();
  if (activeScene)
  {
    QObject::disconnect(activeScene,
      QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationScene::beginPlay), this,
      QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::beginPlay));
    QObject::disconnect(activeScene,
      QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationScene::endPlay), this,
      QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::endPlay));
  }

  this->Internals->ActiveServer = server;
  activeScene = this->getActiveScene();
  Q_EMIT this->activeServerChanged(server);
  Q_EMIT this->activeSceneChanged(activeScene);

  if (activeScene)
  {
    QObject::connect(activeScene,
      QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationScene::beginPlay), this,
      QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::beginPlay));
    QObject::connect(activeScene,
      QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationScene::endPlay), this,
      QOverload<vtkObject*, unsigned long, void*, void*>::of(&pqAnimationManager::endPlay));
  }
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::getActiveScene() const
{
  return this->getScene(this->Internals->ActiveServer);
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::getScene(pqServer* server) const
{
  if (server && this->Internals->Scenes.contains(server))
  {
    return this->Internals->Scenes.value(server);
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationManager::getCue(
  pqAnimationScene* scene, vtkSMProxy* proxy, const char* propertyname, int index) const
{
  return (scene ? scene->getCue(proxy, propertyname, index) : nullptr);
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::saveGeometry(const QString& filename, pqView* view)
{
  if (!view)
  {
    return false;
  }

  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
  {
    return false;
  }

  SM_SCOPED_TRACE(CallFunction)
    .arg("WriteAnimationGeometry")
    .arg(filename.toUtf8().data())
    .arg("view", view->getProxy())
    .arg("comment", qPrintable(tr("save animation geometry from a view")));

  vtkSMProxy* sceneProxy = scene->getProxy();
  vtkSMAnimationSceneGeometryWriter* writer = vtkSMAnimationSceneGeometryWriter::New();
  writer->SetFileName(filename.toUtf8().data());
  writer->SetAnimationScene(sceneProxy);
  writer->SetViewModule(view->getProxy());
  bool status = writer->Save();
  writer->Delete();
  return status;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onTick(int progress)
{
  Q_EMIT this->saveProgress(qPrintable(tr("Saving Animation")), progress);
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onBeginPlay(
  vtkObject* /*caller*/, unsigned long /*event_id*/, void*, void* /*reversed*/)
{
  this->Internals->AnimationPlaying = true;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onEndPlay(
  vtkObject* /*caller*/, unsigned long /*event_id*/, void*, void* /*reversed*/)
{
  this->Internals->AnimationPlaying = false;
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::animationPlaying() const
{
  return this->Internals->AnimationPlaying;
}
