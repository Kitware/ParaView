/*=========================================================================

   Program: ParaView
   Module:    pqViewMenuManager.cxx

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
#include "pqViewMenuManager.h"

#include "pqLockPanelsReaction.h"
#include "pqPVApplicationCore.h"
#include "pqSetName.h"
#include "pqTabbedMultiViewWidget.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>

//-----------------------------------------------------------------------------
pqViewMenuManager::pqViewMenuManager(QMainWindow* mainWindow, QMenu* menu)
  : Superclass(mainWindow)
{
  Q_ASSERT(mainWindow != NULL);
  Q_ASSERT(menu != NULL);

  this->Menu = menu;
  this->Window = mainWindow;

  // essential to ensure that the full screen shortcut is setup correctly.
  this->buildMenu();

  QObject::connect(menu, SIGNAL(aboutToShow()), this, SLOT(buildMenu()));
}

namespace
{
bool toolbarLessThan(const QToolBar* tb1, const QToolBar* tb2)
{
  return tb1->toggleViewAction()->text() < tb2->toggleViewAction()->text();
}

bool dockWidgetLessThan(const QDockWidget* tb1, const QDockWidget* tb2)
{
  return tb1->toggleViewAction()->text() < tb2->toggleViewAction()->text();
}
}
//-----------------------------------------------------------------------------
void pqViewMenuManager::buildMenu()
{
  this->Menu->clear();
  QList<QMenu*> child_menus = this->Menu->findChildren<QMenu*>();
  foreach (QMenu* menu, child_menus)
  {
    delete menu;
  }

  QMenu* toolbars = this->Menu->addMenu("Toolbars") << pqSetName("Toolbars");
  QList<QToolBar*> all_toolbars = this->Window->findChildren<QToolBar*>();
  qSort(all_toolbars.begin(), all_toolbars.end(), toolbarLessThan);

  foreach (QToolBar* toolbar, all_toolbars)
  {
    // Nested toolbars should be skipped. These are the non-top-level toolbars
    // such as those on the view frame or other widgets.
    if (toolbar->parentWidget() == this->Window)
    {
      toolbars->addAction(toolbar->toggleViewAction());
    }
  }
  this->Menu->addSeparator();

  QList<QDockWidget*> all_docks = this->Window->findChildren<QDockWidget*>();
  qSort(all_docks.begin(), all_docks.end(), dockWidgetLessThan);
  foreach (QDockWidget* dock_widget, all_docks)
  {
    this->Menu->addAction(dock_widget->toggleViewAction());
  }
  this->Menu->addSeparator();

  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (viewManager)
  {
    QAction* toggleDecoration = this->Menu->addAction("Toggle Borders");
    toggleDecoration->setObjectName("actionToggleWindowBorders");
    toggleDecoration->setShortcut(QKeySequence("Ctrl+D"));
    toggleDecoration->setToolTip("Hide window borders/decoration\
      to stage the scene for a screenshot");
    QObject::connect(
      toggleDecoration, SIGNAL(triggered()), viewManager, SLOT(toggleWidgetDecoration()));
    QAction* fullscreen = this->Menu->addAction("Full Screen");
    fullscreen->setObjectName("actionFullScreen");
    fullscreen->setShortcut(QKeySequence("F11"));
    QObject::connect(fullscreen, SIGNAL(triggered()), viewManager, SLOT(toggleFullScreen()));
  }

  QAction* lockDockWidgetsAction = this->Menu->addAction("Toggle Lock Panels");
  lockDockWidgetsAction->setObjectName("actionLockDockWidgets");
  lockDockWidgetsAction->setToolTip("Toggle locking of dockable panels so they\
    cannot be moved");

  new pqLockPanelsReaction(lockDockWidgetsAction);
}
