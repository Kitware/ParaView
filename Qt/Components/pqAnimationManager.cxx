/*=========================================================================

   Program: ParaView
   Module:    pqAnimationManager.cxx

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
#include "pqAnimationManager.h"
#include "ui_pqAbortAnimation.h"
#include "ui_pqAnimationSettings.h"

#include "vtkProcessModule.h"
#include "vtkSMAnimationSceneGeometryWriter.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMProxyManager.h"
#include "vtkSMPVAnimationSceneProxy.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMServerProxyManagerReviver.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QMap>
#include <QMessageBox>
#include <QPointer>
#include <QSize>
#include <QtDebug>

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqProxy.h"
#include "pqRenderViewModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

static inline int pqCeil(double val)
{
  if (val == static_cast<int>(val))
    {
    return static_cast<int>(val);
    }
  return static_cast<int>(val+1.0);
}
//-----------------------------------------------------------------------------
class pqAnimationManager::pqInternals
{
public:
  QPointer<pqServer> ActiveServer;
  QPointer<QWidget> ViewWidget;
  typedef QMap<pqServer*, QPointer<pqAnimationScene> > SceneMap;
  SceneMap Scenes;
  Ui::Dialog* AnimationSettingsDialog;

  QSize OldMaxSize;
  QSize OldSize;
};

//-----------------------------------------------------------------------------
pqAnimationManager::pqAnimationManager(QObject* _parent/*=0*/) 
:QObject(_parent)
{
  this->Internals = new pqAnimationManager::pqInternals();
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(proxyAdded(pqProxy*)),
    this, SLOT(onProxyAdded(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)),
    this, SLOT(onProxyRemoved(pqProxy*)));

  QObject::connect(smmodel, SIGNAL(renderModuleAdded(pqRenderViewModule*)),
    this, SLOT(updateViewModules()));
  QObject::connect(smmodel, SIGNAL(renderModuleRemoved(pqRenderViewModule*)),
    this, SLOT(updateViewModules()));
}

//-----------------------------------------------------------------------------
pqAnimationManager::~pqAnimationManager()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::setViewWidget(QWidget* w)
{
  this->Internals->ViewWidget = w;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::updateViewModules()
{
  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return;
    }
  QList<pqGenericViewModule*> viewModules = 
    pqApplicationCore::instance()->getServerManagerModel()->getViewModules(
      this->Internals->ActiveServer);
  
  QList<pqSMProxy> viewList;
  foreach(pqGenericViewModule* view, viewModules)
    {
    viewList.push_back(pqSMProxy(view->getProxy()));
    } 

  vtkSMAnimationSceneProxy* sceneProxy = scene->getAnimationSceneProxy();
  pqSMAdaptor::setProxyListProperty(sceneProxy->GetProperty("ViewModules"),
    viewList);
  sceneProxy->UpdateProperty("ViewModules");
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
      emit this->activeSceneChanged(this->getActiveScene());
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
      emit this->activeSceneChanged(this->getActiveScene());
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onActiveServerChanged(pqServer* server)
{
  this->Internals->ActiveServer = server;
  if (server && !this->getActiveScene())
    {
    this->createActiveScene();
    }
  emit this->activeSceneChanged(this->getActiveScene());
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
pqAnimationScene* pqAnimationManager::createActiveScene() 
{
  if (this->Internals->ActiveServer)
    {
    pqPipelineBuilder* builder = pqApplicationCore::instance()->getPipelineBuilder();
    vtkSMProxy *scene = builder->createProxy("animation", "PVAnimationScene", "animation",
      this->Internals->ActiveServer, false);
    scene->SetServers(vtkProcessModule::CLIENT);
    // this will result in a call to onProxyAdded() and proper
    // signals will be emitted.
    if (!scene)
      {
      qDebug() << "Failed to create scene proxy.";
      }
   
    this->updateViewModules();
    return this->getActiveScene();
    }
  return 0;
}


//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationManager::getCue(
  pqAnimationScene* scene, vtkSMProxy* proxy, const char* propertyname, 
  int index) const
{
  return (scene? scene->getCue(proxy, propertyname, index) : 0);
}


//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationManager::createCue(pqAnimationScene* scene, 
    vtkSMProxy* proxy, const char* propertyname, int index) 
{
  if (!scene)
    {
    qDebug() << "Cannot create cue without scene.";
    return 0;
    }

  return scene->createCue(proxy, propertyname, index);
}

