// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  : QToolBar(tr("Save State and Screenshot"), p)
{
  QIcon saveIcon = qApp->style()->standardIcon(QStyle::SP_DriveFDIcon);
  QAction* saveAction = new QAction(saveIcon, tr("Save State and Screenshot"), this);
  this->addAction(saveAction);

  QIcon settingsIcon = qApp->style()->standardIcon(QStyle::SP_DirHomeIcon);
  QAction* settingsAction = new QAction(settingsIcon, tr("Configure Save"), this);
  this->addAction(settingsAction);

  this->setObjectName("SaveStateAndScreenshot");

  new pqSaveStateAndScreenshotReaction(saveAction, settingsAction);
}
