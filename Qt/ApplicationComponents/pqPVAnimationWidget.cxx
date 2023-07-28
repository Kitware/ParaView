// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPVAnimationWidget.h"

#include "pqAnimationManager.h"
#include "pqPVApplicationCore.h"

//-----------------------------------------------------------------------------
pqPVAnimationWidget::pqPVAnimationWidget(QWidget* parentObject)
  : Superclass(parentObject)
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  QObject::connect(
    mgr, SIGNAL(activeSceneChanged(pqAnimationScene*)), this, SLOT(setScene(pqAnimationScene*)));
}
