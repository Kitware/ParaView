// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqActionGroupInterface_h
#define pqActionGroupInterface_h

#include "pqComponentsModule.h"
#include <QtPlugin>
class QActionGroup;

/**
 * interface class for plugins that create QActionGroups
 * for adding actions to menus and toolbars
 */
class PQCOMPONENTS_EXPORT pqActionGroupInterface
{
public:
  pqActionGroupInterface();
  virtual ~pqActionGroupInterface();

  /**
   * the identifier for this action group
   *  return "ToolBar/MyTools to put them in a toolbar called MyTools
   *  return "MenuBar/MyMenu to put the actions under MyMenu
   */
  virtual QString groupName() = 0;

  /**
   * the instance of the QActionGroup that defines the actions
   */
  virtual QActionGroup* actionGroup() = 0;
};

Q_DECLARE_INTERFACE(pqActionGroupInterface, "com.kitware/paraview/actiongroup")

#endif
