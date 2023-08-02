// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnimationTimeToolbar.h"

#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqAnimationTimeWidget.h"
#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "vtkCommand.h"
#include "vtkPVGeneralSettings.h"

//-----------------------------------------------------------------------------
void pqAnimationTimeToolbar::constructor()
{
  this->setWindowTitle(tr("Current Time Controls"));
  this->AnimationTimeWidget = new pqAnimationTimeWidget(this);
  this->addWidget(this->AnimationTimeWidget);
  this->connect(pqPVApplicationCore::instance()->animationManager(),
    SIGNAL(activeSceneChanged(pqAnimationScene*)), SLOT(setAnimationScene(pqAnimationScene*)));
  pqCoreUtilities::connect(vtkPVGeneralSettings::GetInstance(), vtkCommand::ModifiedEvent, this,
    SLOT(updateTimeDisplay()));
}

//-----------------------------------------------------------------------------
void pqAnimationTimeToolbar::setAnimationScene(pqAnimationScene* scene)
{
  this->AnimationTimeWidget->setAnimationScene(scene);
}

//-----------------------------------------------------------------------------
void pqAnimationTimeToolbar::updateTimeDisplay()
{
  this->AnimationTimeWidget->render();
}

//-----------------------------------------------------------------------------
pqAnimationTimeWidget* pqAnimationTimeToolbar::animationTimeWidget() const
{
  return this->AnimationTimeWidget;
}
