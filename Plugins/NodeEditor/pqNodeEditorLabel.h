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

#ifndef pqNodeEditorLabel_h
#define pqNodeEditorLabel_h

#include <QGraphicsTextItem>

#include <functional>

class QGraphicsSceneMouseEvent;
class QString;

/**
 * Extend the QGraphicsTextItem so the behavior on mouse events
 * is freely configurable.
 */
class pqNodeEditorLabel : public QGraphicsTextItem
{
public:
  pqNodeEditorLabel(QString label, QGraphicsItem* parent, bool cursor = true);
  ~pqNodeEditorLabel() override = default;

  /**
   * Set the function to call when a mouse button is pressed on the text.
   * It will usually invalidate the event and pass it to its parent. This
   * behavior is needed for supporting drag and drop of the parent item
   * with the mouse being on the label.
   */
  void setMousePressEventCallback(const std::function<void(QGraphicsSceneMouseEvent*)>& callback);

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
  std::function<void(QGraphicsSceneMouseEvent*)> mousePressCallback = {};
};

#endif // pqNodeEditorLabel_h
