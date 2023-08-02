// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqToolBarInterface_h
#define pqToolBarInterface_h

#include "pqComponentsModule.h"
#include <QObject>

/**
 * @class pqToolBarInterface
 * @brief Interface class for plugins that add a QToolBar to the main window.
 *
 * pqToolBarInterface is the interface which plugins adding a QToolBar to the
 * main window should implement. One would typically use `add_paraview_toolbar`
 * CMake macro for the same.
 */
class QToolBar;
class PQCOMPONENTS_EXPORT pqToolBarInterface
{
public:
  pqToolBarInterface();
  virtual ~pqToolBarInterface();

  virtual QToolBar* toolBar(QWidget* parentWidget) = 0;

private:
  Q_DISABLE_COPY(pqToolBarInterface)
};

Q_DECLARE_INTERFACE(pqToolBarInterface, "com.kitware/paraview/toolbar")
#endif
