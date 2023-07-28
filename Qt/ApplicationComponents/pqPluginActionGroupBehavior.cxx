// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPluginActionGroupBehavior.h"

#include "pqActionGroupInterface.h"
#include "pqApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqQtDeprecated.h"

#include <QActionGroup>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>

namespace
{
QAction* findExitAction(QMenu* menu)
{
  Q_FOREACH (QAction* action, menu->actions())
  {
    QString name = action->text().toLower();
    name.remove('&');
    if (name == "exit" || name == "quit")
    {
      return action;
    }
  }
  return nullptr;
}

QAction* findHelpMenuAction(QMenuBar* menubar)
{
  QList<QAction*> menuBarActions = menubar->actions();
  Q_FOREACH (QAction* existingMenuAction, menuBarActions)
  {
    QString menuName = existingMenuAction->text().toLower();
    menuName.remove('&');
    if (menuName == "help")
    {
      return existingMenuAction;
    }
  }
  return nullptr;
}
}

//-----------------------------------------------------------------------------
pqPluginActionGroupBehavior::pqPluginActionGroupBehavior(QMainWindow* parentObject)
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
void pqPluginActionGroupBehavior::addPluginInterface(QObject* iface)
{
  pqActionGroupInterface* agi = qobject_cast<pqActionGroupInterface*>(iface);
  if (!agi)
  {
    return;
  }

  QMainWindow* mainWindow = qobject_cast<QMainWindow*>(this->parent());
  if (!mainWindow)
  {
    qWarning("Could not find MainWindow. Cannot load actions from the plugin.");
    return;
  }

  QString name = agi->groupName();
  QStringList splitName = name.split('/', PV_QT_SKIP_EMPTY_PARTS);

  if (splitName.size() == 2 && splitName[0] == "ToolBar")
  {
    QToolBar* tb = new QToolBar(splitName[1], mainWindow);
    tb->setObjectName(splitName[1]);
    tb->addActions(agi->actionGroup()->actions());
    mainWindow->addToolBar(tb);
  }
  else if (splitName.size() == 2 && splitName[0] == "MenuBar")
  {
    QMenu* menu = nullptr;
    QList<QAction*> menuBarActions = mainWindow->menuBar()->actions();
    Q_FOREACH (QAction* existingMenuAction, menuBarActions)
    {
      QString menuName = existingMenuAction->text();
      menuName.remove('&');
      if (menuName == splitName[1])
      {
        menu = existingMenuAction->menu();
        break;
      }
    }
    if (menu)
    {
      QAction* exitAction = ::findExitAction(menu);

      // Add to existing menu (before exit action, if exists).
      QAction* a;
      if (exitAction == nullptr)
      {
        menu->addSeparator();
      }
      Q_FOREACH (a, agi->actionGroup()->actions())
      {
        menu->insertAction(exitAction, a);
      }
      if (exitAction != nullptr)
      {
        menu->insertSeparator(exitAction);
      }
    }
    else
    {
      // Create new menu.
      menu = new QMenu(splitName[1], mainWindow);
      menu->setObjectName(splitName[1]);
      menu->addActions(agi->actionGroup()->actions());
      // insert new menus before the Help menu is possible.
      mainWindow->menuBar()->insertMenu(::findHelpMenuAction(mainWindow->menuBar()), menu);
    }
  }
  else if (!splitName.empty())
  {
    QString msg = QString("Do not know what action group \"%1\" is").arg(splitName[0]);
    qWarning("%s", msg.toUtf8().data());
  }
  else
  {
    qWarning("Action group doesn't have an identifier.");
  }
}
