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

#include <View.h>

#include <pqDeleteReaction.h>

// qt includes
#include <QAction>
#include <QKeyEvent>
#include <QWheelEvent>

NE::View::View(QWidget* parent)
  : QGraphicsView(parent){};

NE::View::View(QGraphicsScene* scene, QWidget* parent)
  : QGraphicsView(scene, parent)
  , deleteAction(new QAction(this))
{
  // create delete reaction
  new pqDeleteReaction(this->deleteAction);

  this->setRenderHint(QPainter::Antialiasing);
}

NE::View::~View() = default;

void NE::View::wheelEvent(QWheelEvent* event)
{
  const ViewportAnchor anchor = this->transformationAnchor();
  this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  int angle = event->angleDelta().y();
  double factor = (angle > 0) ? 1.1 : 0.9;
  this->scale(factor, factor);
  this->setTransformationAnchor(anchor);
}

void NE::View::keyReleaseEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Delete)
  {
    this->deleteAction->trigger();
  }

  return QWidget::keyReleaseEvent(event);
}
