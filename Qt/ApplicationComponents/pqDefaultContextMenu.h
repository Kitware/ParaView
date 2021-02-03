/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
class PQAPPLICATIONCOMPONENTS_EXPORT pqDefaultContextMenu : public QObject,
                                                            public pqContextMenuInterface
{
  Q_OBJECT
  Q_INTERFACES(pqContextMenuInterface)
  using Superclass = pqContextMenuInterface;

public:
  pqDefaultContextMenu(QObject* parent = 0);
  ~pqDefaultContextMenu() override;

  /**
   * Create ParaView's default context menu.
   * It will always return false (i.e., allow lower-priority menus to append/modify
   * its output).
   */
  bool contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
    pqRepresentation* dataContext, const QList<unsigned int>& dataBlockContext) const override;

  /// The priority is used to order calls to pqContextMenuInterface::contextMenu among
  /// all registered instances of pqContextMenuInterface.
  int priority() const override { return 0; }

protected Q_SLOTS:

  /**
  * called to hide the representation.
  */
  void hide();

  /**
  * called to hide the block. the action which emits the signal will
  * contain the block index in its data().
  */
  void hideBlock();

  /**
  * called to show only the selected block. the action which emits the
  * signal will contain the block index in its data().
  */
  void showOnlyBlock();

  /**
  * called to show all blocks.
  */
  void showAllBlocks();

  /**
  * called to unset the visibility flag for the block. after this call the
  * block will inherit the visibility from its parent. the action which
  * emits the signal will contain the block index in its data()
  */
  void unsetBlockVisibility();

  /**
  * called to set the color for the block. the action which emits the
  * signal will contain the block index in its data()
  */
  void setBlockColor();

  /**
  * called to unset the color for the block. the action which emits the
  * signal will contain the block index in its data()
  */
  void unsetBlockColor();

  /**
  * called to set the opacity for the block. the action which emits the
  * signal will contain the block index in its data()
  */
  void setBlockOpacity();

  /**
  * called to unset the opacity for the block. the action which emits the
  * signal will contain the block index in its data()
  */
  void unsetBlockOpacity();

  /**
  * called to change the representation type.
  */
  void reprTypeChanged(QAction* action);

  /**
  * called to change the coloring mode.
  */
  void colorMenuTriggered(QAction* action);

protected:
  /**
  * called to build the color arrays submenu.
  */
  virtual void buildColorFieldsMenu(pqPipelineRepresentation* pipelineRepr, QMenu* menu) const;

  /**
  * return the name of the block from its flat index
  */
  QString lookupBlockName(unsigned int flatIndex) const;

  mutable QPoint Position;
  mutable QPointer<pqDataRepresentation> PickedRepresentation;
  mutable QList<unsigned int> PickedBlocks;

private:
  Q_DISABLE_COPY(pqDefaultContextMenu)
};

#endif
