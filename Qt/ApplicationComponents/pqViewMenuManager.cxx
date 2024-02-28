// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqViewMenuManager.h"

#include "pqEqualizeLayoutReaction.h"
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
  assert(mainWindow != nullptr);
  assert(menu != nullptr);

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
  this->ToolbarsMenu = this->Menu->addMenu(QIcon(":/pqWidgets/Icons/pqToolbar.svg"), tr("Toolbars"))
    << pqSetName("Toolbars");
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
    new pqPreviewMenuManager(
      (this->Menu->addMenu(QIcon(":/pqWidgets/Icons/pqPreview.svg"), tr("Preview"))
        << pqSetName("Preview")));

    QAction* fullscreen = this->Menu->addAction(
      QIcon(":/pqWidgets/Icons/pqFullscreen.svg"), tr("Full Screen (layout)"));
    fullscreen->setObjectName("actionFullScreen");
    fullscreen->setShortcut(QKeySequence(Qt::Key_F11));
    fullscreen->setAutoRepeat(false);
    QObject::connect(
      fullscreen, &QAction::triggered, viewManager, &pqTabbedMultiViewWidget::toggleFullScreen);

    QAction* activeViewFullscreen = this->Menu->addAction(
      QIcon(":/pqWidgets/Icons/pqActiveViewFullscreen.svg"), tr("Full Screen (active view)"));
    activeViewFullscreen->setObjectName("actionFullScreenActiveView");
    activeViewFullscreen->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_F11));
    QObject::connect(activeViewFullscreen, &QAction::triggered, viewManager,
      &pqTabbedMultiViewWidget::toggleFullScreenActiveView);

    // Disable one fullscreen mode when the other is in use
    QObject::connect(viewManager, &pqTabbedMultiViewWidget::fullScreenEnabled, activeViewFullscreen,
      [activeViewFullscreen](bool enabled) { activeViewFullscreen->setEnabled(!enabled); });
    QObject::connect(viewManager, &pqTabbedMultiViewWidget::fullScreenActiveViewEnabled, fullscreen,
      [fullscreen](bool enabled) { fullscreen->setEnabled(!enabled); });

    auto showDecorations = this->Menu->addAction(tr("Show Frame Decorations"));
    showDecorations->setCheckable(true);
    showDecorations->setChecked(viewManager->decorationsVisibility());
    QObject::connect(showDecorations, &QAction::triggered, viewManager,
      &pqTabbedMultiViewWidget::setDecorationsVisibility);
    this->ShowFrameDecorationsAction = showDecorations;
  }

  QAction* lockDockWidgetsAction =
    this->Menu->addAction(QIcon(":/pqWidgets/Icons/pqToggleLock.svg"), tr("Toggle Lock Panels"));
  lockDockWidgetsAction->setObjectName("actionLockDockWidgets");
  lockDockWidgetsAction->setToolTip(tr("Toggle locking of dockable panels so they\
    cannot be moved"));
  new pqLockPanelsReaction(lockDockWidgetsAction);

  QMenu* equalizeMenu = this->Menu->addMenu(tr("Equalize Views")) << pqSetName("equalizeViewsMenu");
  QAction* equalizeViewsHorizontallyAction = equalizeMenu->addAction(tr("Horizontally"));
  equalizeViewsHorizontallyAction->setObjectName("equalizeViewsHorizontallyAction");
  equalizeViewsHorizontallyAction->setToolTip(
    tr("Equalize layout so views are evenly sized horizontally"));
  new pqEqualizeLayoutReaction(
    pqEqualizeLayoutReaction::Orientation::HORIZONTAL, equalizeViewsHorizontallyAction);

  QAction* equalizeViewsVerticallyAction = equalizeMenu->addAction(tr("Vertically"));
  equalizeViewsVerticallyAction->setObjectName("equalizeViewsVerticallyAction");
  equalizeViewsVerticallyAction->setToolTip(
    tr("Equalize layout so views are evenly sized vertically"));
  new pqEqualizeLayoutReaction(
    pqEqualizeLayoutReaction::Orientation::VERTICAL, equalizeViewsVerticallyAction);

  QAction* equalizeViewsBothAction = equalizeMenu->addAction(tr("Both"));
  equalizeViewsBothAction->setObjectName("equalizeViewsBothAction");
  equalizeViewsBothAction->setToolTip(
    tr("Equalize layout so views are evenly sized horizontally and vertically"));
  new pqEqualizeLayoutReaction(
    pqEqualizeLayoutReaction::Orientation::BOTH, equalizeViewsBothAction);
}

//-----------------------------------------------------------------------------
void pqViewMenuManager::updateMenu()
{
  // update invariants only.
  assert(this->ToolbarsMenu);

  this->ToolbarsMenu->clear();
  QList<QToolBar*> all_toolbars = this->Window->findChildren<QToolBar*>();
  std::sort(all_toolbars.begin(), all_toolbars.end(), toolbarLessThan);

  Q_FOREACH (QToolBar* toolbar, all_toolbars)
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
  Q_FOREACH (QDockWidget* dock_widget, all_docks)
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
