// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqViewMenuManager_h
#define pqViewMenuManager_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QPointer>

class QMenu;
class QMainWindow;
class QAction;

/**
 * pqViewMenuManager keeps ParaView View menu populated with all the available
 * dock widgets and toolbars. This needs special handling since new dock
 * widget/toolbars may get added when plugins are loaded.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqViewMenuManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqViewMenuManager(QMainWindow* mainWindow, QMenu* menu);

protected Q_SLOTS:
  /**
   * build the menu from scratch. Clears all existing items in the menu before
   * building it.
   */
  void buildMenu();

  /**
   * This is called to update items in the menu that are not static and may
   * change as a result of loading of plugins, for example viz. actions for
   * controlling visibilities of toolbars are dock panels.
   * It removes any actions for those currently present and adds actions for
   * toolbars and panels in the application. This slot is called when the menu
   * triggers `aboutToShow` signal.
   */
  virtual void updateMenu();

protected: // NOLINT(readability-redundant-access-specifiers)
  QPointer<QMenu> Menu;
  QPointer<QMenu> ToolbarsMenu;
  QPointer<QAction> DockPanelSeparators[2];
  QPointer<QAction> ShowFrameDecorationsAction;

private:
  Q_DISABLE_COPY(pqViewMenuManager)
  QMainWindow* Window;
};

#endif
