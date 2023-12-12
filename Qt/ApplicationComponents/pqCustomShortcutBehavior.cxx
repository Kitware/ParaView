// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCustomShortcutBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCustomizeShortcutsDialog.h"
#include "pqKeySequences.h"
#include "pqModalShortcut.h"
#include "pqSettings.h"

#include <QAction>
#include <QDebug>
#include <QKeySequence>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

//-----------------------------------------------------------------------------
pqCustomShortcutBehavior::pqCustomShortcutBehavior(QMainWindow* parentObject)
  : Superclass(parentObject)
{
  loadMenuItemShortcuts();

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), this,
    SLOT(loadMenuItemShortcuts()));
}

namespace
{

void loadShortcuts(const QList<QAction*>& actions, pqSettings* settings)
{
  for (QAction* action : actions)
  {
    QString actionName = pqCustomizeShortcutsDialog::getActionName(action);
    actionName = actionName.replace("&", "");
    actionName = actionName.replace(" ", "_");
    if (action->menu())
    {
      auto menu = action->menu();
      settings->beginGroup(actionName);
      loadShortcuts(menu->actions(), settings);
      settings->endGroup();
    }
    else if (!actionName.isEmpty())
    {
      // store the default key sequence before overriding it with the
      // user specified one in the settings.
      QKeySequence defaultShortcut = action->shortcut();
      action->setProperty("ParaViewDefaultKeySequence", defaultShortcut);
      auto variant = settings->value(actionName, QVariant());
      if (variant.canConvert<QKeySequence>())
      {
        pqKeySequences::instance().addModalShortcut(
          variant.value<QKeySequence>(), action, qobject_cast<QWidget*>(action->parent()));
      }
    }
  }
}
}

void pqCustomShortcutBehavior::loadMenuItemShortcuts()
{
  auto mainWindow = qobject_cast<QMainWindow*>(this->parent());
  if (!mainWindow)
  {
    qWarning() << "Could not find MainWindow.  Cannot set custom shortcuts.";
    return;
  }

  auto menuBar = mainWindow->menuBar();
  pqSettings* settings = pqApplicationCore::instance()->settings();

  settings->beginGroup("pqCustomShortcuts");
  loadShortcuts(menuBar->actions(), settings);
  settings->endGroup();
}
