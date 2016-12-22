/*=========================================================================

   Program: ParaView
   Module:    pqPythonShellReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqPythonShellReaction.h"

#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "vtkPVConfig.h"

#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonDialog.h"
#include "pqPythonManager.h"
#endif

//-----------------------------------------------------------------------------
pqPythonShellReaction::pqPythonShellReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  parentObject->setEnabled(true);
}

//-----------------------------------------------------------------------------
void pqPythonShellReaction::showPythonShell()
{
#ifdef PARAVIEW_ENABLE_PYTHON
  pqPythonManager* manager = pqPVApplicationCore::instance()->pythonManager();
  if (manager)
  {
    pqPythonDialog* dialog = manager->pythonShellDialog();
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
  }
#else
  pqCoreUtilities::promptUser("pqPythonShellReaction::NoPython", QMessageBox::Critical,
    tr("Python support not enabled"), tr("Python support is required to use the Python shell, but "
                                         "Python is not available in this build."),
    QMessageBox::Ok | QMessageBox::Save);
#endif
}

//-----------------------------------------------------------------------------
void pqPythonShellReaction::executeScript(const char* filename)
{
#ifdef PARAVIEW_ENABLE_PYTHON
  pqPythonManager* manager = pqPVApplicationCore::instance()->pythonManager();
  if (manager)
  {
    manager->executeScript(filename);
  }
#else
  Q_UNUSED(filename);
  pqCoreUtilities::promptUser("pqPythonShellReaction::NoPythonExecuteScript", QMessageBox::Critical,
    tr("Python support not enabled"), tr("Python support is required to execute Python script, but "
                                         "Python is not available in this build."),
    QMessageBox::Ok | QMessageBox::Save);
#endif
}
