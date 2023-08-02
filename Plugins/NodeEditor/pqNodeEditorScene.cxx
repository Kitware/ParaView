// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (C) 2021 Jonas Lukasczyk
// SPDX-License-Identifier: BSD-3-Clause

#include "pqNodeEditorScene.h"

#include "pqNodeEditorEdge.h"
#include "pqNodeEditorNode.h"
#include "pqNodeEditorUtils.h"

#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqView.h>

#include <vtkLogger.h>

#include <QPainter>

#include <cmath>
#include <sstream>

#if NodeEditor_ENABLE_GRAPHVIZ
// Older Graphviz releases (before 2.40.0) require `HAVE_CONFIG_H` to define
// `POINTS_PER_INCH`.
#define HAVE_CONFIG_H
#include <cgraph.h>
#include <geom.h>
#include <gvc.h>
#undef HAVE_CONFIG_H
#endif // NodeEditor_ENABLE_GRAPHVIZ

// ----------------------------------------------------------------------------
pqNodeEditorScene::pqNodeEditorScene(QObject* parent)
  : QGraphicsScene(parent)
{
}

// ----------------------------------------------------------------------------
QPointF pqNodeEditorScene::snapToGrid(const qreal& x, const qreal& y, const qreal& resolution)
{
  const auto gridSize = pqNodeEditorUtils::CONSTS::GRID_SIZE * resolution;
  return QPointF(x - std::fmod(x, gridSize), y - std::fmod(y, gridSize));
}

// ----------------------------------------------------------------------------
int pqNodeEditorScene::computeLayout(const std::unordered_map<vtkIdType, pqNodeEditorNode*>& nodes,
  std::unordered_map<vtkIdType, std::vector<pqNodeEditorEdge*>>& edges)
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
      const vtkIdType proxyId = pqNodeEditorUtils::getID(proxy);
      // ignore view and hidden nodes in pipeline layout
      if (!it.second->isVisible() || it.second->getNodeType() == pqNodeEditorNode::NodeType::VIEW)
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
      for (size_t i = 0; i < it.second->getOutputPorts().size(); i++)
      {
        sOutputPorts += "<o" + std::to_string(i) + ">|";
      }

      for (size_t i = 0; i < it.second->getInputPorts().size(); i++)
      {
        sInputPorts += "<i" + std::to_string(i) + ">|";
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
    // this version is bottom up (default) and try to reduce the overlap
    // See https://www.graphviz.org/pdf/libguide.pdf for more detail
    dotString += "digraph g {\noverlap = true;splines = true;graph[pad=\"0.5\", ranksep=\"0.5\", "
                 "nodesep=\"0.5\"];\n" +
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

      if (it.second->getNodeType() == pqNodeEditorNode::NodeType::VIEW)
      {
        continue;
      }

      Agnode_t* n = agnode(G,
        const_cast<char*>(std::to_string(pqNodeEditorUtils::getID(it.second->getProxy())).data()),
        0);
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
      vtkLogF(WARNING, "[NodeEditorPlugin] Error when freeing Graphviz resources.");
    }
  }

  // set positions
  {
    int i = -2;
    for (const auto& it : nodes)
    {
      i += 2;

      if (it.second->getNodeType() == pqNodeEditorNode::NodeType::VIEW)
      {
        continue;
      }

      it.second->setPos(pqNodeEditorScene::snapToGrid(coords[i], coords[i + 1]));
    }
  }

  // compute initial x position for all views
  std::vector<std::pair<pqNodeEditorNode*, qreal>> viewXMap;
  for (const auto& it : nodes)
  {
    if (it.second->getNodeType() != pqNodeEditorNode::NodeType::VIEW)
    {
      continue;
    }

    qreal avgX = 0;
    auto edgesIt = edges.find(pqNodeEditorUtils::getID(it.second->getProxy()));
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
  qreal lastX = VTK_DOUBLE_MIN;
  for (const auto& it : viewXMap)
  {
    const qreal width = it.first->boundingRect().width();
    qreal x = it.second;
    if (lastX + width > x)
    {
      x = lastX + width + 10.0;
    }
    it.first->setPos(pqNodeEditorScene::snapToGrid(
      x, maxY + maxHeight + 2.0 * pqNodeEditorUtils::CONSTS::GRID_SIZE));
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
void pqNodeEditorScene::drawBackground(QPainter* painter, const QRectF& rect)
{
  painter->setPen(pqNodeEditorUtils::CONSTS::COLOR_GRID);

  // get rectangle bounds
  const qreal recL = rect.left();
  const qreal recR = rect.right();
  const qreal recT = rect.top();
  const qreal recB = rect.bottom();

  // determine whether to use low or high resoltion grid
  const qreal gridResolution = (recB - recT) > 2000 ? 4 : 1;
  const qreal gridSize = gridResolution * pqNodeEditorUtils::CONSTS::GRID_SIZE;

  // find top left corner of active rectangle and snap to grid
  const QPointF& snappedTopLeft = pqNodeEditorScene::snapToGrid(recL, recT, gridResolution);

  // iterate over the x range of the rectangle to draw vertical lines
  for (qreal x = snappedTopLeft.x(); x < recR; x += gridSize)
  {
    painter->drawLine(x, recT, x, recB);
  }

  // iterate over the y range of the rectangle to draw horizontal lines
  for (qreal y = snappedTopLeft.y(); y < recB; y += gridSize)
  {
    painter->drawLine(recL, y, recR, y);
  }
}
