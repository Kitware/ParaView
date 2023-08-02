// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

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
