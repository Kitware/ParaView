// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPlugin_h
#define pqPlugin_h

#include <QObjectList>

/**
 * the main plugin interface for GUI extensions one instance of this resides in
 * the plugin
 */
class pqPlugin
{
public:
  /**
   * destructor
   */
  virtual ~pqPlugin() {}

  virtual QObjectList interfaces() = 0;
};

Q_DECLARE_INTERFACE(pqPlugin, "com.kitware/paraview/plugin")

#endif
