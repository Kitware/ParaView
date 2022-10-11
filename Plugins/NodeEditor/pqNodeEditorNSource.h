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

#ifndef pqNodeEditorNSource_h
#define pqNodeEditorNSource_h

#include "pqNodeEditorNode.h"

class pqPipelineSource;

class pqNodeEditorNSource : public pqNodeEditorNode
{
  Q_OBJECT

public:
  pqNodeEditorNSource(
    QGraphicsScene* scene, pqPipelineSource* source, QGraphicsItem* parent = nullptr);
  ~pqNodeEditorNSource() override = default;

  NodeType getNodeType() const final { return NodeType::SOURCE; }

protected:
  void setupPaintTools(QPen& pen, QBrush& brush) override;
};

#endif // pqNodeEditorNSource_h
