// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqNodeEditorEdge_h
#define pqNodeEditorEdge_h

#include "pqNodeEditorUtils.h"
#include <QGraphicsItem>

class pqNodeEditorNode;
class QGraphicsPathItem;

/**
 * Every instance of this class corresponds to an edge between an output port
 * and an input port. This class internally detects if the positions of the
 * corresponding ports change and updates itself automatically.
 *
 * This class also display 2 edges : one at the given Z depth, the other at the
 * maximum Z layer. The second one will be displayed slighlty transparent. This
 * is in order to have some informations on the edges even when nodes are above.
 * The overlay needs to be added manually to the scene.
 */
class pqNodeEditorEdge
  : public QObject
  , public QGraphicsItem
{
  Q_OBJECT
  Q_INTERFACES(QGraphicsItem)

public:
  /**
   * Edge style enumeration: PIPELINE is for an edge between 2 sources and VIEW is for an edge
   * between a source and a view node.
   */
  enum class Type : int
  {
    PIPELINE = 0,
    VIEW
  };

  /**
   * Create an edge from the @c producer node to the @c consumer, linking their specified ports.
   * The edge is created and added into the specified @c scene.
   * One can also set its type and parent.
   */
  pqNodeEditorEdge(pqNodeEditorNode* producer, int producerOutputPortIdx,
    pqNodeEditorNode* consumer, int consumerInputPortIdx, Type type = Type::PIPELINE,
    QGraphicsItem* parent = nullptr);

  /**
   * Remove the edge from the scene it has been added to.
   */
  ~pqNodeEditorEdge() override;

  ///@{
  /*
   * Get/Set the type of the edge (0:normal edge, 1: view edge). Update the style accordingly.
   */
  void setType(Type type);
  Type getType() { return this->type; };
  ///@}

  ///@{
  /*
   * Get the producer/consumer of this edge
   */
  pqNodeEditorNode* getProducer() { return this->producer; };
  pqNodeEditorNode* getConsumer() { return this->consumer; };
  ///@}

  ///@{
  /*
   * Get the producer/consumer port idx
   */
  int getProducerOutputPortIdx() { return this->producerOutputPortIdx; };
  int getConsumerInputPortIdx() { return this->consumerInputPortIdx; };
  ///@}

  QGraphicsPathItem* overlay() const { return this->edgeOverlay; }

  /*
   * Get edge information as string.
   */
  std::string toString();

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Recompute the points where the edge should pass by.
   * Should be called whenever one of the port the edge is attached to move.
   */
  int updatePoints();

protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  Type type = Type::PIPELINE;

  QPainterPath path;
  QGraphicsPathItem* edgeOverlay = nullptr;

  pqNodeEditorNode* producer = nullptr;
  int producerOutputPortIdx = 0;
  pqNodeEditorNode* consumer = nullptr;
  int consumerInputPortIdx = 0;
};

#endif // pqNodeEditorEdge_h
