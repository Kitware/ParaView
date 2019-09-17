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
#include "pqPreviewMenuManager.h"
#include "pqSetName.h"
#include "pqTabbedMultiViewWidget.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>

#include <algorithm>
#include <cassert>

//-----------------------------------------------------------------------------
pqViewMenuManager::pqViewMenuManager(QMainWindow* mainWindow, QMenu* menu)
  : Superclass(mainWindow)
{
  assert(mainWindow != NULL);
  assert(menu != NULL);

  this->Menu = menu;
  this->Window = mainWindow;

  this->buildMenu();

  // update variants in the menu before each show.
  this->connect(menu, SIGNAL(aboutToShow()), SLOT(updateMenu()));
}

namespace
{
QString trimShortcut(QString s)
{
  return s.remove('&');
}

bool toolbarLessThan(const QToolBar* tb1, const QToolBar* tb2)
{
  return trimShortcut(tb1->toggleViewAction()->text()) <
    trimShortcut(tb2->toggleViewAction()->text());
}

bool dockWidgetLessThan(const QDockWidget* tb1, const QDockWidget* tb2)
{
  return trimShortcut(tb1->toggleViewAction()->text()) <
    trimShortcut(tb2->toggleViewAction()->text());
}
}

//-----------------------------------------------------------------------------
void pqViewMenuManager::buildMenu()
{
  // this is done to keep consistent with previous behavior.
  this->Menu->clear();

  // Add invariant items to the menu.
  this->ToolbarsMenu = this->Menu->addMenu("Toolbars") << pqSetName("Toolbars");
  this->DockPanelSeparators[0] = this->Menu->addSeparator();
  this->DockPanelSeparators[1] = this->Menu->addSeparator();

  // Add dynamic menu items. We add them here too to ensure that test playback
  // doesn't have to rely on showing the menu before it can activate dynamic
  // actions.
  this->updateMenu();

  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (viewManager)
  {
    new pqPreviewMenuManager((this->Menu->addMenu("Preview") << pqSetName("Preview")));

    QAction* fullscreen = this->Menu->addAction("Full Screen");
    fullscreen->setObjectName("actionFullScreen");
    fullscreen->setShortcut(QKeySequence("F11"));
    QObject::connect(
      fullscreen, &QAction::triggered, viewManager, &pqTabbedMultiViewWidget::toggleFullScreen);

    auto showDecorations = this->Menu->addAction("Show Frame Decorations");
    showDecorations->setCheckable(true);
    showDecorations->setChecked(viewManager->decorationsVisibility());
    QObject::connect(showDecorations, &QAction::triggered, viewManager,
      &pqTabbedMultiViewWidget::setDecorationsVisibility);
    this->ShowFrameDecorationsAction = showDecorations;
  }

  QAction* lockDockWidgetsAction = this->Menu->addAction("Toggle Lock Panels");
  lockDockWidgetsAction->setObjectName("actionLockDockWidgets");
  lockDockWidgetsAction->setToolTip("Toggle locking of dockable panels so they\
    cannot be moved");
  new pqLockPanelsReaction(lockDockWidgetsAction);
}

//-----------------------------------------------------------------------------
void pqViewMenuManager::updateMenu()
{
  // update invariants only.
  assert(this->ToolbarsMenu);

  this->ToolbarsMenu->clear();
  QList<QToolBar*> all_toolbars = this->Window->findChildren<QToolBar*>();
  std::sort(all_toolbars.begin(), all_toolbars.end(), toolbarLessThan);

  foreach (QToolBar* toolbar, all_toolbars)
  {
    // Nested toolbars should be skipped. These are the non-top-level toolbars
    // such as those on the view frame or other widgets.
    if (toolbar->parentWidget() == this->Window)
    {
      this->ToolbarsMenu->addAction(toolbar->toggleViewAction());
    }
  }

  assert(this->DockPanelSeparators[0] && this->DockPanelSeparators[1]);
  // remove all dock panel actions and add them back in.
  QList<QAction*> acts = this->Menu->actions();
  int start = acts.indexOf(this->DockPanelSeparators[0]);
  int end = acts.indexOf(this->DockPanelSeparators[1]);
  assert(start != -1 && end != -1);
  for (int cc = start + 1; cc < end; ++cc)
  {
    this->Menu->removeAction(acts[cc]);
  }

  QList<QDockWidget*> all_docks = this->Window->findChildren<QDockWidget*>();
  std::sort(all_docks.begin(), all_docks.end(), dockWidgetLessThan);
  foreach (QDockWidget* dock_widget, all_docks)
  {
    this->Menu->insertAction(
      /*before*/ this->DockPanelSeparators[1], dock_widget->toggleViewAction());
  }

  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (viewManager && this->ShowFrameDecorationsAction)
  {
    this->ShowFrameDecorationsAction->setChecked(viewManager->decorationsVisibility());
  }
}
