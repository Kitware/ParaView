// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2007, Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPluginLocationInterface_h
#define pqPluginLocationInterface_h

#include "pqCoreModule.h"
#include <QtPlugin>

/**
 * Abstract interface for storing the file system location of a
 * dyanamically-loaded plugin.
 */
class PQCORE_EXPORT pqPluginLocationInterface
{
public:
  virtual ~pqPluginLocationInterface() = default;

  /**
   * Called once after the plugin is loaded. If `location` is null, the plugin
   * is static and has no file system location.
   */
  virtual void StoreLocation(const char* location) = 0;

protected:
  pqPluginLocationInterface();

private:
  Q_DISABLE_COPY(pqPluginLocationInterface)
};

Q_DECLARE_INTERFACE(pqPluginLocationInterface, "com.kitware/paraview/Location")

#endif // pqPluginLocationInterface_h
