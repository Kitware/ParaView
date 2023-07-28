// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef ExampleContextMenu_h
#define ExampleContextMenu_h

#include "pqContextMenuInterface.h"

/**
 * @class ExampleContextMenu
 * @brief Add a context menu to pqViews.
 *
 * ExampleContextMenu is the interface which plugins adding a context menu
 * to pqViews should implement. One would typically use the `add_paraview_context_menu`
 * CMake macro to ensure an instance of the class is created and registered with
 * the pqPipelineContextMenuBehavior class (which is responsible for creating
 * the context menu).
 */

class ExampleContextMenu
  : public QObject
  , public pqContextMenuInterface
{
  Q_OBJECT
  Q_INTERFACES(pqContextMenuInterface)
public:
  ExampleContextMenu();
  ExampleContextMenu(QObject* parent);
  ~ExampleContextMenu() override;

  /// This method is called when a context menu is requested,
  /// usually by a right click in a pqView instance.
  ///
  /// This method should return true if the context is one
  /// handled by this instance (and presumably it will modify
  /// the provided QMenu) and false otherwise (indicating the
  /// context is not one this instance handles).
  ///
  /// Each registered interface is called until one returns
  /// true, so your implementation should return false as
  /// quickly as possible.
  ///
  /// If dataContext is a pqDataRepresentation and holds
  /// multiblock data, the dataBlockContext is a list of
  /// block IDs to which the menu actions should apply.
  bool contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
    pqRepresentation* dataContext, const QList<unsigned int>& dataBlockContext) const override;

  /// Run before the default context menu (which has a priority of 0).
  int priority() const override { return 1; }

public slots:
  virtual void twiddleThumbsAction();

private:
  Q_DISABLE_COPY(ExampleContextMenu)
};

#endif
