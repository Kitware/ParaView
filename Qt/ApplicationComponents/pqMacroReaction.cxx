/*=========================================================================

   Program: ParaView
   Module:    pqEditTraceReaction.cxx

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
#include "pqMacroReaction.h"

#include "pqPVApplicationCore.h"
#include "pqPythonManager.h"

#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPythonMacroSupervisor.h"

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

  pqFileDialog fileDialog(nullptr, pqCoreUtilities::mainWidget(),
    tr("Open Python File to create a Macro:"), QString(),
    tr("Python script (*.py);;All Files (*)"));
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    pythonManager->addMacro(fileDialog.getSelectedFiles()[0]);
  }
}
//-----------------------------------------------------------------------------
void pqMacroReaction::enable(bool canDoAction)
{
  this->parentAction()->setEnabled(canDoAction);
}
