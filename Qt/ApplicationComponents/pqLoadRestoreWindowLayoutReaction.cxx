/*=========================================================================

   Program: ParaView
   Module:  pqLoadRestoreWindowLayoutReaction.cxx

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
    "ParaView Window Layout (*.pwin);;All files (*)");
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
