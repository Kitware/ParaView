// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAboutDialogReaction.h"

#include "pqAboutDialog.h"
#include "pqCoreUtilities.h"

//-----------------------------------------------------------------------------
pqAboutDialogReaction::pqAboutDialogReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
void pqAboutDialogReaction::showAboutDialog()
{
  pqAboutDialog about_dialog(pqCoreUtilities::mainWidget());
  about_dialog.exec();
}
