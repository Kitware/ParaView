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
