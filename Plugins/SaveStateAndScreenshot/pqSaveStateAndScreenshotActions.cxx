/*=========================================================================

   Program: ParaView
   Module:    pqSaveStateAndScreenshotActions.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqSaveStateAndScreenshotActions.h"
#include "pqSaveStateAndScreenshotReaction.h"

#include <QApplication>
#include <QStyle>

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqSaveStateReaction.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
pqSaveStateAndScreenshotActions::pqSaveStateAndScreenshotActions(QWidget* p)
  : QToolBar("Save State and Screenshot", p)
{
  QIcon saveIcon = qApp->style()->standardIcon(QStyle::SP_DriveFDIcon);
  QAction* saveAction = new QAction(saveIcon, "Save State and Screenshot", this);
  this->addAction(saveAction);

  QIcon settingsIcon = qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);
  QAction* settingsAction = new QAction(settingsIcon, "Configure Save", this);
  this->addAction(settingsAction);

  this->setObjectName("SaveStateAndScreenshot");

  new pqSaveStateAndScreenshotReaction(saveAction, settingsAction);
}