//-----------------------------------------------------------------------------
void pqAnimationManager::durationChanged()
{
  this->frameRateChanged();
}

//-----------------------------------------------------------------------------
void pqAnimationManager::frameRateChanged()
{
  double framerate =
    this->Internals->AnimationSettingsDialog->frameRate->value();
  double num_frames = 
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->value();

  double duration =  
    this->Internals->AnimationSettingsDialog->animationDuration->value();

  switch (this->getActiveScene()->getAnimationSceneProxy()->GetPlayMode())
    {
  case vtkSMPVAnimationSceneProxy::SEQUENCE:
  case vtkSMPVAnimationSceneProxy::SNAP_TO_TIMESTEPS:
    this->Internals->AnimationSettingsDialog->animationDuration->
      blockSignals(true);
    this->Internals->AnimationSettingsDialog->animationDuration->setValue(
      num_frames/framerate);
    this->Internals->AnimationSettingsDialog->animationDuration->
      blockSignals(false);
    break;

  case vtkSMPVAnimationSceneProxy::REALTIME:
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(true);
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->setValue(
      static_cast<int>(duration*framerate));
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(false);
    break;
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::numberOfFramesChanged()
{
  this->frameRateChanged();
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::saveAnimation(const QString& filename)
{
  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return false;
    }
  vtkSMPVAnimationSceneProxy* sceneProxy = 
    vtkSMPVAnimationSceneProxy::SafeDownCast(scene->getAnimationSceneProxy());

  QFileInfo fileinfo(filename);
  QString filePrefix = filename;
  int dot_pos;
  if ((dot_pos = filename.lastIndexOf(".")) != -1)
    {
    filePrefix = filename.left(dot_pos);
    }
  QString extension = fileinfo.suffix();

  QDialog dialog;
  Ui::Dialog dialogUI;
  this->Internals->AnimationSettingsDialog = &dialogUI;
  dialogUI.setupUi(&dialog);

  // TODO: Until we fix IceT rendermodule to work without client
  // one cannot disconnect if there is more than 1 view.
  dialogUI.checkBoxDisconnect->setEnabled(
    this->Internals->ActiveServer->isRemote() 
    && (sceneProxy->GetNumberOfViewModules()==1));
  bool isMPEG = (extension == "mpg");
  if (isMPEG)
    {
    // Size bounds for mpeg.
    dialogUI.spinBoxWidth->setMaximum(1920);
    dialogUI.spinBoxHeight->setMaximum(1080);

    // Frame rate is fixed when using vtkMPIWriter.
    dialogUI.frameRate->setValue(30);
    dialogUI.frameRate->setEnabled(false);
    }
  else
    {
    // Size bounds for mpeg.
    dialogUI.spinBoxWidth->setMaximum(8000);
    dialogUI.spinBoxHeight->setMaximum(8000);
    }

  // Set current size of the window.
  QSize viewSize = scene->getViewSize();
  dialogUI.spinBoxHeight->setValue(viewSize.height());
  dialogUI.spinBoxWidth->setValue(viewSize.width());

  // Set current duration/frame rate/no. of. frames.
  double frame_rate = dialogUI.frameRate->value();
  switch (sceneProxy->GetPlayMode())
    {
  case vtkSMPVAnimationSceneProxy::SEQUENCE:
    dialogUI.spinBoxNumberOfFrames->setValue(
      sceneProxy->GetNumberOfFrames());
    dialogUI.animationDuration->setEnabled(false);
    dialogUI.animationDuration->setValue(
      sceneProxy->GetNumberOfFrames()/frame_rate);
    break;

  case vtkSMPVAnimationSceneProxy::REALTIME:
    dialogUI.animationDuration->setValue(sceneProxy->GetDuration());
    dialogUI.spinBoxNumberOfFrames->setValue(
      static_cast<int>(sceneProxy->GetDuration()*frame_rate));
    dialogUI.spinBoxNumberOfFrames->setEnabled(false);
    break;

  case vtkSMPVAnimationSceneProxy::SNAP_TO_TIMESTEPS:
    dialogUI.spinBoxNumberOfFrames->setValue(
      sceneProxy->GetNumberOfTimeSteps());
    dialogUI.animationDuration->setValue(
      sceneProxy->GetNumberOfTimeSteps()*frame_rate);
    dialogUI.spinBoxNumberOfFrames->setEnabled(false);
    dialogUI.animationDuration->setEnabled(false);
    break;
    }

  QObject::connect(
    dialogUI.animationDuration, SIGNAL(valueChanged(double)),
    this, SLOT(durationChanged()));
  QObject::connect(
    dialogUI.frameRate, SIGNAL(valueChanged(double)),
    this, SLOT(frameRateChanged()));
  QObject::connect(
    dialogUI.spinBoxNumberOfFrames, SIGNAL(valueChanged(int)),
    this, SLOT(numberOfFramesChanged()));

  if (!dialog.exec())
    {
    this->Internals->AnimationSettingsDialog = 0;
    return false;
    }
  this->Internals->AnimationSettingsDialog = 0;

  // Update Scene properties based on user options. 
  double num_frames = sceneProxy->GetNumberOfFrames();
  double duration = sceneProxy->GetDuration();
  switch (sceneProxy->GetPlayMode())
    {
  case vtkSMPVAnimationSceneProxy::SEQUENCE:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"), 
      dialogUI.spinBoxNumberOfFrames->value());
    break;

  case vtkSMPVAnimationSceneProxy::REALTIME:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("Duration"),
      dialogUI.animationDuration->value());
    break;
    }
  sceneProxy->UpdateVTKObjects();

  QSize newSize(dialogUI.spinBoxWidth->value(),
    dialogUI.spinBoxHeight->value());

  // Enfore the multiple of 4 criteria.
  int magnification = this->updateViewSizes(newSize, viewSize, isMPEG);
 
  if (dialogUI.checkBoxDisconnect->checkState() == Qt::Checked)
    {
    pqServer* server = this->Internals->ActiveServer;
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

    vtkSMProxy* writer = pxm->NewProxy("writers", "AnimationSceneImageWriter");
    writer->SetConnectionID(server->GetConnectionID());
    pxm->RegisterProxy("animation", "writer", writer);
    writer->Delete();

    pqSMAdaptor::setElementProperty(writer->GetProperty("FileName"),
      filename.toAscii().data());
    pqSMAdaptor::setElementProperty(writer->GetProperty("Magnification"), 
      magnification); 
    pqSMAdaptor::setElementProperty(writer->GetProperty("FrameRate"),
      dialogUI.frameRate->value());
    writer->UpdateVTKObjects();

    // We save the animation offline.
    vtkSMProxy* cleaner = 
      pxm->NewProxy("connection_cleaners", "AnimationPlayer");
    cleaner->SetConnectionID(server->GetConnectionID());
    pxm->RegisterProxy("animation","cleaner",cleaner);
    cleaner->Delete();

    pqSMAdaptor::setProxyProperty(cleaner->GetProperty("Writer"), writer);
    cleaner->UpdateVTKObjects();

    vtkSMServerProxyManagerReviver* reviver = 
      vtkSMServerProxyManagerReviver::New();
    int status = reviver->ReviveRemoteServerManager(server->GetConnectionID());
    reviver->Delete();
    pqApplicationCore::instance()->removeServer(server);
    return status;
    }

  vtkSMAnimationSceneImageWriter* writer = vtkSMAnimationSceneImageWriter::New();
  writer->SetFileName(filename.toAscii().data());
  writer->SetMagnification(magnification);
  writer->SetAnimationScene(sceneProxy);
  writer->SetFrameRate(dialogUI.frameRate->value());
  bool status = writer->Save();
  writer->Delete();

  // Restore, duration and number of frames.
  switch (sceneProxy->GetPlayMode())
    {
  case vtkSMPVAnimationSceneProxy::SEQUENCE:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"), 
      num_frames);
    break;

  case vtkSMPVAnimationSceneProxy::REALTIME:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("Duration"),
      duration);
    break;
    }
  sceneProxy->UpdateVTKObjects();

  this->restoreViewSizes();
  return status;
}

