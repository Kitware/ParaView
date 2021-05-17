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

#pragma once

// qt includes
#include <QGraphicsScene>

// std includes
#include <unordered_map>

namespace NE
{
class Node;
class Edge;
}

namespace NE
{

/// This class extends QGraphicsScene to
/// * draw a grid background;
/// * monitor the creation/modification/destruction of proxies to automatically
///   modify the scene accordingly;
/// * manage the instances of nodes and edges;
class Scene : public QGraphicsScene
{
  Q_OBJECT

public:
  Scene(QObject* parent = nullptr);
  ~Scene();

  QRect getBoundingRect(std::unordered_map<int, NE::Node*>& nodes);

public slots:
  int computeLayout(std::unordered_map<int, NE::Node*>& nodes,
    std::unordered_map<int, std::vector<NE::Edge*> >& edges);

protected:
  /// Draws a grid background.
  void drawBackground(QPainter* painter, const QRectF& rect);
};
}
