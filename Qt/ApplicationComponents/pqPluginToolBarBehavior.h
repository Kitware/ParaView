// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPluginToolBarBehavior_h
#define pqPluginToolBarBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

/**
 * @class pqPluginToolBarBehavior
 * @ingroup Behaviors
 * @brief Behavior that monitors loaded plugins to handle any implementations of
 *        pqToolBarInterface.
 *
 * pqPluginToolBarBehavior monitors loaded plugins to handle implementations of
 * pqToolBarInterface. If any found, this will execute the appropriate calls to
 * create and add the toolbar to the UI.
 *
 * Plugins that add toolbars generally use `add_paraview_toolbar` macro in their
 * CMake code.
 */

class QMainWindow;
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginToolBarBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginToolBarBehavior(QMainWindow* parent = nullptr);
  ~pqPluginToolBarBehavior() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void addPluginInterface(QObject* iface);

private:
  Q_DISABLE_COPY(pqPluginToolBarBehavior)
};

#endif
