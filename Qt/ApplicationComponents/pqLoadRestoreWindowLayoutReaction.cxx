// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLoadRestoreWindowLayoutReaction.h"

#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqSettings.h"

#include <QMainWindow>

//-----------------------------------------------------------------------------
pqLoadRestoreWindowLayoutReaction::pqLoadRestoreWindowLayoutReaction(
  bool load, QAction* parentObject)
  : Superclass(parentObject)
  , Load(load)
{
}

//-----------------------------------------------------------------------------
pqLoadRestoreWindowLayoutReaction::~pqLoadRestoreWindowLayoutReaction() = default;

//-----------------------------------------------------------------------------
void pqLoadRestoreWindowLayoutReaction::onTriggered()
{
  QMainWindow* window = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());

  pqFileDialog fileDialog(nullptr, window, this->parentAction()->text(), QString(),
    "ParaView Window Layout (*.pwin);;All files (*)", false);
  fileDialog.setFileMode(this->Load ? pqFileDialog::ExistingFile : pqFileDialog::AnyFile);
  fileDialog.setObjectName("LoadRestoreWindowLayout");

  if (fileDialog.exec() == QDialog::Accepted)
  {
    QString filename = fileDialog.getSelectedFiles()[0];
    // we use pqSettings instead of QSettings since pqSettings is better and
    // saving window position/geometry than QSettings + QWindow.
    pqSettings settings(filename, QSettings::IniFormat);
    if (this->Load)
    {
      settings.restoreState("pqLoadRestoreWindowLayoutReaction", *window);
    }
    else
    {
      settings.clear();
      settings.saveState(*window, "pqLoadRestoreWindowLayoutReaction");
    }
  }
}
