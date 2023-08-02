// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
