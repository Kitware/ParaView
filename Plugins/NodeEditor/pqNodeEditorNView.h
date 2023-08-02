// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqNodeEditorNView_h
#define pqNodeEditorNView_h

#include "pqNodeEditorNode.h"

class pqView;

class pqNodeEditorNView : public pqNodeEditorNode
{
  Q_OBJECT

public:
  pqNodeEditorNView(pqView* source, QGraphicsItem* parent = nullptr);
  ~pqNodeEditorNView() override = default;

  NodeType getNodeType() const final { return NodeType::VIEW; }

  void setNodeActive(bool active) override;

protected:
  void setupPaintTools(QPen& pen, QBrush& brush) override;
};

#endif // pqNodeEditorNView_h
