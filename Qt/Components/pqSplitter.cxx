/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqSplitter.h"

/**
 * @class pqSplitterHandle
 * @brief A custom splitter handle derived from QSplitterHandle.
 *
 * pqSplitterHandle paints a separator optionally. pqSplitter controls
 * whether the separator should be painted or not, as well as the width and
 * color of the separator through the private members of this class.
 */
class pqSplitter::pqSplitterHandle : public QSplitterHandle
{
  friend class pqSplitter;

public:
  pqSplitterHandle(Qt::Orientation orientation, QSplitter* parent, const QColor& color)
    : QSplitterHandle(orientation, parent)
    , Color(color)
  {
  }
  ~pqSplitterHandle() {}
protected:
  // Paint a rectangular separator only when Paint is set to true and Color is valid.
  void paintEvent(QPaintEvent*) override
  {
    if (Paint && Color.isValid())
    {
      QPainter painter(this);
      QPen pen;
      pen.setColor(Color);
      QRect rec;
      painter.setPen(pen);
      if (orientation() == Qt::Horizontal)
      {
        rec = QRect(rect().left(), 0, Width, rect().height());
      }
      else
      {
        rec = QRect(0, rect().top(), rect().width(), Width);
      }
      painter.fillRect(rec, Color);
    }
  }

private:
  bool Paint = false;
  int Width = 1;
  QColor Color = QColor::Invalid;
};

void pqSplitter::setSplitterHandlePaint(bool paint)
{
  auto h = this->handle(1);
  if (h)
  {
    h->Paint = paint;
    h->update();
  }
}

void pqSplitter::setSplitterHandleWidth(int width)
{
  auto h = this->handle(1);
  if (h)
  {
    h->Width = width;
    h->update();
  }
}

void pqSplitter::setSplitterHandleColor(double color[3])
{
  auto h = this->handle(1);
  if (h)
  {
    h->Color = QColor::fromRgbF(color[0], color[1], color[2]);
    h->update();
  }
}

QSplitterHandle* pqSplitter::createHandle()
{
  return new pqSplitterHandle(
    this->orientation(), this, this->palette().color(QPalette::ColorRole::Window));
}

pqSplitter::pqSplitterHandle* pqSplitter::handle(int index) const
{
  return dynamic_cast<pqSplitterHandle*>(this->QSplitter::handle(index));
}
