/*=========================================================================

   Program: ParaView
   Module:    pqAnimationManager.cxx

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

========================================================================*/
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

#define SEQUENCE 0
#define REALTIME 1
#define SNAP_TO_TIMESTEPS 2

//-----------------------------------------------------------------------------
class pqAnimationManager::pqInternals
{
public:
  pqInternals()
    : AnimationPlaying(false)
  {
  }

  QPointer<pqServer> ActiveServer;
  typedef QMap<pqServer*, QPointer<pqAnimationScene> > SceneMap;
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

  QObject::connect(this, SIGNAL(beginPlay()), this, SLOT(onBeginPlay()));
  QObject::connect(this, SIGNAL(endPlay()), this, SLOT(onEndPlay()));
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
    QObject::disconnect(activeScene, SIGNAL(beginPlay()), this, SIGNAL(beginPlay()));
    QObject::disconnect(activeScene, SIGNAL(endPlay()), this, SIGNAL(endPlay()));
  }

  this->Internals->ActiveServer = server;
  activeScene = this->getActiveScene();
  Q_EMIT this->activeServerChanged(server);
  Q_EMIT this->activeSceneChanged(activeScene);

  if (activeScene)
  {
    QObject::connect(activeScene, SIGNAL(beginPlay()), this, SIGNAL(beginPlay()));
    QObject::connect(activeScene, SIGNAL(endPlay()), this, SIGNAL(endPlay()));
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
  return 0;
}

//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationManager::getCue(
  pqAnimationScene* scene, vtkSMProxy* proxy, const char* propertyname, int index) const
{
  return (scene ? scene->getCue(proxy, propertyname, index) : 0);
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
    .arg(filename.toLocal8Bit().data())
    .arg("view", view->getProxy())
    .arg("comment", "save animation geometry from a view");

  vtkSMProxy* sceneProxy = scene->getProxy();
  vtkSMAnimationSceneGeometryWriter* writer = vtkSMAnimationSceneGeometryWriter::New();
  writer->SetFileName(filename.toLocal8Bit().data());
  writer->SetAnimationScene(sceneProxy);
  writer->SetViewModule(view->getProxy());
  bool status = writer->Save();
  writer->Delete();
  return status;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onTick(int progress)
{
  Q_EMIT this->saveProgress("Saving Animation", progress);
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onBeginPlay()
{
  this->Internals->AnimationPlaying = true;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onEndPlay()
{
  this->Internals->AnimationPlaying = false;
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::animationPlaying() const
{
  return this->Internals->AnimationPlaying;
}
