// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPluginDockWidgetsBehavior_h
#define pqPluginDockWidgetsBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QMainWindow;

/**
 * @ingroup Behaviors
 * pqPluginDockWidgetsBehavior adds support for loading dock widgets from
 * plugins. In other words, it adds support for plugins created using
 * ADD_PARAVIEW_DOCK_WINDOW.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginDockWidgetsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginDockWidgetsBehavior(QMainWindow* parent = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void addPluginInterface(QObject* iface);

private:
  Q_DISABLE_COPY(pqPluginDockWidgetsBehavior)
};

#endif
