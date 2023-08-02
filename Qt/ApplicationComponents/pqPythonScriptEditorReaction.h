// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPythonScriptEditorReaction_h
#define pqPythonScriptEditorReaction_h

#include <pqCoreUtilities.h>
#include <pqMasterOnlyReaction.h>
#include <pqPVApplicationCore.h>
#include <pqPythonScriptEditor.h>

class PQAPPLICATIONCOMPONENTS_EXPORT pqPythonScriptEditorReaction : public pqMasterOnlyReaction
{
  Q_OBJECT

public:
  pqPythonScriptEditorReaction(QAction* action)
    : pqMasterOnlyReaction(action)
  {
  }

protected Q_SLOTS:
  void onTriggered() override
  {
    pqPythonScriptEditor* pyEditor = pqPythonScriptEditor::getUniqueInstance();
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();

    pyEditor->setPythonManager(pythonManager);
    pyEditor->show();
    pyEditor->raise();
    pyEditor->activateWindow();
  }

private:
  Q_DISABLE_COPY(pqPythonScriptEditorReaction)
};

#endif // pqPythonScriptEditorReaction_h
