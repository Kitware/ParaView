/*=========================================================================
Copyright (c) 2021, Jonas Lukasczyk
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include <Scene.h>

// node editor includes
#include <Edge.h>
#include <Node.h>
#include <Utils.h>

// qt includes
#include <QPainter>

// paraview/vtk includes
#include <pqPipelineSource.h>
#include <pqView.h>

// std includes
#include <limits>
#include <sstream>

NE::Scene::Scene(QObject* parent)
  : QGraphicsScene(parent)
{
}

NE::Scene::~Scene()
{
}

#if NE_ENABLE_GRAPHVIZ
#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#endif

int NE::Scene::computeLayout(std::unordered_map<int, NE::Node*>& nodes,
  std::unordered_map<int, std::vector<NE::Edge*> >& edges)
{
  NE::log("Computing Graph Layout");

#if NE_ENABLE_GRAPHVIZ

  // compute dot string
  qreal maxHeight = 0.0;
  qreal maxY = 0;
  qreal minY = 0;
  std::string dotString;
  {
    std::stringstream nodeString;
    std::stringstream edgeString;

    for (auto it : nodes)
    {
      auto proxyAsView = dynamic_cast<pqView*>(it.second->getProxy());
      if (proxyAsView)
        continue;

      const auto& b = it.second->boundingRect();
      qreal width = b.width() / 100.0;
      qreal height = b.height() / 100.0;
      if (maxHeight < height)
        maxHeight = height;

      nodeString << NE::getID(it.second->getProxy()) << "["
                 << "label=\"\","
                 << "shape=box,"
                 << "width=" << width << ","
                 << "height=" << height << ""
                 << "];\n";

      for (auto edge : edges[NE::getID(it.second->getProxy())])
      {
        edgeString << NE::getID(edge->getProducer()->getProxy()) << " -> "
                   << NE::getID(edge->getConsumer()->getProxy()) << ";\n";
      }
    }

    dotString += std::string("") + "digraph g {\n" +
      "rankdir=LR;graph[pad=\"0\", ranksep=\"2\", nodesep=\"" + std::to_string(maxHeight) +
      "\"];\n" + nodeString.str() + edgeString.str() + "\n}";
    // NE::log(dotString);
  }

  std::vector<qreal> coords(2 * nodes.size(), 0.0);
  // compute layout
  {
    Agraph_t* G = agmemread(dotString.data());
    GVC_t* gvc = gvContext();
    gvLayout(gvc, G, "dot");

    // read layout
    int i = -2;
    for (auto it : nodes)
    {
      i += 2;

      auto proxy = it.second->getProxy();

      auto proxyAsView = dynamic_cast<pqView*>(proxy);
      if (proxyAsView)
        continue;

      Agnode_t* n = agnode(G, const_cast<char*>(std::to_string(NE::getID(proxy)).data()), 0);
      if (n != nullptr)
      {
        auto& coord = ND_coord(n);

        coords[i] = coord.x;
        coords[i + 1] = coord.y;

        if (minY > coord.y)
          minY = coord.y;

        if (maxY < coord.y)
          maxY = coord.y;
      }
    }

    // free memory
    gvFreeLayout(gvc, G);
    agclose(G);
    gvFreeContext(gvc);
  }

  // set positions
  {
    int i = -2;
    for (auto it : nodes)
    {
      i += 2;

      auto proxyAsView = dynamic_cast<pqView*>(it.second->getProxy());
      if (proxyAsView)
        continue;

      it.second->setPos(coords[i], coords[i + 1] - minY);
    }
  }

  // compute initial x position for all views
  std::vector<std::pair<Node*, qreal> > viewXMap;
  for (auto it : nodes)
  {
    auto proxyAsView = dynamic_cast<pqView*>(it.second->getProxy());
    if (!proxyAsView)
      continue;

    qreal avgX = 0;
    auto edgesIt = edges.find(NE::getID(proxyAsView));
    if (edgesIt != edges.end())
    {
      int nEdges = edgesIt->second.size();
      if (nEdges > 0)
      {
        for (auto edge : edgesIt->second)
          avgX += edge->getProducer()->pos().x();
        avgX /= nEdges;
      }
    }

    viewXMap.emplace_back(it.second, avgX);
  }

  // sort views by current x coord
  std::sort(viewXMap.begin(), viewXMap.end(),
    [](const std::pair<Node*, qreal>& a, const std::pair<Node*, qreal>& b) {
      return a.second < b.second;
    });

  // make sure all views have enough space
  qreal lastX = 0.0;
  for (auto it : viewXMap)
  {
    const qreal width = it.first->boundingRect().width();
    qreal x = it.second;
    if (lastX + width > x)
      x = lastX + width + 10.0;
    it.first->setPos(x, maxY + maxHeight * 100.0 + 10.0);
    lastX = x;
  }

  return 1;
#else
  NE::log("ERROR: GraphViz support disabled!", true);
  return 0;
#endif
}

QRect NE::Scene::getBoundingRect(std::unordered_map<int, NE::Node*>& nodes)
{
  int x0 = std::numeric_limits<int>::max();
  int x1 = std::numeric_limits<int>::min();
  int y0 = std::numeric_limits<int>::max();
  int y1 = std::numeric_limits<int>::min();

  for (auto it : nodes)
  {
    auto p = it.second->pos();
    auto b = it.second->boundingRect();
    if (x0 > p.x() + b.left())
      x0 = p.x() + b.left();
    if (x1 < p.x() + b.right())
      x1 = p.x() + b.right();
    if (y0 > p.y() + b.top())
      y0 = p.y() + b.top();
    if (y1 < p.y() + b.bottom())
      y1 = p.y() + b.bottom();
  }

  return QRect(x0, y0, x1 - x0, y1 - y0);
}

void NE::Scene::drawBackground(QPainter* painter, const QRectF& rect)
{
  const int gridSize = 25;

  qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
  qreal top = int(rect.top()) - (int(rect.top()) % gridSize);

  QVarLengthArray<QLineF, 100> lines;

  for (qreal x = left; x < rect.right(); x += gridSize)
    lines.append(QLineF(x, rect.top(), x, rect.bottom()));
  for (qreal y = top; y < rect.bottom(); y += gridSize)
    lines.append(QLineF(rect.left(), y, rect.right(), y));

  painter->setRenderHints(QPainter::HighQualityAntialiasing);
  painter->setPen(QColor(60, 60, 60));
  painter->drawLines(lines.data(), lines.size());
}
