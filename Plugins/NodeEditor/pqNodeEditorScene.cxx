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

#include "pqNodeEditorScene.h"

#include "pqNodeEditorEdge.h"
#include "pqNodeEditorNode.h"
#include "pqNodeEditorUtils.h"

#include <pqPipelineSource.h>
#include <pqView.h>

#include <vtkLogger.h>

#include <QPainter>

#include <limits>
#include <sstream>

#ifdef NodeEditor_ENABLE_GRAPHVIZ
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#endif // NodeEditor_ENABLE_GRAPHVIZ

// ----------------------------------------------------------------------------
pqNodeEditorScene::pqNodeEditorScene(QObject* parent)
  : QGraphicsScene(parent)
{
}

// ----------------------------------------------------------------------------
int pqNodeEditorScene::computeLayout(const std::unordered_map<int, pqNodeEditorNode*>& nodes,
  std::unordered_map<int, std::vector<pqNodeEditorEdge*> >& edges)
{
#ifdef NodeEditor_ENABLE_GRAPHVIZ
  // Scale factor between graphviz and Qt coordinates
  constexpr double GRAPHVIZ_SCALE_FACTOR = 100.0;

  // compute dot string
  qreal maxHeight = 0.0;
  qreal maxY = 0;
  qreal minY = 0;
  std::string dotString;
  {
    std::stringstream nodeString;
    std::stringstream edgeString;

    for (const auto& it : nodes)
    {
      pqProxy* proxy = it.second->getProxy();
      int proxyId = pqNodeEditorUtils::getID(proxy);
      if (dynamic_cast<pqView*>(proxy) != nullptr)
      {
        continue;
      }

      const QRectF& b = it.second->boundingRect();
      qreal width = b.width() / GRAPHVIZ_SCALE_FACTOR;
      qreal height = b.height() / GRAPHVIZ_SCALE_FACTOR;
      if (maxHeight < height)
      {
        maxHeight = height;
      }

      // Construct the string representing a node in the text-based graphviz representation of the
      // graph.
      // See https://www.graphviz.org/pdf/libguide.pdf for more detail
      nodeString << proxyId << "["
                 << "label=\"\","
                 << "shape=box,"
                 << "width=" << width << ","
                 << "height=" << height << ""
                 << "];\n";

      // Construct the string representing all edges in the graph
      // See https://www.graphviz.org/pdf/libguide.pdf for more detail
      for (pqNodeEditorEdge* edge : edges[proxyId])
      {
        edgeString << pqNodeEditorUtils::getID(edge->getProducer()->getProxy()) << " -> "
                   << pqNodeEditorUtils::getID(edge->getConsumer()->getProxy()) << ";\n";
      }
    }

    // describe the overall look of the graph. For example : rankdir=LR -> Left To Right layout
    // See https://www.graphviz.org/pdf/libguide.pdf for more detail
    dotString += "digraph g {\nrankdir=LR;graph[pad=\"0\", ranksep=\"2\", nodesep=\"" +
      std::to_string(maxHeight) + "\"];\n" + nodeString.str() + edgeString.str() + "\n}";
  }

  std::vector<qreal> coords(2 * nodes.size(), 0.0);
  // compute layout
  {
    Agraph_t* G = agmemread(dotString.data());
    GVC_t* gvc = gvContext();
    if (!G || !gvc || gvLayout(gvc, G, "dot"))
    {
      vtkLogF(ERROR, "[NodeEditorPlugin] Cannot intialize Graphviz context.");
      return 0;
    }

    // read layout
    int i = -2;
    for (const auto& it : nodes)
    {
      i += 2;

      pqProxy* proxy = it.second->getProxy();
      if (dynamic_cast<pqView*>(proxy) != nullptr)
      {
        continue;
      }

      Agnode_t* n =
        agnode(G, const_cast<char*>(std::to_string(pqNodeEditorUtils::getID(proxy)).data()), 0);
      if (n != nullptr)
      {
        auto& coord = ND_coord(n);

        coords[i] = coord.x;
        coords[i + 1] = coord.y;

        if (minY > coord.y)
        {
          minY = coord.y;
        }

        if (maxY < coord.y)
        {
          maxY = coord.y;
        }
      }
    }

    // free memory
    int status = gvFreeLayout(gvc, G);
    status += agclose(G);
    status += gvFreeContext(gvc);
    if (status)
    {
      vtkLogF(WARNING, "Error when freeing Graphviz ressources.");
    }
  }

  // set positions
  {
    int i = -2;
    for (const auto& it : nodes)
    {
      i += 2;

      if (dynamic_cast<pqView*>(it.second->getProxy()) != nullptr)
      {
        continue;
      }

      it.second->setPos(coords[i], coords[i + 1] - minY);
    }
  }

  // compute initial x position for all views
  std::vector<std::pair<pqNodeEditorNode*, qreal> > viewXMap;
  for (const auto& it : nodes)
  {
    auto* proxyAsView = dynamic_cast<pqView*>(it.second->getProxy());
    if (!proxyAsView)
    {
      continue;
    }

    qreal avgX = 0;
    auto edgesIt = edges.find(pqNodeEditorUtils::getID(proxyAsView));
    if (edgesIt != edges.end())
    {
      int nEdges = edgesIt->second.size();
      if (nEdges > 0)
      {
        for (pqNodeEditorEdge* edge : edgesIt->second)
        {
          avgX += edge->getProducer()->pos().x();
        }
        avgX /= nEdges;
      }
    }

    viewXMap.emplace_back(it.second, avgX);
  }

  // sort views by current x coord
  std::sort(viewXMap.begin(), viewXMap.end(),
    [](const std::pair<pqNodeEditorNode*, qreal>& a, const std::pair<pqNodeEditorNode*, qreal>& b) {
      return a.second < b.second;
    });

  // make sure all views have enough space
  qreal lastX = 0.0;
  for (const auto& it : viewXMap)
  {
    const qreal width = it.first->boundingRect().width();
    qreal x = it.second;
    if (lastX + width > x)
    {
      x = lastX + width + 10.0;
    }
    it.first->setPos(x, maxY + maxHeight * GRAPHVIZ_SCALE_FACTOR + 10.0);
    lastX = x;
  }

  return 1;
#else  // NodeEditor_ENABLE_GRAPHVIZ
  (void)nodes;
  (void)edges;
  return 0;
#endif // NodeEditor_ENABLE_GRAPHVIZ
}