//-----------------------------------------------------------------------------
int pqAnimationManager::updateViewSizes(QSize newSize, QSize currentSize, bool isMPEG)
{
  QSize requested_newSize = newSize;
  // Enforce requested size restrictions based on the choosen
  // format.
  if (isMPEG)
    {
    int &width = newSize.rwidth();
    int &height = newSize.rheight();
    if ((width % 32) > 0)
      {
      width -= width % 32;
      }
    if ((height % 8) > 0)
      {
      height -= height % 8;
      }
    if (width > 1920)
      {
      width = 1920;
      }
    if (height > 1080)
      {
      height = 1080;
      }
    }
  else
    {
    int &width = newSize.rwidth();
    int &height = newSize.rheight();
    if ((width % 4) > 0)
      {
      width -= width % 4;
      }
    if ((height % 4) > 0)
      {
      height -= height % 4;
      }
    }

  if (requested_newSize != newSize)
    {
    QMessageBox::warning(NULL, "Resolution Changed",
      QString("The requested resolution has been changed from (%1, %2)\n").arg(
        requested_newSize.width()).arg(requested_newSize.height()) + 
      QString("to (%1, %2) to match format specifications.").arg(
        newSize.width()).arg(newSize.height()));
    }

  int magnification = 1;

  // If newSize > currentSize, then magnification is involved.
  int temp = pqCeil(newSize.width()/static_cast<double>(currentSize.width()));
  magnification = (temp> magnification)? temp: magnification;

  temp = pqCeil(newSize.height()/static_cast<double>(currentSize.height()));
  magnification = (temp > magnification)? temp : magnification;

  newSize = newSize/magnification;

  if (!this->Internals->ViewWidget)
    {
    qDebug() << "ViewWidget must be set to the parent of all views.";
    }
  else
    {
    this->Internals->OldSize = this->Internals->ViewWidget->size();
    this->Internals->OldMaxSize = this->Internals->ViewWidget->maximumSize();
    this->Internals->ViewWidget->setMaximumSize(newSize);
    this->Internals->ViewWidget->resize(newSize);
    QCoreApplication::processEvents();
    }

  return magnification;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::restoreViewSizes()
{
  if (this->Internals->ViewWidget)
    {
    this->Internals->ViewWidget->setMaximumSize(this->Internals->OldMaxSize);
    this->Internals->ViewWidget->resize(this->Internals->OldSize);
    }
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::saveGeometry(const QString& filename, 
  pqGenericViewModule* view)
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
  vtkSMAnimationSceneProxy* sceneProxy = scene->getAnimationSceneProxy();

  vtkSMAnimationSceneGeometryWriter* writer = vtkSMAnimationSceneGeometryWriter::New();
  writer->SetFileName(filename.toAscii().data());
  writer->SetAnimationScene(sceneProxy);
  writer->SetViewModule(view->getProxy());
  bool status = writer->Save();
  writer->Delete();
  return status;
}
