// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDefaultContextMenu_h
#define pqDefaultContextMenu_h

#include "pqApplicationComponentsModule.h"
#include "pqContextMenuInterface.h"
#include "vtkType.h"
#include <QList> // needed for QList.
#include <QObject>
#include <QPoint> // needed for QPoint.
#include <QPointer>

class pqDataRepresentation;
class pqPipelineRepresentation;
class pqView;
class QAction;
class QMenu;

/**
 * This interface creates ParaView's default context menu in render views.
 * It has priority 0, so you can modify the QMenu it creates
 * by using a lower (negative) priority in your own custom interface.
 * You can eliminate the default menu by assigning your custom interface a
 * positive priority and have its contextMenu() method return true.
 *
 * @sa pqPipelineContextMenuBehavior
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqDefaultContextMenu
  : public QObject
  , public pqContextMenuInterface
{
  Q_OBJECT
  Q_INTERFACES(pqContextMenuInterface)
  using Superclass = pqContextMenuInterface;

public:
  pqDefaultContextMenu(QObject* parent = nullptr);
  ~pqDefaultContextMenu() override;

  ///@{
  /**
   * Create ParaView's default context menu.
   * It will always return false (i.e., allow lower-priority menus to append/modify
   * its output).
   */
  using pqContextMenuInterface::contextMenu;
  bool contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
    pqRepresentation* dataContext, const QList<unsigned int>& dataBlockContext) const override;
  ///@}

  /**
   * The priority is used to order calls to pqContextMenuInterface::contextMenu among
   * all registered instances of pqContextMenuInterface.
   */
  int priority() const override { return 0; }

protected Q_SLOTS:

  /**
   * called to hide the representation.
   */
  void hide();

  /**
   * called to change the representation type.
   */
  void reprTypeChanged(QAction* action);

  /**
   * called to change the coloring mode.
   */
  void colorMenuTriggered(QAction* action);

  /**
   * called to show all blocks.
   */
  void showAllBlocks() const;

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * called to build the color arrays submenu.
   */
  virtual void buildColorFieldsMenu(pqPipelineRepresentation* pipelineRepr, QMenu* menu) const;

  mutable QPoint Position;
  mutable QPointer<pqDataRepresentation> PickedRepresentation;

private:
  Q_DISABLE_COPY(pqDefaultContextMenu)
};

#endif
