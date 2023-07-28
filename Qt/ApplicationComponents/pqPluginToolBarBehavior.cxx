// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPluginToolBarBehavior.h"

#include "pqApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqToolBarInterface.h"

#include <QMainWindow>
#include <QToolBar>

//-----------------------------------------------------------------------------
pqPluginToolBarBehavior::pqPluginToolBarBehavior(QMainWindow* parentObject)
  : Superclass(parentObject)
{
  pqInterfaceTracker* tracker = pqApplicationCore::instance()->interfaceTracker();
  this->connect(tracker, SIGNAL(interfaceRegistered(QObject*)), SLOT(addPluginInterface(QObject*)));

  // handle any already loaded plugins.
  Q_FOREACH (QObject* iface, tracker->interfaces())
  {
    this->addPluginInterface(iface);
  }
}

//-----------------------------------------------------------------------------
pqPluginToolBarBehavior::~pqPluginToolBarBehavior() = default;

//-----------------------------------------------------------------------------
void pqPluginToolBarBehavior::addPluginInterface(QObject* iface)
{
  if (pqToolBarInterface* tbi = qobject_cast<pqToolBarInterface*>(iface))
  {
    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->parent());
    if (!mainWindow)
    {
      qWarning("Could not find MainWindow. Cannot load actions from the plugin.");
    }
    else if (QToolBar* tb = tbi->toolBar(mainWindow))
    {
      mainWindow->addToolBar(tb);
    }
  }
}
