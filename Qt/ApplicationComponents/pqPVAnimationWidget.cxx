// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// Hide PARAVIEW_DEPRECATED_IN_5_12_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

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
