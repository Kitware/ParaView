// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqWelcomeDialogReaction.h"

#include "pqCoreUtilities.h"
#include "pqWelcomeDialog.h"

//-----------------------------------------------------------------------------
pqWelcomeDialogReaction::pqWelcomeDialogReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
void pqWelcomeDialogReaction::showWelcomeDialog()
{
  pqWelcomeDialog welcomeDialog(pqCoreUtilities::mainWidget());
  welcomeDialog.exec();
}
