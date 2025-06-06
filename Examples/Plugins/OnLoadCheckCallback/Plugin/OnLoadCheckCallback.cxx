// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCoreUtilities.h"
#include "vtkPVLogger.h"
#include "vtkPVPlugin.h"

#include <QMessageBox>
#include <QString>

// The signature of the OnLoadCheck function has to be the following.
// C_EXPORT and C_DECL defines both come from vtkPVPlugin.h header, which export the function in a
// cross platform way.
C_EXPORT bool C_DECL pv_plugin_on_load_check()
{
  // Part specific to testing. When this function is called from ctest, the environment variable
  // below is set to bypass the message box interaction as an XML test in this case can not work.
  QString testingEnvironmentVar = qEnvironmentVariable("CTEST_ON_LOAD_CHECK_TESTING");
  if (!testingEnvironmentVar.isEmpty())
  {
    if (testingEnvironmentVar == "0")
    {
      return false;
    }
    else if (testingEnvironmentVar == "1")
    {
      return true;
    }
  }

  QMessageBox::StandardButton answer = QMessageBox::question(
    pqCoreUtilities::mainWidget(), "Load plugin", "Do you really want to load this plugin?");

  bool loadingAccepted = false;
  switch (answer)
  {
    case QMessageBox::Yes:
      vtkLogF(INFO, "Plugin loading accepted!");
      loadingAccepted = true;
      break;
    case QMessageBox::No:
      vtkLogF(INFO, "Plugin loading aborted!");
      loadingAccepted = false;
      break;
  }

  // If this function returns true, the plugin will be loaded. If false, the plugin won't be
  // loaded.
  return loadingAccepted;
}
