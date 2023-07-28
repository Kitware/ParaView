// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqNodeEditorNSource_h
#define pqNodeEditorNSource_h

#include "pqNodeEditorNode.h"

class pqPipelineSource;
class pqOutputPort;

class pqNodeEditorNSource : public pqNodeEditorNode
{
  Q_OBJECT

public:
  pqNodeEditorNSource(pqPipelineSource* source, QGraphicsItem* parent = nullptr);
  ~pqNodeEditorNSource() override = default;

  NodeType getNodeType() const final { return NodeType::SOURCE; }

Q_SIGNALS:
  void inputPortClicked(int port, bool clear);
  void outputPortClicked(pqOutputPort* port, bool exclusive);

protected:
  void setupPaintTools(QPen& pen, QBrush& brush) override;
};

#endif // pqNodeEditorNSource_h
