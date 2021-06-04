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

#include "pqNodeEditorPort.h"

#include "pqNodeEditorUtils.h"

// qt includes
#include <QApplication>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPalette>
#include <QPen>

#include <iostream>

// ----------------------------------------------------------------------------
namespace details
{
class PortLabel : public QGraphicsTextItem
{
public:
  PortLabel(QString label, QGraphicsItem* parent)
    : QGraphicsTextItem(label, parent)
  {
    this->setCursor(Qt::PointingHandCursor);
  };
  ~PortLabel() = default;
  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override
  {
    auto font = this->font();
    font.setBold(true);
    this->setFont(font);
  };
  void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override
  {
    auto font = this->font();
    font.setBold(false);
    this->setFont(font);
  };
};
}

// ----------------------------------------------------------------------------
pqNodeEditorPort::pqNodeEditorPort(int type, QString name, QGraphicsItem* parent)
  : QGraphicsItem(parent)
{
  this->label = new details::PortLabel(name, this);
  this->label->setPos(
    type == 0 ? this->portRadius + 3 : -this->portRadius - 3 - this->label->boundingRect().width(),
    -0.5 * this->label->boundingRect().height());

  this->disc = new QGraphicsEllipseItem(
    -this->portRadius, -this->portRadius, 2 * this->portRadius, 2 * this->portRadius, this);
  this->disc->setBrush(QApplication::palette().dark());
  this->setStyle(0);
}

// ----------------------------------------------------------------------------
pqNodeEditorPort::~pqNodeEditorPort() = default;

// ----------------------------------------------------------------------------
int pqNodeEditorPort::setStyle(int style)
{
  this->disc->setPen(
    QPen(style == 1 ? QApplication::palette().highlight() : QApplication::palette().light(),
      this->borderWidth));
  return 1;
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorPort::boundingRect() const
{
  return QRectF(0, 0, 0, 0);
}

// ----------------------------------------------------------------------------
void pqNodeEditorPort::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
}
