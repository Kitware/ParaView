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

#ifndef pqNodeEditorEdge_h
#define pqNodeEditorEdge_h

#include <QGraphicsPathItem>

class pqNodeEditorNode;
class QGraphicsScene;

/**
 * Every instance of this class corresponds to an edge between an output port
 * and an input port. This class internally detects if the positions of the
 * corresponding ports change and updates itself automatically.
 */
class pqNodeEditorEdge : public QObject, public QGraphicsPathItem
{
  Q_OBJECT

public:
  /**
   * Edge style enumeration. NORMAL is for an edge between 2 sources and VIEW is for an edge between
   * a source and a view node.
   */
  enum class EdgeType : int
  {
    NORMAL = 0,
    VIEW
  };

  /**
   * Create an edge from the @c producer node to the @c consumer, linking their specified ports.
   * The edge is created and added into the specified @c scene.
   * One can also set its type and parent.
   */
  pqNodeEditorEdge(QGraphicsScene* scene, pqNodeEditorNode* producer, int producerOutputPortIdx,
    pqNodeEditorNode* consumer, int consumerInputPortIdx, EdgeType type = EdgeType::NORMAL,
    QGraphicsItem* parent = nullptr);

  /**
   * Remove the edge from the scene it has been added to.
   */
  virtual ~pqNodeEditorEdge();

  //@{
  /*
   * Get/Set the type of the edge (0:normal edge, 1: view edge). Update the style accordingly.
   */
  void setType(EdgeType type);
  int getType() { return static_cast<int>(this->type); };
  //@}

  //@{
  /*
   * Get the producer/consumer of this edge
   */
  pqNodeEditorNode* getProducer() { return this->producer; };
  pqNodeEditorNode* getConsumer() { return this->consumer; };
  //@}

  /*
   * Get edge information as string.
   */
  std::string toString();

public slots:
  /**
   * Recompute the points where the edge should pass by.
   * Should be called whenever one of the port the edge is attached to move.
   */
  int updatePoints();

protected:
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  QGraphicsScene* scene;

  EdgeType type{ EdgeType::NORMAL };
  QPointF oPoint;
  QPointF cPoint;
  QPointF iPoint;

  pqNodeEditorNode* producer;
  int producerOutputPortIdx;
  pqNodeEditorNode* consumer;
  int consumerInputPortIdx;
};

#endif // pqNodeEditorEdge_h