// ----------------------------------------------------------------------------
QRect pqNodeEditorScene::getBoundingRect(const std::unordered_map<int, pqNodeEditorNode*>& nodes)
{
  int x0 = std::numeric_limits<int>::max();
  int x1 = std::numeric_limits<int>::min();
  int y0 = std::numeric_limits<int>::max();
  int y1 = std::numeric_limits<int>::min();

  for (const auto& it : nodes)
  {
    QPointF p = it.second->pos();
    QRectF b = it.second->boundingRect();
    if (x0 > p.x() + b.left())
    {
      x0 = p.x() + b.left();
    }
    if (x1 < p.x() + b.right())
    {
      x1 = p.x() + b.right();
    }
    if (y0 > p.y() + b.top())
    {
      y0 = p.y() + b.top();
    }
    if (y1 < p.y() + b.bottom())
    {
      y1 = p.y() + b.bottom();
    }
  }

  return QRect(x0, y0, x1 - x0, y1 - y0);
}

// ----------------------------------------------------------------------------
void pqNodeEditorScene::drawBackground(QPainter* painter, const QRectF& rect)
{
  constexpr int GRID_SIZE = 25;

  qreal left = int(rect.left()) - (int(rect.left()) % GRID_SIZE);
  qreal top = int(rect.top()) - (int(rect.top()) % GRID_SIZE);

  QVarLengthArray<QLineF, 100> lines;

  for (qreal x = left; x < rect.right(); x += GRID_SIZE)
  {
    lines.append(QLineF(x, rect.top(), x, rect.bottom()));
  }
  for (qreal y = top; y < rect.bottom(); y += GRID_SIZE)
  {
    lines.append(QLineF(rect.left(), y, rect.right(), y));
  }

  painter->setRenderHints(QPainter::Antialiasing);
  painter->setPen(QColor(60, 60, 60));
  painter->drawLines(lines.data(), lines.size());
}
