// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLockPanelsReaction.h"

#include "pqLockPanelsBehavior.h"

#include <QMainWindow>

//-----------------------------------------------------------------------------
pqLockPanelsReaction::pqLockPanelsReaction(QAction* action)
  : Superclass(action)
{
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));
}

//-----------------------------------------------------------------------------
pqLockPanelsReaction::~pqLockPanelsReaction() = default;

//-----------------------------------------------------------------------------
void pqLockPanelsReaction::actionTriggered()
{
  pqLockPanelsBehavior::toggleLockPanels();
}
