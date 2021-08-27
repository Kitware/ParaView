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

#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqView.h>

#include <vtkLogger.h>

#include <QPainter>

#include <limits>
#include <sstream>

#if NodeEditor_ENABLE_GRAPHVIZ
#include <cgraph.h>
#include <geom.h>
#include <gvc.h>
#endif // NodeEditor_ENABLE_GRAPHVIZ

// ----------------------------------------------------------------------------
pqNodeEditorScene::pqNodeEditorScene(QObject* parent)
  : QGraphicsScene(parent)
{
}

// ----------------------------------------------------------------------------
int pqNodeEditorScene::computeLayout(const std::unordered_map<int, pqNodeEditorNode*>& nodes,
  std::unordered_map<int, std::vector<pqNodeEditorEdge*>>& edges)
{
#if NodeEditor_ENABLE_GRAPHVIZ
  // compute dot string
  qreal maxHeight = 0.0;
  qreal maxY = 0;
  std::string dotString;
  {
    std::stringstream nodeString;
    std::stringstream edgeString;

    for (const auto& it : nodes)
    {
      pqProxy* proxy = it.second->getProxy();
      int proxyId = pqNodeEditorUtils::getID(proxy);
      if (dynamic_cast<pqView*>(proxy) != nullptr) // ignore view in pipeline layout
      {
        continue;
      }

      const QRectF& b = it.second->boundingRect();
      qreal width = b.width() / POINTS_PER_INCH; // convert from points to inches
      qreal height = b.height() / POINTS_PER_INCH;
      if (maxHeight < height)
      {
        maxHeight = height;
      }

      // Construct the string representing a node in the text-based graphviz representation of the
      // graph.
      // See https://www.graphviz.org/pdf/libguide.pdf for more detail
      std::string sInputPorts = "";
      std::string sOutputPorts = "";
      if (const auto proxyAsSource = dynamic_cast<pqPipelineSource*>(proxy))
      {
        for (int i = 0; i < proxyAsSource->getNumberOfOutputPorts(); i++)
        {
          sOutputPorts += "<o" + std::to_string(i) + ">|";
        }
      }

      if (const auto proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy))
      {
        for (int i = 0; i < proxyAsFilter->getNumberOfInputPorts(); i++)
        {
          sInputPorts += "<i" + std::to_string(i) + ">|";
        }
      }

      nodeString << proxyId << "["
                 << "shape=record,"
                 << "label=\"{{" + sInputPorts.substr(0, sInputPorts.size() - 1) + "}|{" +
          sOutputPorts.substr(0, sOutputPorts.size() - 1) + "}}\","
                 << "width=" << width << ","
                 << "height=" << height << ""
                 << "];\n";

      // Construct the string representing all edges in the graph
      // See https://www.graphviz.org/pdf/libguide.pdf for more detail
      for (pqNodeEditorEdge* edge : edges[proxyId])
      {
        edgeString << pqNodeEditorUtils::getID(edge->getProducer()->getProxy()) << ":<o"
                   << edge->getProducerOutputPortIdx() << "> -> "
                   << pqNodeEditorUtils::getID(edge->getConsumer()->getProxy()) << ":<i"
                   << edge->getConsumerInputPortIdx() << ">;\n";
      }
    }

    // describe the overall look of the graph. For example : rankdir=LR -> Left To Right layout
    // See https://www.graphviz.org/pdf/libguide.pdf for more detail
    dotString +=
      "digraph g {\nrankdir=LR;splines = line;graph[pad=\"0\", ranksep=\"1\", nodesep=\"1\"];\n" +
      nodeString.str() + edgeString.str() + "\n}";
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
        const auto& coord = ND_coord(n);
        const auto& w = ND_width(n);
        const auto& h = ND_height(n);

        auto& x = coords[i];
        auto& y = coords[i + 1];
        x = (coord.x - w * POINTS_PER_INCH / 2.0); // convert w/h in inches to points
        y = (-coord.y - h * POINTS_PER_INCH / 2.0);

        maxY = std::max(maxY, y);
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

      it.second->setPos(coords[i], coords[i + 1]);
    }
  }

  // compute initial x position for all views
  std::vector<std::pair<pqNodeEditorNode*, qreal>> viewXMap;
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
    it.first->setPos(x, maxY + maxHeight);
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
