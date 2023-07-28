// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqContextMenuInterface_h
#define pqContextMenuInterface_h

#include "pqComponentsModule.h"
#include <QList>
#include <QObject>

/**
 * @class pqContextMenuInterface
 * @brief Interface class for plugins that add a context menu to pqViews.
 *
 * pqContextMenuInterface is the interface which plugins adding a context menu
 * to pqViews should implement. One would typically use the `add_paraview_context_menu`
 * CMake macro to ensure an instance of the class is created and registered with
 * the pqPipelineContextMenuBehavior class (which is responsible for creating
 * the context menu).
 */
class QMenu;
class pqView;
class pqRepresentation;

class PQCOMPONENTS_EXPORT pqContextMenuInterface
{
public:
  pqContextMenuInterface();
  virtual ~pqContextMenuInterface();

  /// This method is called when a context menu is requested,
  /// usually by a right click in a pqView instance.
  ///
  /// This method should return true if (a) the context is one
  /// handled by this instance (and presumably it will modify
  /// the provided QMenu); and (b) this instance should be the
  /// last interface to contribute to the menu.
  /// Returning false indicates the context is not one this
  /// instance handles *or* that interfaces with a lower priority
  /// may modify the menu.
  ///
  /// Each registered interface is called in order of descending
  /// priority until one returns true, so your implementation
  /// should return false as quickly as possible.
  ///
  /// If dataContext is a pqDataRepresentation and holds
  /// multiblock data, the dataBlockContext is a list of
  /// block IDs to which the menu actions should apply.
  virtual bool contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
    pqRepresentation* dataContext, const QList<unsigned int>& dataBlockContext) const;

  /**
   * This is a newer variant of the contextMenu where the dataBlockContext is
   * provided as selectors instead of composite ids. Selectors are more reliable
   * especially when dealing with partitioned datasets and their collections in
   * distributed mode and hence should be preferred.
   */
  virtual bool contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
    pqRepresentation* dataContext, const QStringList& dataBlockContext) const;

  /// This method's return value is used to set the precedence of the interface.
  ///
  /// Interfaces with greater priority are invoked before others and may cause
  /// menu-building to terminate early.
  /// ParaView's default context-menu interface uses a priority of 0 and returns false.
  ///
  /// If you wish to modify the default menu, assign a negative priority to your interface.
  /// If you wish to override the default menu, assign a positive priority to your interface
  /// and have `contextMenu()` return true.
  virtual int priority() const { return -1; }

private:
  Q_DISABLE_COPY(pqContextMenuInterface)
};

Q_DECLARE_INTERFACE(pqContextMenuInterface, "com.kitware/paraview/contextmenu")
#endif
