// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAutoStartInterface_h
#define pqAutoStartInterface_h

#include "pqCoreModule.h"
#include <QtPlugin>

/**
 * Abstract interface for "auto-start" plugins. An auto-start plugin is a
 * plugin that is notified by ParaView when ParaView starts and exits.
 * In practice, no plugin can be loaded until ParaView is up and running, hence
 * the \c startup() is called immediately after the plugin is loaded. When the
 * application is about to exit or if the plugin is about to be unloaded,
 * \c shutdown() is called on all the registered /// interfaces.
 */
class PQCORE_EXPORT pqAutoStartInterface
{
public:
  virtual ~pqAutoStartInterface();

  /**
   * Called once after the ParaView starts. If ParaView is already running when
   * the plugin is loaded, this method will be called when the plugin is loaded.
   */
  virtual void startup() = 0;

  /**
   * Called once before the program shuts down.
   */
  virtual void shutdown() = 0;

protected:
  pqAutoStartInterface();

private:
  Q_DISABLE_COPY(pqAutoStartInterface)
};

Q_DECLARE_INTERFACE(pqAutoStartInterface, "com.kitware/paraview/autostart")

#endif // !pqAutoStartInterface_h
