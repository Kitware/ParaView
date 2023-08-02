// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqDockWindowInterface_h
#define pqDockWindowInterface_h

#include "pqComponentsModule.h"
#include <QString>
#include <QtPlugin>
class QDockWidget;
class QWidget;

/**
 * interface class for plugins that add a QDockWindow
 */
class PQCOMPONENTS_EXPORT pqDockWindowInterface
{
public:
  pqDockWindowInterface();
  virtual ~pqDockWindowInterface();

  virtual QString dockArea() const = 0;

  /**
   * Creates a dock window with the given parent
   */
  virtual QDockWidget* dockWindow(QWidget* p) = 0;

private:
  Q_DISABLE_COPY(pqDockWindowInterface)
};

Q_DECLARE_INTERFACE(pqDockWindowInterface, "com.kitware/paraview/dockwindow")

#endif
