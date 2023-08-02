// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCategoryToolbarsBehavior_h
#define pqCategoryToolbarsBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QList>
#include <QObject>
#include <QPointer>

class pqProxyGroupMenuManager;
class QMainWindow;
class QAction;

/**
 * @ingroup Behaviors
 * pqCategoryToolbarsBehavior is used when the application wants to enable
 * categories from a pqProxyGroupMenuManager to show up in a toolbar.
 * ex. One may want to have a toolbar listing all the filters in "Common"
 * category. This behavior also ensures that as plugins are loaded, if new
 * categories request that the be added as a toolbar, new toolbars for those
 * are added and also if new items get added to a category already shown as a
 * toolbar, then the toolbar is updated.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCategoryToolbarsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCategoryToolbarsBehavior(pqProxyGroupMenuManager* menuManager, QMainWindow* mainWindow);

protected Q_SLOTS:
  /**
   * Called when menuManager fires the menuPopulated() signal.
   */
  void updateToolbars();

  /**
   * This slot gets attached to a pqEventDispatcher so that some toolbars
   * can be hidden before each test starts (to prevent small test-image differences
   * due to layout differences between machines).
   */
  void prepareForTest();

private:
  Q_DISABLE_COPY(pqCategoryToolbarsBehavior)

  QPointer<QMainWindow> MainWindow;
  QPointer<pqProxyGroupMenuManager> MenuManager;
  QList<QAction*> ToolbarsToHide;
};

#endif
