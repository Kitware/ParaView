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

#ifndef pqNodeEditorEdge_h
#define pqNodeEditorEdge_h

#include <QGraphicsPathItem>
#include <QGraphicsScene>

// forward declarations
class pqNodeEditorNode;

/**
 * Every instance of this class corresponds to an edge between an output port
 * and an input port. This class internally detects if the positions of the
 * corresponding ports change and updates itself automatically.
 */
class pqNodeEditorEdge : public QObject, public QGraphicsPathItem
{
  Q_OBJECT

public:
  pqNodeEditorEdge(QGraphicsScene* scene, pqNodeEditorNode* producer, int producerOutputPortIdx,
    pqNodeEditorNode* consumer, int consumerInputPortIdx, int type = 0,
    QGraphicsItem* parent = nullptr);
  ~pqNodeEditorEdge();

  //@{
  /*
   * Get/Set the state of the edge (0:normal edge, 1: view edge)
   */
  int setType(int type);
  int getType() { return this->type; };
  //@}

  //@{
  /*
   * Get the producer/consumer of this edge
   */
  pqNodeEditorNode* getProducer() { return this->producer; };
  pqNodeEditorNode* getConsumer() { return this->consumer; };
  //@}

  /*
   * Print edge information.
   */
  std::string toString();

public slots:
  int updatePoints();

protected:
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
  QGraphicsScene* scene;

  int type{ 0 };
  QPointF oPoint;
  QPointF cPoint;
  QPointF iPoint;

  pqNodeEditorNode* producer;
  int producerOutputPortIdx;
  pqNodeEditorNode* consumer;
  int consumerInputPortIdx;
};

#endif // pqNodeEditorEdge_h
