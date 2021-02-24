/*=========================================================================

   Program: ParaView
   Module:    pqPluginActionGroupBehavior.cxx

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
#include "pqPluginActionGroupBehavior.h"

#include "pqActionGroupInterface.h"
#include "pqApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqQtDeprecated.h"

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>

namespace
{
QAction* findExitAction(QMenu* menu)
{
  foreach (QAction* action, menu->actions())
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
  foreach (QAction* existingMenuAction, menuBarActions)
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
  foreach (QObject* iface, pm->interfaces())
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
    foreach (QAction* existingMenuAction, menuBarActions)
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
      foreach (a, agi->actionGroup()->actions())
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
  else if (splitName.size())
  {
    QString msg = QString("Do not know what action group \"%1\" is").arg(splitName[0]);
    qWarning("%s", msg.toLocal8Bit().data());
  }
  else
  {
    qWarning("Action group doesn't have an identifier.");
  }
}
