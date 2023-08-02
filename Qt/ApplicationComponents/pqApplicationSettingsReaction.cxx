// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqApplicationSettingsReaction.h"

#include "pqCoreUtilities.h"
#include "pqSettingsDialog.h"

QPointer<pqSettingsDialog> pqApplicationSettingsReaction::Dialog;

//-----------------------------------------------------------------------------
pqApplicationSettingsReaction::pqApplicationSettingsReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqApplicationSettingsReaction::~pqApplicationSettingsReaction()
{
  delete pqApplicationSettingsReaction::Dialog;
}

//-----------------------------------------------------------------------------
void pqApplicationSettingsReaction::showApplicationSettingsDialog(const QString& tabName)
{
  if (!pqApplicationSettingsReaction::Dialog)
  {
    pqApplicationSettingsReaction::Dialog = new pqSettingsDialog(pqCoreUtilities::mainWidget());
    pqApplicationSettingsReaction::Dialog->setObjectName("ApplicationSettings");
    pqApplicationSettingsReaction::Dialog->setAttribute(Qt::WA_DeleteOnClose, true);
  }
  pqApplicationSettingsReaction::Dialog->show();
  pqApplicationSettingsReaction::Dialog->raise();
  pqApplicationSettingsReaction::Dialog->showTab(tabName);
}
