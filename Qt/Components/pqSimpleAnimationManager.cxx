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
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMKeyFrameAnimationCueManipulatorProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMTimestepKeyFrameProxy.h"
#include "QVTKWidget.h"
#include "vtkEventQtSlotConnect.h"

// Qt includes.
#include <QDialog>
#include <QFileInfo>
#include <QtDebug>

// ParaView includes.
#include "pqPipelineSource.h"
#include "pqPipelineDisplay.h"
#include "pqSMAdaptor.h"
#include "pqApplicationCore.h"
#include "pqRenderModule.h"
#include "ui_pqAbortAnimation.h"
#include "ui_pqAnimationSettings.h"

//-----------------------------------------------------------------------------
class pqSimpleAnimationManagerInternal
{
public:
  vtkSmartPointer<vtkSMAnimationSceneProxy> AnimationScene;
  vtkSmartPointer<vtkSMAnimationCueProxy> AnimationCue;
  vtkSmartPointer<vtkSMKeyFrameAnimationCueManipulatorProxy> 
    KeyframeManipulator;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqSimpleAnimationManagerInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqSimpleAnimationManager::pqSimpleAnimationManager(QObject *_p/*=0*/)
  :QObject(_p)
{
  this->Source = 0;
  this->Internal = new pqSimpleAnimationManagerInternal;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  // Create animation scene.
  this->Internal->AnimationScene = vtkSMAnimationSceneProxy::SafeDownCast(
    pxm->NewProxy("animation", "AnimationScene"));
  this->Internal->AnimationScene->Delete();

  this->Internal->VTKConnect->Connect(this->Internal->AnimationScene,
    vtkCommand::AnimationCueTickEvent, this,
    SLOT(onAnimationTick()));

  pqSMAdaptor::setElementProperty(this->Internal->AnimationScene->GetProperty("Loop"),
    0);

  // Create a cue for the timesteps property.
  this->Internal->AnimationCue = vtkSMAnimationCueProxy::SafeDownCast(
    pxm->NewProxy("animation", "AnimationCue"));
  this->Internal->AnimationCue->Delete();
  pqSMAdaptor::setProxyProperty(this->Internal->AnimationScene->GetProperty(
      "Cues"), this->Internal->AnimationCue);
  pqSMAdaptor::setEnumerationProperty(this->Internal->AnimationCue->GetProperty(
      "TimeMode"), "Normalized");
  pqSMAdaptor::setElementProperty(this->Internal->AnimationCue->GetProperty(
      "EndTime"), 1.0);

  // Create a key frame manupulator for the cue.
  this->Internal->KeyframeManipulator = 
    vtkSMKeyFrameAnimationCueManipulatorProxy::SafeDownCast(
      pxm->NewProxy("animation_manipulators", "KeyFrameAnimationCueManipulator"));
  this->Internal->KeyframeManipulator->Delete();

  pqSMAdaptor::setProxyProperty(this->Internal->AnimationCue->GetProperty(
      "Manipulator"), this->Internal->KeyframeManipulator);

  this->Internal->KeyframeManipulator->UpdateVTKObjects();
  this->Internal->AnimationCue->UpdateVTKObjects();
  this->Internal->AnimationScene->UpdateVTKObjects();

}

//-----------------------------------------------------------------------------
pqSimpleAnimationManager::~pqSimpleAnimationManager()
{
  this->Source = 0;
  delete this->Internal;
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
  this->Internal->AnimationScene->Stop();
}

//-----------------------------------------------------------------------------
bool pqSimpleAnimationManager::createTimestepAnimation(
  pqPipelineSource* source, const QString& filename)
{
  if (!this->canAnimate(source))
    {
    return false;
    }

  vtkSMProxy* proxy = source->getProxy();
  vtkSMProperty* timestep = proxy->GetProperty("TimeStep");
  vtkSMDoubleVectorProperty* timestepValues = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      timestep->GetInformationProperty());
  proxy->UpdatePropertyInformation(timestepValues);
  pqRenderModule* activeView = source->getDisplay(0)->getRenderModule(0);

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

  vtkSMAnimationCueProxy* cue = this->Internal->AnimationCue;
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  pqSMAdaptor::setProxyProperty(cue->GetProperty("AnimatedProxy"), proxy);
  pqSMAdaptor::setElementProperty(cue->GetProperty("AnimatedPropertyName"), 
    "TimeStep");
  pqSMAdaptor::setElementProperty(cue->GetProperty("AnimatedElement"), 0);
  cue->UpdateVTKObjects();

  vtkSMProxyProperty* keyFramesProperty = vtkSMProxyProperty::SafeDownCast(
    this->Internal->KeyframeManipulator->GetProperty("KeyFrames"));
  keyFramesProperty->RemoveAllProxies();

  const char* keyframeType = (useDataSetTimesteps)?
    "TimestepKeyFrame" : "RampKeyFrame";

  vtkSMKeyFrameProxy* keyFrame = vtkSMKeyFrameProxy::SafeDownCast(
    pxm->NewProxy("animation_keyframes", keyframeType));
  pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyTime"),   0.0);
  pqSMAdaptor::setElementProperty(keyFrame->GetProperty("NumberOfKeyValues"), 1);
  keyFrame->UpdateVTKObjects();
  pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyValues"), 0.0);
  keyFrame->UpdateVTKObjects();
  keyFramesProperty->AddProxy(keyFrame);
  keyFrame->Delete();

  keyFrame = vtkSMKeyFrameProxy::SafeDownCast(
    pxm->NewProxy("animation_keyframes", keyframeType));
  pqSMAdaptor::setElementProperty(keyFrame->GetProperty("KeyTime"), 1.0);
  pqSMAdaptor::setElementProperty(keyFrame->GetProperty("NumberOfKeyValues"), 1);
  keyFrame->UpdateVTKObjects();
  vtkSMDoubleVectorProperty::SafeDownCast(keyFrame->GetProperty("KeyValues"))
    ->SetElement(0, timestepValues->GetNumberOfElements()-1);
  keyFrame->UpdateVTKObjects();
  keyFramesProperty->AddProxy(keyFrame);
  keyFrame->Delete();
  this->Internal->KeyframeManipulator->UpdateVTKObjects();

  this->Internal->AnimationScene->SetRenderModuleProxy(
    activeView->getRenderModuleProxy());

  activeView->getWidget()->resize(dialogUI.spinBoxWidth->value(),
    dialogUI.spinBoxHeight->value());

  this->Internal->AnimationScene->SetStartTime(0);
  this->Internal->AnimationScene->SetEndTime(
    dialogUI.spinBoxAnimationDuration->value());

  QDialog abortDialog;
  Ui::AbortAnimation abortDialogUI;
  abortDialogUI.setupUi(&abortDialog);
  abortDialog.show();
  QApplication::processEvents();

  QObject::connect(&abortDialog, SIGNAL(accepted()), 
    this, SLOT(abortSavingAnimation()));


  int status = this->Internal->AnimationScene->SaveImages(
    filePrefix.toStdString().c_str(),
    extension.toStdString().c_str(), 
    dialogUI.spinBoxWidth->value(),
    dialogUI.spinBoxHeight->value(),
    dialogUI.spinBoxFrameRate->value(), 0);
 
  activeView->getWidget()->resize(viewSize);   
  return (status == 0);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

