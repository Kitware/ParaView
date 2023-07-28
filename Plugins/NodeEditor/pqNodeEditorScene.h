// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqNodeEditorScene_h
#define pqNodeEditorScene_h

#include <QGraphicsScene>

#include "vtkType.h" // for vtkIdType

#include <unordered_map> // for std::unordered_map
#include <vector>        // for std::vector

class pqNodeEditorNode;
class pqNodeEditorEdge;

/**
 * This class extends QGraphicsScene to draw a grid background;
 * monitor the creation/modification/destruction of proxies to automatically
 * modify the scene accordingly; and manage the instances of nodes and edges.
 */
class pqNodeEditorScene : public QGraphicsScene
{
  Q_OBJECT

public:
  pqNodeEditorScene(QObject* parent = nullptr);
  ~pqNodeEditorScene() override = default;

Q_SIGNALS:
  /**
   * Fired when an en edge created from the drag and drop interaction of a port disc
   * is released. @c fromNode is the ID of the producer proxy and @c fromPort its port number.
   * @c toNode and @c toPort re the same informations but relative to the consumer.
   */
  void edgeDragAndDropRelease(vtkIdType fromNode, int fromPort, vtkIdType toNode, int toPort);

public Q_SLOTS:
  /**
   * Compute an optimized layout for the nodes in the scene.
   * Return 1 if success, 0 else.
   *
   * If GraphViz has not been at the compilation this function will do nothing.
   */
  int computeLayout(const std::unordered_map<vtkIdType, pqNodeEditorNode*>& nodes,
    std::unordered_map<vtkIdType, std::vector<pqNodeEditorEdge*>>& edges);

protected:
  /**
   * Snaps the given x and y coordinate to the next available top left gird point.
   * Optionally the grid can be scaled with the resolution parameter.
   */
  static QPointF snapToGrid(const qreal& x, const qreal& y, const qreal& resolution = 1.0);

  /**
   * Draws a grid background.
   */
  void drawBackground(QPainter* painter, const QRectF& rect) override;
};

#endif // pqNodeEditorScene_h
