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

#ifndef pqNodeEditorScene_h
#define pqNodeEditorScene_h

#include <QGraphicsScene>

#include <unordered_map>

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
  virtual ~pqNodeEditorScene() = default;

  /**
   * Given a list of nodes, return the bounding box englobing all of these nodes.
   * The bounding box is expressed in coordinates relative to the scene.
   */
  static QRect getBoundingRect(const std::unordered_map<int, pqNodeEditorNode*>& nodes);

public slots:
  /**
   * Compute an optimized layout for the nodes in the scene.
   * Return 1 if success, 0 else.
   *
   * If GraphViz has not been at the compilation this function will do nothing.
   */
  int computeLayout(const std::unordered_map<int, pqNodeEditorNode*>& nodes,
    std::unordered_map<int, std::vector<pqNodeEditorEdge*>>& edges);

protected:
  /**
   *  Draws a grid background.
   */
  void drawBackground(QPainter* painter, const QRectF& rect);
};

#endif // pqNodeEditorScene_h
