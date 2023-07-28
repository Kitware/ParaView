// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPluginActionGroupBehavior_h
#define pqPluginActionGroupBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QMainWindow;

/**
 * @ingroup Behaviors
 * pqPluginActionGroupBehavior adds support for loading menus/toolbars from
 * plugins. In other words, it adds support for plugins created using
 * ADD_PARAVIEW_ACTION_GROUP.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginActionGroupBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginActionGroupBehavior(QMainWindow* parent = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void addPluginInterface(QObject* iface);

private:
  Q_DISABLE_COPY(pqPluginActionGroupBehavior)
};

#endif
