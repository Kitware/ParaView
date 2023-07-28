// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqBlockContextMenu_h
#define pqBlockContextMenu_h

#include "pqApplicationComponentsModule.h" // for exports
#include "pqContextMenuInterface.h"
#include <QObject>

/**
 * @class pqBlockContextMenu
 * @brief add context menu items to control block appearance properties.
 *
 * pqBlockContextMenu is a concrete implementation of the pqContextMenuInterface
 * that add context-menu actions to control display properties for chosen
 * blocks. This is currently only supported for render view.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqBlockContextMenu
  : public QObject
  , public pqContextMenuInterface
{
  Q_OBJECT
  Q_INTERFACES(pqContextMenuInterface)
  typedef QObject Superclass;

public:
  pqBlockContextMenu(QObject* parent = nullptr);
  ~pqBlockContextMenu() override;

  using pqContextMenuInterface::contextMenu;
  bool contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
    pqRepresentation* dataContext, const QStringList& dataBlockContext) const override;

  int priority() const override { return 1; }

private:
  Q_DISABLE_COPY(pqBlockContextMenu)
};

#endif
