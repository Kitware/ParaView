// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqServerManagerModelInterface_h
#define pqServerManagerModelInterface_h

#include "pqCoreModule.h"
#include <QtPlugin>

class vtkSMProxy;
class pqServer;
class pqProxy;

/**
 * pqServerManagerModelInterface defines an interface that can be used to
 * register new types of pqProxy subclasses to create when a vtkSMProxy is
 * registered with the ProxyManager in a ParaView application.
 */
class PQCORE_EXPORT pqServerManagerModelInterface
{
public:
  pqServerManagerModelInterface();
  virtual ~pqServerManagerModelInterface();

  /**
   * Creates a pqProxy subclass for the vtkSMProxy given the details for its
   * registration with the proxy manager.
   * \arg \c regGroup - registration group for the proxy.
   * \arg \c regName  - registration name for the proxy.
   * \arg \c proxy    - vtkSMProxy instance to create the pqProxy for.
   * \arg \c server   - pqServer instance on which the proxy is present.
   */
  virtual pqProxy* createPQProxy(
    const QString& regGroup, const QString& regName, vtkSMProxy* proxy, pqServer* server) const = 0;
};

Q_DECLARE_INTERFACE(pqServerManagerModelInterface, "com.kitware/paraview/servermanagermodel")

#endif
