/*=========================================================================

   Program: ParaView
   Module:    pqSimpleAnimationManager.cxx

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

#include "pqSimpleAnimationManager.h"

// ParaView Server Manager includes.
#include "QVTKWidget.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMServerProxyManagerReviver.h"
#include "vtkSMTimestepKeyFrameProxy.h"

// Qt includes.
#include <QDialog>
#include <QFileInfo>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqRenderModule.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "ui_pqAbortAnimation.h"
#include "ui_pqAnimationSettings.h"

//-----------------------------------------------------------------------------
class pqSimpleAnimationManagerProxies
{
public:
  vtkSmartPointer<vtkSMProxy> AnimationScene;
  vtkSmartPointer<vtkSMProxy> AnimationCue;
  vtkSmartPointer<vtkSMProxy> KeyframeManipulator;
  vtkSmartPointer<vtkSMProxy> KeyFrame0;
  vtkSmartPointer<vtkSMProxy> KeyFrame1;

  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqSimpleAnimationManagerProxies()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }

  ~pqSimpleAnimationManagerProxies()
    {
    this->unregisterProxies();
    }

  void createSceneProxies(pqServer* server, bool useDataSetTimesteps)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

    // Create animation scene.
    this->AnimationScene.TakeReference(
      pxm->NewProxy("animation", "AnimationScene"));
    this->AnimationScene->SetConnectionID(server->GetConnectionID());
    
    pqSMAdaptor::setElementProperty(
      this->AnimationScene->GetProperty("Loop"), 0);

    // Create a cue for the timesteps property.
    this->AnimationCue.TakeReference(
      pxm->NewProxy("animation", "AnimationCue"));
    this->AnimationCue->SetConnectionID(server->GetConnectionID());
    
    pqSMAdaptor::setProxyProperty(this->AnimationScene->GetProperty(
        "Cues"), this->AnimationCue);
    pqSMAdaptor::setEnumerationProperty(this->AnimationCue->GetProperty(
        "TimeMode"), "Normalized");
    pqSMAdaptor::setElementProperty(this->AnimationCue->GetProperty(
        "EndTime"), 1.0);

    // Create a key frame manupulator for the cue.
    this->KeyframeManipulator.TakeReference(
      pxm->NewProxy("animation_manipulators", "KeyFrameAnimationCueManipulator"));
    this->KeyframeManipulator->SetConnectionID(server->GetConnectionID());

    pqSMAdaptor::setProxyProperty(this->AnimationCue->GetProperty("Manipulator"), 
      this->KeyframeManipulator);

    vtkSMProxyProperty* keyFramesProperty = vtkSMProxyProperty::SafeDownCast(
      this->KeyframeManipulator->GetProperty("KeyFrames"));
    keyFramesProperty->RemoveAllProxies();

    // Create key frame proxies.
    const char* keyframeType = (useDataSetTimesteps)?
      "TimestepKeyFrame" : "RampKeyFrame";
    this->KeyFrame0.TakeReference(
      pxm->NewProxy("animation_keyframes", keyframeType));
    this->KeyFrame0->SetConnectionID(server->GetConnectionID());
    pqSMAdaptor::setElementProperty(this->KeyFrame0->GetProperty("KeyTime"),   0.0);
    pqSMAdaptor::setElementProperty(this->KeyFrame0->GetProperty("NumberOfKeyValues"), 1);
    keyFramesProperty->AddProxy(this->KeyFrame0);

    this->KeyFrame1.TakeReference(
      pxm->NewProxy("animation_keyframes", keyframeType));
    this->KeyFrame1->SetConnectionID(server->GetConnectionID());
    pqSMAdaptor::setElementProperty(this->KeyFrame1->GetProperty("KeyTime"), 1.0);
    pqSMAdaptor::setElementProperty(this->KeyFrame1->GetProperty("NumberOfKeyValues"), 1);
    keyFramesProperty->AddProxy(this->KeyFrame1);

    this->KeyFrame0->UpdateVTKObjects();
    this->KeyFrame1->UpdateVTKObjects();
    this->KeyframeManipulator->UpdateVTKObjects();
    this->AnimationCue->UpdateVTKObjects();
    this->AnimationScene->UpdateVTKObjects();

    pxm->RegisterProxy("animation","Scene", this->AnimationScene);
    pxm->RegisterProxy("animation", "Cue", this->AnimationCue);
    pxm->RegisterProxy("animation", "Manipulator", this->KeyframeManipulator);
    pxm->RegisterProxy("animation", "key0", this->KeyFrame0);
    pxm->RegisterProxy("animation", "key1", this->KeyFrame1);
    }

  void unregisterProxies()
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

    pxm->UnRegisterProxy("animation", "Scene", this->AnimationScene);
    pxm->UnRegisterProxy("animation", "Cue", this->AnimationCue);
    pxm->UnRegisterProxy("animation", "Manipulator", this->KeyframeManipulator);
    pxm->UnRegisterProxy("animation", "key0", this->KeyFrame0);
    pxm->UnRegisterProxy("animation", "key1", this->KeyFrame1);

    this->AnimationScene = 0;
    this->AnimationCue = 0;
    this->KeyframeManipulator = 0;
    this->KeyFrame0 = 0;
    this->KeyFrame1 = 0;
    }
};

//-----------------------------------------------------------------------------
pqSimpleAnimationManager::pqSimpleAnimationManager( QObject *_p/*=0*/)
  :QObject(_p)
{
  this->Source = 0;
  this->Server = 0;
  this->RenderModule = 0;
  this->Proxies = 0;

}

