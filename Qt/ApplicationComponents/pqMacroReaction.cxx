// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMacroReaction.h"

#include "pqPVApplicationCore.h"
#include "pqPythonManager.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"

//-----------------------------------------------------------------------------
pqMacroReaction::pqMacroReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
  this->enable(pythonManager);
}

//-----------------------------------------------------------------------------
void pqMacroReaction::createMacro()
{
  pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
  if (!pythonManager)
  {
    qCritical("No application wide python manager.");
    return;
  }

  pqServer* server = pqActiveObjects::instance().activeServer();
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(),
    tr("Open Python File to create a Macro:"), QString(),
    tr("Python Files") + QString(" (*.py);;") + tr("All Files") + QString(" (*)"), false, false);
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    pythonManager->addMacro(fileDialog.getSelectedFiles()[0], fileDialog.getSelectedLocation());
  }
}
//-----------------------------------------------------------------------------
void pqMacroReaction::enable(bool canDoAction)
{
  this->parentAction()->setEnabled(canDoAction);
}
