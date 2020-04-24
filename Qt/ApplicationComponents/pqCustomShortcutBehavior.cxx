/*=========================================================================

   Program: ParaView
   Module:    pqCustomShortcutBehavior.cxx

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
#include "pqCustomShortcutBehavior.h"

#include "pqActiveObjects.h"
#include "pqCustomizeShortcutsDialog.h"
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

void loadShortcuts(const QList<QAction*>& actions, pqSettings& settings)
{
  for (QAction* action : actions)
  {
    QString actionName = pqCustomizeShortcutsDialog::getActionName(action);
    actionName = actionName.replace("&", "");
    actionName = actionName.replace(" ", "_");
    if (action->menu())
    {
      auto menu = action->menu();
      settings.beginGroup(actionName);
      loadShortcuts(menu->actions(), settings);
      settings.endGroup();
    }
    else if (!actionName.isEmpty())
    {
      // store the default key sequence before overriding it with the
      // user specified one in the settings.
      QKeySequence defaultShortcut = action->shortcut();
      action->setProperty("ParaViewDefaultKeySequence", defaultShortcut);
      auto variant = settings.value(actionName, QVariant());
      if (variant.canConvert<QKeySequence>())
      {
        action->setShortcut(variant.value<QKeySequence>());
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
  pqSettings settings;

  settings.beginGroup("pqCustomShortcuts");
  loadShortcuts(menuBar->actions(), settings);
  settings.endGroup();
}