//-----------------------------------------------------------------------------
pqSimpleAnimationManager::~pqSimpleAnimationManager()
{
  this->Source = 0;
  delete this->Proxies;
}

//-----------------------------------------------------------------------------
void pqSimpleAnimationManager::onAnimationTick()
{
  QApplication::processEvents();
}

//-----------------------------------------------------------------------------
bool pqSimpleAnimationManager::canAnimate(pqPipelineSource* source)
{
  if (!source)
    {
    return false;
    }
  vtkSMProxy* proxy = source->getProxy();
  if (!proxy)
    {
    return false;
    }
  vtkSMProperty* values = proxy->GetProperty("TimestepValues");
  vtkSMProperty* timestep = proxy->GetProperty("TimeStep");
  return (values && timestep && timestep->GetInformationProperty() == values);
}

//-----------------------------------------------------------------------------
void pqSimpleAnimationManager::abortSavingAnimation()
{
  if (this->Proxies)
    {
    vtkSMAnimationSceneProxy* scene = 
      vtkSMAnimationSceneProxy::SafeDownCast(this->Proxies->AnimationScene);
    if (scene)
      {
      scene->Stop();
      }
    }
}

//-----------------------------------------------------------------------------
bool pqSimpleAnimationManager::createTimestepAnimation(
  pqPipelineSource* source, const QString& filename)
{
  if (!this->canAnimate(source))
    {
    return false;
    }

  if (!this->Server || !this->RenderModule)
    {
    qDebug() << "Server and RenderModule must be set on pqSimpleAnimationManager";
    return false;
    }

  vtkSMProxy* proxy = source->getProxy();
  vtkSMProperty* timestep = proxy->GetProperty("TimeStep");
  vtkSMDoubleVectorProperty* timestepValues = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      timestep->GetInformationProperty());
  proxy->UpdatePropertyInformation(timestepValues);
  pqRenderModule* activeView = this->RenderModule;

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
  dialogUI.setupUi(&dialog);
  dialogUI.checkBoxDisconnect->setEnabled(this->Server->isRemote());

  if (extension == "mpg")
    {
    // Size bounds for mpeg.
    dialogUI.spinBoxWidth->setMaximum(1920);
    dialogUI.spinBoxHeight->setMaximum(1080);

    // Frame rate is fixed when using vtkMPIWriter.
    dialogUI.spinBoxFrameRate->setValue(30);
    dialogUI.spinBoxFrameRate->setEnabled(false);
    }
  else
    {
    // Size bounds for mpeg.
    dialogUI.spinBoxWidth->setMaximum(8000);
    dialogUI.spinBoxHeight->setMaximum(8000);
    }

  // Set current size of the window.
  QSize viewSize = activeView->getWidget()->size();
  dialogUI.spinBoxHeight->setValue(viewSize.height());
  dialogUI.spinBoxWidth->setValue(viewSize.width());

  // Set default duration.
  dialogUI.spinBoxAnimationDuration->setValue(
    timestepValues->GetNumberOfElements());

  if (!dialog.exec())
    {
    return false;
    }

  bool useDataSetTimesteps = 
    dialogUI.checkBoxUseDatasetTimesteps->checkState() == Qt::Checked;

  this->Proxies = new pqSimpleAnimationManagerProxies;
  this->Proxies->createSceneProxies(this->Server, useDataSetTimesteps);

  vtkSMProxy* cue = this->Proxies->AnimationCue;
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  pqSMAdaptor::setProxyProperty(cue->GetProperty("AnimatedProxy"), proxy);
  pqSMAdaptor::setElementProperty(cue->GetProperty("AnimatedPropertyName"), 
    "TimeStep");
  pqSMAdaptor::setElementProperty(cue->GetProperty("AnimatedElement"), 0);
  cue->UpdateVTKObjects();

  pqSMAdaptor::setElementProperty(
    this->Proxies->KeyFrame0->GetProperty("KeyValues"), 0.0);
  this->Proxies->KeyFrame0->UpdateVTKObjects();

  pqSMAdaptor::setElementProperty(
    this->Proxies->KeyFrame1->GetProperty("KeyValues"),
    timestepValues->GetNumberOfElements()-1);
  this->Proxies->KeyFrame1->UpdateVTKObjects();

  pqSMAdaptor::setProxyProperty(
    this->Proxies->AnimationScene->GetProperty("RenderModule"), 
    activeView->getProxy());

  activeView->getWidget()->resize(dialogUI.spinBoxWidth->value(),
    dialogUI.spinBoxHeight->value());

  pqSMAdaptor::setElementProperty(
    this->Proxies->AnimationScene->GetProperty("StartTime"),    0);
  pqSMAdaptor::setElementProperty(
    this->Proxies->AnimationScene->GetProperty("EndTime"),
    dialogUI.spinBoxAnimationDuration->value());
  this->Proxies->AnimationScene->UpdateVTKObjects();
 
  if (dialogUI.checkBoxDisconnect->checkState() == Qt::Checked)
    {
    // We save the animation offline.
    vtkSMProxy* cleaner = 
      pxm->NewProxy("connection_cleaners", "AnimationPlayer");
    cleaner->SetConnectionID(this->Server->GetConnectionID());
    pxm->RegisterProxy("animation","cleaner",cleaner);
    cleaner->Delete();

    pqSMAdaptor::setElementProperty(cleaner->GetProperty("AnimationFileName"),
      filename.toStdString().c_str());
    pqSMAdaptor::setMultipleElementProperty(cleaner->GetProperty("Size"), 0,
      dialogUI.spinBoxWidth->value());
    pqSMAdaptor::setMultipleElementProperty(cleaner->GetProperty("Size"), 1,
      dialogUI.spinBoxHeight->value());
    pqSMAdaptor::setElementProperty(cleaner->GetProperty("FrameRate"),
      dialogUI.spinBoxFrameRate->value());
    cleaner->UpdateVTKObjects();

    vtkSMServerProxyManagerReviver* reviver = 
      vtkSMServerProxyManagerReviver::New();
    int status = 
      reviver->ReviveRemoteServerManager(this->Server->GetConnectionID());
    reviver->Delete();

    // we must remove all the proxies we created before the server
    // disconnects.
    delete this->Proxies;
    this->Proxies = 0;
    pqApplicationCore::instance()->removeServer(this->Server);
    return status;
    }

  QDialog abortDialog;
  Ui::AbortAnimation abortDialogUI;
  abortDialogUI.setupUi(&abortDialog);
  abortDialog.show();
  QApplication::processEvents();

  QObject::connect(&abortDialog, SIGNAL(accepted()), 
    this, SLOT(abortSavingAnimation()));


  vtkSMAnimationSceneProxy* scene = vtkSMAnimationSceneProxy::SafeDownCast(
    this->Proxies->AnimationScene);
  int status = 0;
  if (scene)
    {
    status = scene->SaveImages(
      filePrefix.toStdString().c_str(),
      extension.toStdString().c_str(), 
      dialogUI.spinBoxWidth->value(),
      dialogUI.spinBoxHeight->value(),
      dialogUI.spinBoxFrameRate->value(), 0);
    }
 
  activeView->getWidget()->resize(viewSize);   
  delete this->Proxies;
  this->Proxies = 0;
  return (status == 0);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

