/*=========================================================================

  Program:   ParaView
  Plugin:    NodeEditor

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  ParaViewPluginsNodeEditor - BSD 3-Clause License - Copyright (C) 2021 Jonas Lukasczyk

  See the Copyright.txt file provided
  with ParaViewPluginsNodeEditor for license information.
-------------------------------------------------------------------------*/

#ifndef pqNodeEditorPort_h
#define pqNodeEditorPort_h

#include <QGraphicsItem>

class QGraphicsEllipseItem;
class QGraphicsTextItem;

/**
 * Every instance of this class corresponds to a port belonging to a pqNodeEditorNode.
 * It is displayed as a disc and a label at the left or the right
 * this disc, according to if the port is either an input or output node.
 * This class is not aware of the node it belongs to and thus does not handle its position.
 */
class pqNodeEditorPort : public QGraphicsItem
{

public:
  enum class Type : int
  {
    INPUT = 0,
    OUTPUT
  };

  /**
   * Create a node port of a specific type with a specific name. The one instanciating the port
   * should take care of where to place it on the scene.
   */
  pqNodeEditorPort(Type type, QString name = "", QGraphicsItem* parent = nullptr);
  virtual ~pqNodeEditorPort() = default;

  QGraphicsEllipseItem* getDisc() { return this->disc; }
  QGraphicsTextItem* getLabel() { return this->label; }

  /**
   * Determines if the port is marked as selected.
   */
  int setMarkedAsSelected(bool selected);

  /**
   * Determines if the port is marked as visible.
   */
  int setMarkedAsVisible(bool visible);

protected:
  QRectF boundingRect() const override;

  /**
   * Remove any painting from the paint function, and let the owner of the port do the display.
   */
  void paint(QPainter* /*painter*/, const QStyleOptionGraphicsItem* /*option*/,
    QWidget* /*widget*/) override{};

private:
  QGraphicsEllipseItem* disc;
  QGraphicsTextItem* label;
};

#endif // pqNodeEditorPort_h
