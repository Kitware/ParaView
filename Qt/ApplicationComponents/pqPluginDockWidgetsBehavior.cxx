// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPluginDockWidgetsBehavior.h"

#include "pqApplicationCore.h"
#include "pqDockWindowInterface.h"
#include "pqInterfaceTracker.h"

#include <QDockWidget>
#include <QMainWindow>

//-----------------------------------------------------------------------------
pqPluginDockWidgetsBehavior::pqPluginDockWidgetsBehavior(QMainWindow* parentObject)
  : Superclass(parentObject)
{
  pqInterfaceTracker* pm = pqApplicationCore::instance()->interfaceTracker();
  QObject::connect(
    pm, SIGNAL(interfaceRegistered(QObject*)), this, SLOT(addPluginInterface(QObject*)));
  Q_FOREACH (QObject* iface, pm->interfaces())
  {
    this->addPluginInterface(iface);
  }
}

//-----------------------------------------------------------------------------
void pqPluginDockWidgetsBehavior::addPluginInterface(QObject* iface)
{
  pqDockWindowInterface* dwi = qobject_cast<pqDockWindowInterface*>(iface);
  if (!dwi)
  {
    return;
  }
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->parent());
  if (!mainWindow)
  {
    qWarning("Could not find MainWindow. Cannot load dock widgets from the plugin.");
    return;
  }

  // Get the dock area.
  QString area = dwi->dockArea();
  Qt::DockWidgetArea dArea = Qt::LeftDockWidgetArea;
  if (area.compare("Right", Qt::CaseInsensitive) == 0)
  {
    dArea = Qt::RightDockWidgetArea;
  }
  else if (area.compare("Top", Qt::CaseInsensitive) == 0)
  {
    dArea = Qt::TopDockWidgetArea;
  }
  else if (area.compare("Bottom", Qt::CaseInsensitive) == 0)
  {
    dArea = Qt::BottomDockWidgetArea;
  }

  // Create the dock window.
  QDockWidget* dock = dwi->dockWindow(mainWindow);
  mainWindow->addDockWidget(dArea, dock);
}
