// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqNodeEditorNRepresentation_h
#define pqNodeEditorNRepresentation_h

#include "pqNodeEditorNode.h"

class pqRepresentation;

class pqNodeEditorNRepresentation : public pqNodeEditorNode
{
  Q_OBJECT

public:
  pqNodeEditorNRepresentation(pqRepresentation* repr, QGraphicsItem* parent = nullptr);
  ~pqNodeEditorNRepresentation() override = default;

  NodeType getNodeType() const final { return NodeType::REPRESENTATION; }

  void setNodeActive(bool active) override;

  QRectF boundingRect() const override;

protected:
  void setupPaintTools(QPen& pen, QBrush& brush) override;
};

#endif // pqNodeEditorNRepresentation_h
