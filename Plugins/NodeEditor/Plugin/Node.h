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

// QT includes
#include <QGraphicsItem>

// forward declarations
class pqProxy;
class pqProxyWidget;
class pqView;
class pqPipelineSource;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;

namespace NE
{
class Port;
}

namespace NE
{
class Node : public QObject, public QGraphicsItem
{
  Q_OBJECT
  Q_INTERFACES(QGraphicsItem)

public:
  Node(QGraphicsScene* scene, pqProxy* proxy, QGraphicsItem* parent = nullptr);

  /// Creates a node for a pqPipelineSource that consists of
  /// * an encapsulating rectangle
  /// * input and output ports
  /// * a widgetContainer for properties
  Node(QGraphicsScene* scene, pqPipelineSource* source, QGraphicsItem* parent = nullptr);

  /// TODO
  Node(QGraphicsScene* scene, pqView* view, QGraphicsItem* parent = nullptr);

  /// Destructor
  ~Node();

  /// Delete copy constructor.
  Node(const Node&) = delete;
  /// Delete copy constructor.
  Node& operator=(const Node&) = delete;

  /// Get corresponding pqProxy of the node.
  pqProxy* getProxy() { return this->proxy; }

  /// Get input ports of the node.
  std::vector<NE::Port*>& getInputPorts() { return this->iPorts; }

  /// Get output ports of the node.
  std::vector<NE::Port*>& getOutputPorts() { return this->oPorts; }

  /// Get widget container of the node.
  pqProxyWidget* getProxyProperties() { return this->proxyProperties; }

  /// Get widget container of the node.
  QGraphicsTextItem* getLabel() { return this->label; }

  /// Update the size of the node to fit its contents.
  int updateSize();

  int getVerbosity();
  int setVerbosity(int i);

  // sets the type of the node (0:normal, 1: selected filter, 2: selected view)
  int setOutlineStyle(int style);
  int getOutlineStyle() { return this->outlineStyle; };

  int setBackgroundStyle(int style);
  int getBackgroundStyle() { return this->backgroundStyle; };

  QRectF boundingRect() const override;

signals:
  void nodeResized();
  void nodeMoved();

protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  QGraphicsScene* scene;
  pqProxy* proxy;
  pqProxyWidget* proxyProperties;
  QWidget* widgetContainer;
  QGraphicsTextItem* label;

  std::vector<NE::Port*> iPorts;
  std::vector<NE::Port*> oPorts;

  int outlineStyle{ 0 };    // 0: normal, 1: selected filter, 2: selected view
  int backgroundStyle{ 0 }; // 0: normal, 1: modified
  int verbosity{ 0 };       // 0: empty, 1: non-advanced, 2: advanced

  int labelHeight{ 30 };

  int portHeight{ 24 };
  int portContainerHeight{ 0 };

  int widgetContainerHeight{ 0 };
  int widgetContainerWidth{ 0 };
};
}
