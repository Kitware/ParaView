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

#ifndef pqNodeEditorNView_h
#define pqNodeEditorNView_h

#include "pqNodeEditorNode.h"

class pqView;

class pqNodeEditorNView : public pqNodeEditorNode
{
  Q_OBJECT

public:
  pqNodeEditorNView(QGraphicsScene* scene, pqView* source, QGraphicsItem* parent = nullptr);
  ~pqNodeEditorNView() override = default;

  NodeType getNodeType() const final { return NodeType::VIEW; }

protected:
  void setupPaintTools(QPen& pen, QBrush& brush) override;
};

#endif // pqNodeEditorNView_h
