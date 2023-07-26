// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqExampleVisualizationsDialogReaction.h"

#include "pqCoreUtilities.h"
#include "pqExampleVisualizationsDialog.h"

//-----------------------------------------------------------------------------
pqExampleVisualizationsDialogReaction::pqExampleVisualizationsDialogReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqExampleVisualizationsDialogReaction::~pqExampleVisualizationsDialogReaction() = default;

//-----------------------------------------------------------------------------
void pqExampleVisualizationsDialogReaction::showExampleVisualizationsDialog()
{
  pqExampleVisualizationsDialog dialog(pqCoreUtilities::mainWidget());
  dialog.exec();
}
