// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCustomizeShortcutsReaction.h"

#include "pqCoreUtilities.h"
#include "pqCustomizeShortcutsDialog.h"

pqCustomizeShortcutsReaction::pqCustomizeShortcutsReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

void pqCustomizeShortcutsReaction::showCustomizeShortcutsDialog()
{
  pqCustomizeShortcutsDialog dialog(pqCoreUtilities::mainWidget());
  dialog.exec();
}
