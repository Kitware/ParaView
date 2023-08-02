// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPluginPathBehavior_h
#define pqPluginPathBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class pqServer;

/**
 * @ingroup Behaviors
 * Applications may want to support auto-loading of plugins from certain
 * locations when a client-server connection is made. In case of ParaView,
 * PV_PLUGIN_PATH environment variable is used to locate such auto-load plugin
 * locations. This behavior encapsulates this functionality.
 * Currently, besides the environment_variable specified in the constructor,
 * this class is hard-coded to look at a few locations relative to the
 * executable. That can be changed in future allow application to customize
 * those locations as well.
 * TODO: This class is work in progress. Due to lack of time I am deferring
 * this until later. Currently pqPluginManager does this work, we need to move
 * the corresponding code to this behavior to allow better customization.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginPathBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginPathBehavior(const QString& environment_variable, QObject* parent = 0);

protected Q_SLOTS:
  void loadDefaultPlugins(pqServer*);

private:
  Q_DISABLE_COPY(pqPluginPathBehavior)
};

#endif
