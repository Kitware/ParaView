// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSetMainWindowTitleReaction.h"

#include "pqCoreUtilities.h"
#include <QInputDialog>

//-----------------------------------------------------------------------------
pqSetMainWindowTitleReaction::pqSetMainWindowTitleReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqSetMainWindowTitleReaction::~pqSetMainWindowTitleReaction() = default;

//-----------------------------------------------------------------------------
void pqSetMainWindowTitleReaction::showSetMainWindowTitleDialog()
{
  bool ok = false;
  QWidget* mainWindow = pqCoreUtilities::mainWidget();
  QString text = QInputDialog::getText(mainWindow, tr("Rename Window"), tr("New title:"),
    QLineEdit::Normal, mainWindow->windowTitle(), &ok);
  if (ok && !text.isEmpty())
  {
    mainWindow->setWindowTitle(text);
  }
}
