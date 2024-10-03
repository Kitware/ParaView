// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTimeManagerWidget.h"
#include "ui_pqTimeManagerWidget.h"

#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "pqPropertyLinks.h"
#include "vtkCommand.h"
#include "vtkCompositeAnimationPlayer.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <limits>

struct pqTimeManagerWidget::pqInternals
{
  Ui::TimeManagerWidget Ui;
  pqPropertyLinks SceneLinks;

  vtkNew<vtkEventQtSlotConnect> Connector;

  pqInternals(pqTimeManagerWidget* self)
  {
    this->Ui.setupUi(self);
    self->connect(this->Ui.advancedButton, &QToolButton::toggled, self,
      &pqTimeManagerWidget::updateWidgetsVisibility);
    // Note: size of both widgets depends only on this max value...
    // So for now we have widgets larger than actually needed.
    this->Ui.nbOfFramesValue->setMaximum(std::numeric_limits<int>::max());
    this->Ui.strideStep->setMaximum(std::numeric_limits<int>::max());
  }
};

//-----------------------------------------------------------------------------
pqTimeManagerWidget::pqTimeManagerWidget(QWidget* parent)
  : Superclass(parent)
  , Internals(new pqTimeManagerWidget::pqInternals(this))
{
  this->updateWidgetsVisibility();
  pqCoreUtilities::connect(vtkPVGeneralSettings::GetInstance(), vtkCommand::ModifiedEvent, this,
    SLOT(onSettingsChanged()));
  this->onSettingsChanged();
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  QObject::connect(
    mgr, &pqAnimationManager::activeSceneChanged, this, &pqTimeManagerWidget::setActiveScene);
}

//-----------------------------------------------------------------------------
pqTimeManagerWidget::~pqTimeManagerWidget() = default;

//-----------------------------------------------------------------------------
void pqTimeManagerWidget::updateWidgetsVisibility()
{
  bool advanced = this->Internals->Ui.advancedButton->isChecked();
  this->Internals->Ui.stride->setVisible(advanced);
  this->Internals->Ui.timeline->setAdvancedMode(advanced);

  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  auto scene = mgr->getActiveScene();
  if (scene)
  {
    auto sceneProxy = scene->getProxy();
    vtkSMPropertyHelper playModeProp(sceneProxy->GetProperty("PlayMode"));
    int playMode = playModeProp.GetAsInt();
    this->Internals->Ui.nbOfFrames->setEnabled(playMode == vtkCompositeAnimationPlayer::SEQUENCE);
  }
}

//-----------------------------------------------------------------------------
void pqTimeManagerWidget::onSettingsChanged()
{
  this->Internals->Ui.currentTime->render();
}

//-----------------------------------------------------------------------------
void pqTimeManagerWidget::setActiveScene(pqAnimationScene* scene)
{
  this->Internals->SceneLinks.removeAllPropertyLinks();
  this->Internals->Connector->Disconnect();

  this->Internals->Ui.currentTime->setAnimationScene(scene);

  if (!scene)
  {
    return;
  }

  auto sceneProxy = scene->getProxy();
  this->Internals->SceneLinks.addPropertyLink(this->Internals->Ui.strideStep, "value",
    SIGNAL(valueChanged(int)), sceneProxy, sceneProxy->GetProperty("Stride"));
  this->Internals->SceneLinks.addPropertyLink(this->Internals->Ui.nbOfFramesValue, "value",
    SIGNAL(valueChanged(int)), sceneProxy, sceneProxy->GetProperty("NumberOfFrames"));
  this->Internals->Connector->Connect(sceneProxy->GetProperty("PlayMode"),
    vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(updateWidgetsVisibility()));
  this->updateWidgetsVisibility();
}
