// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPersistentMainWindowStateBehavior.h"

#include "pqApplicationCore.h"
#include "pqSettings.h"

#include <QCoreApplication>
#include <QMainWindow>
#include <QTimer>

#include <cassert>

//-----------------------------------------------------------------------------
pqPersistentMainWindowStateBehavior::pqPersistentMainWindowStateBehavior(QMainWindow* parentWindow)
  : Superclass(parentWindow)
{
  assert(parentWindow != nullptr);
  QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(saveState()));

  // This is done after a slight delay so that any GUI elements that get created
  // as a consequence of loading of the configuration files will have their
  // state restored as well.
  QTimer::singleShot(10, this, SLOT(restoreState()));

  this->restoreState();
}

//-----------------------------------------------------------------------------
pqPersistentMainWindowStateBehavior::~pqPersistentMainWindowStateBehavior() = default;

//-----------------------------------------------------------------------------
void pqPersistentMainWindowStateBehavior::restoreState(QMainWindow* window)
{
  pqApplicationCore::instance()->settings()->restoreState("MainWindow", *window);
}

//-----------------------------------------------------------------------------
void pqPersistentMainWindowStateBehavior::saveState(QMainWindow* window)
{
  pqApplicationCore::instance()->settings()->saveState(*window, "MainWindow");
}

//-----------------------------------------------------------------------------
void pqPersistentMainWindowStateBehavior::restoreState()
{
  QMainWindow* window = qobject_cast<QMainWindow*>(this->parent());
  pqPersistentMainWindowStateBehavior::restoreState(window);
}

//-----------------------------------------------------------------------------
void pqPersistentMainWindowStateBehavior::saveState()
{
  QMainWindow* window = qobject_cast<QMainWindow*>(this->parent());
  pqPersistentMainWindowStateBehavior::saveState(window);
}
