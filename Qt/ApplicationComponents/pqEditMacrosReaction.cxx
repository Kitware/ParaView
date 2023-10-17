// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEditMacrosReaction.h"

#include "pqCoreUtilities.h"
#include "pqEditMacrosDialog.h"

QPointer<pqEditMacrosDialog> pqEditMacrosReaction::Dialog = nullptr;

//-----------------------------------------------------------------------------
pqEditMacrosReaction::pqEditMacrosReaction(QAction* action)
  : Superclass(action)
{
}

//-----------------------------------------------------------------------------
void pqEditMacrosReaction::configureMacros()
{
  if (pqEditMacrosReaction::Dialog)
  {
    pqEditMacrosReaction::Dialog->raise();
    pqEditMacrosReaction::Dialog->activateWindow();
  }
  else
  {
    pqEditMacrosReaction::Dialog = new pqEditMacrosDialog(pqCoreUtilities::mainWidget());
    pqEditMacrosReaction::Dialog->setObjectName("EditMacrosDialog");
    pqEditMacrosReaction::Dialog->setAttribute(Qt::WA_DeleteOnClose);
    pqEditMacrosReaction::Dialog->show();
  }
}

//-----------------------------------------------------------------------------
void pqEditMacrosReaction::onTriggered()
{
  pqEditMacrosReaction::configureMacros();
}
