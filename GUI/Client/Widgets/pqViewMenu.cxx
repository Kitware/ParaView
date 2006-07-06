/*=========================================================================

   Program: ParaView
   Module:    pqViewMenu.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqViewMenu.cxx
/// \date 7/3/2006

#include "pqViewMenu.h"

#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QIcon>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QString>
#include <QToolBar>


class pqViewMenuInternal
{
public:
  pqViewMenuInternal();
  ~pqViewMenuInternal() {}

  QMap<QDockWidget *, QAction *> DockMap;
  QMap<QToolBar *, QAction *> ToolMap;
  QMenu *ViewMenu;
  QMenu *ToolbarMenu;
  QAction *DockSeparator;
};


//-----------------------------------------------------------------------------
pqViewMenuInternal::pqViewMenuInternal()
  : DockMap(), ToolMap()
{
  this->ViewMenu = 0;
  this->ToolbarMenu = 0;
  this->DockSeparator = 0;
}


//-----------------------------------------------------------------------------
pqViewMenu::pqViewMenu(QObject *parent)
{
  this->Internal = new pqViewMenuInternal();
}

pqViewMenu::~pqViewMenu()
{
  // The actions will get cleaned up by Qt.
  delete this->Internal;
}

bool pqViewMenu::eventFilter(QObject* watched, QEvent* e)
{
  if(e->type() == QEvent::Hide || e->type() == QEvent::Show)
    {
    QDockWidget *dock = qobject_cast<QDockWidget *>(watched);
    QToolBar *tool = qobject_cast<QToolBar *>(watched);
    if(dock)
      {
      QMap<QDockWidget *, QAction *>::Iterator iter =
          this->Internal->DockMap.find(dock);
      if(iter != this->Internal->DockMap.end())
        {
        (*iter)->setChecked(e->type() == QEvent::Show);
        }
      }
    else if(tool)
      {
      QMap<QToolBar *, QAction *>::Iterator iter =
          this->Internal->ToolMap.find(tool);
      if(iter != this->Internal->ToolMap.end())
        {
        (*iter)->setChecked(e->type() == QEvent::Show);
        }
      }
    }

  return QObject::eventFilter(watched, e);
}

void pqViewMenu::addActionsToMenuBar(QMenuBar *menubar) const
{
  if(menubar)
    {
    QMenu *menu = menubar->addMenu(tr("&View"));
    menu->setObjectName("ViewMenu");
    this->addActionsToMenu(menu);
    }
}

void pqViewMenu::addActionsToMenu(QMenu *menu) const
{
  if(!menu)
    {
    return;
    }

  // Save the menu for dock window and toolbar additions.
  this->Internal->ViewMenu = menu;

  // Set up the view menu.
  this->Internal->ToolbarMenu = new QMenu("&Toolbars",
      this->Internal->ViewMenu);
  this->Internal->ViewMenu->addMenu(this->Internal->ToolbarMenu);
  this->Internal->ToolbarMenu->setEnabled(this->Internal->ToolMap.size() > 0);

  // Add any dock window and toolbar actions available.
  if(this->Internal->DockMap.size() > 0)
    {
    this->Internal->DockSeparator = this->Internal->ViewMenu->addSeparator();
    }

  QMap<QDockWidget *, QAction *>::Iterator iter =
      this->Internal->DockMap.begin();
  for( ; iter != this->Internal->DockMap.end(); ++iter)
    {
    this->Internal->ViewMenu->addAction(*iter);
    }

  // Add the toolbar actions in alphabetical order.
  QString name;
  QString listName;
  QList<QAction *> actions;
  QMap<QToolBar *, QAction *>::Iterator jter =
      this->Internal->ToolMap.begin();
  QList<QAction *>::Iterator kter;
  for( ; jter != this->Internal->ToolMap.end(); ++jter)
    {
    name = (*jter)->text();
    name.remove("&");
    for(kter = actions.begin(); kter != actions.end(); ++kter)
      {
      listName = (*kter)->text();
      listName.remove("&");
      if(QString::compare(name, listName) < 0)
        {
        actions.insert(kter, *jter);
        break;
        }
      }

    if(kter == actions.end())
      {
      actions.append(*jter);
      }
    }

  for(kter = actions.begin(); kter != actions.end(); ++kter)
    {
    this->Internal->ToolbarMenu->addAction(*kter);
    }
}

QAction *pqViewMenu::getMenuAction(QDockWidget *dock) const
{
  QMap<QDockWidget *, QAction *>::Iterator iter =
      this->Internal->DockMap.find(dock);
  if(iter != this->Internal->DockMap.end())
    {
    return *iter;
    }

  return 0;
}

QAction *pqViewMenu::getMenuAction(QToolBar *tool) const
{
  QMap<QToolBar *, QAction *>::Iterator iter =
      this->Internal->ToolMap.find(tool);
  if(iter != this->Internal->ToolMap.end())
    {
    return *iter;
    }

  return 0;
}

void pqViewMenu::addDockWindow(QDockWidget *dock, const QIcon &icon,
    const QString &text)
{
  // Make sure the dock window isn't added already.
  QMap<QDockWidget *, QAction *>::Iterator iter =
      this->Internal->DockMap.find(dock);
  if(iter != this->Internal->DockMap.end())
    {
    return;
    }

  // Add the dock window to the map.
  QAction *action = new QAction(text, this);
  action->setCheckable(true);
  action->setChecked(dock->isVisible());
  this->connect(action, SIGNAL(triggered(bool)), dock, SLOT(setVisible(bool)));
  if(!icon.isNull())
    {
    action->setIcon(icon);
    }

  this->Internal->DockMap.insert(dock, action);
  dock->installEventFilter(this);

  // Add the dock window action to the menu. Add the separator if
  // this is the first action.
  if(this->Internal->ViewMenu)
    {
    // TODO: If more items are added to the view menu, the insert
    // position needs to be determined.
    if(this->Internal->DockMap.size() == 1)
      {
      this->Internal->DockSeparator = this->Internal->ViewMenu->addSeparator();
      }

    this->Internal->ViewMenu->addAction(action);
    }
}

void pqViewMenu::removeDockWindow(QDockWidget *dock)
{
  // Find the dock window and remove it from the map.
  QMap<QDockWidget *, QAction *>::Iterator iter =
      this->Internal->DockMap.find(dock);
  if(iter == this->Internal->DockMap.end())
    {
    return;
    }

  dock->removeEventFilter(this);
  QAction *action = *iter;
  this->Internal->DockMap.erase(iter);
  if(this->Internal->ViewMenu)
    {
    // Remove the action from the menu.
    this->Internal->ViewMenu->removeAction(action);

    // If this was the last dock window action, remove the separator.
    if(this->Internal->DockMap.size() == 0)
      {
      this->Internal->ViewMenu->removeAction(this->Internal->DockSeparator);
      delete this->Internal->DockSeparator;
      this->Internal->DockSeparator = 0;
      }
    }

  // Clean up the action.
  delete action;
}

void pqViewMenu::addToolBar(QToolBar *tool, const QString &text)
{
  // Make sure the toolbar isn't added already.
  QMap<QToolBar *, QAction *>::Iterator iter =
      this->Internal->ToolMap.find(tool);
  if(iter != this->Internal->ToolMap.end())
    {
    return;
    }

  // Add the toolbar to the map.
  QAction *action = new QAction(text, this);
  action->setCheckable(true);
  action->setChecked(tool->isVisible());
  this->connect(action, SIGNAL(triggered(bool)), tool, SLOT(setVisible(bool)));
  this->Internal->ToolMap.insert(tool, action);
  tool->installEventFilter(this);

  // Add the toolbar action to the menu. Enable the toolbar menu if
  // this is the first toolbar added.
  if(this->Internal->ToolbarMenu)
    {
    // Add the toolbar actions in alphabetical order.
    QString name;
    QString actionName = text;
    actionName.remove("&");
    QAction *before = 0;
    QList<QAction *> actions = this->Internal->ToolbarMenu->actions();
    QList<QAction *>::Iterator jter = actions.begin();
    for( ; jter != actions.end(); ++jter)
      {
      name = (*jter)->text();
      name.remove("&");
      if(QString::compare(actionName, name) < 0)
        {
        before = *jter;
        break;
        }
      }

    if(before)
      {
      this->Internal->ToolbarMenu->insertAction(before, action);
      }
    else
      {
      this->Internal->ToolbarMenu->addAction(action);
      }

    if(this->Internal->ToolMap.size() == 1)
      {
      this->Internal->ToolbarMenu->setEnabled(true);
      }
    }
}

void pqViewMenu::removeToolBar(QToolBar *tool)
{
  // Find the tool bar and remove it from the map.
  QMap<QToolBar *, QAction *>::Iterator iter =
      this->Internal->ToolMap.find(tool);
  if(iter == this->Internal->ToolMap.end())
    {
    return;
    }

  tool->removeEventFilter(this);
  QAction *action = *iter;
  this->Internal->ToolMap.erase(iter);
  if(this->Internal->ToolbarMenu)
    {
    // Remove the action from the menu.
    this->Internal->ToolbarMenu->removeAction(action);

    // If this was the last toolbar, disable the toolbar menu.
    if(this->Internal->ToolMap.size() == 0)
      {
      this->Internal->ToolbarMenu->setEnabled(false);
      }
    }

  // Clean up the action.
  delete action;
}


