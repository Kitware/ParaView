/*=========================================================================

   Program: ParaView
   Module:    pqPointMarker.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

/// \file pqPointMarker.cxx
/// \date 9/18/2006

#include "pqPointMarker.h"

#include <QPainter>
#include <QPolygonF>
#include <QSize>
#include <QRectF>


class pqCrossPointMarkerInternal
{
public:
  pqCrossPointMarkerInternal(const QRectF &rect);
  ~pqCrossPointMarkerInternal() {}

  QRectF Rect;
};


class pqPlusPointMarkerInternal
{
public:
  pqPlusPointMarkerInternal(const QRectF &rect);
  ~pqPlusPointMarkerInternal() {}

  QRectF Rect;
};


class pqSquarePointMarkerInternal
{
public:
  pqSquarePointMarkerInternal(const QRectF &rect);
  ~pqSquarePointMarkerInternal() {}

  QRectF Rect;
};


class pqCirclePointMarkerInternal
{
public:
  pqCirclePointMarkerInternal(const QRectF &rect);
  ~pqCirclePointMarkerInternal() {}

  QRectF Rect;
};


class pqDiamondPointMarkerInternal
{
public:
  pqDiamondPointMarkerInternal();
  ~pqDiamondPointMarkerInternal() {}

  QPolygonF Diamond;
};


//----------------------------------------------------------------------------
pqCrossPointMarkerInternal::pqCrossPointMarkerInternal(const QRectF &rect)
  : Rect(rect)
{
}


//----------------------------------------------------------------------------
pqPlusPointMarkerInternal::pqPlusPointMarkerInternal(const QRectF &rect)
  : Rect(rect)
{
}


//----------------------------------------------------------------------------
pqSquarePointMarkerInternal::pqSquarePointMarkerInternal(const QRectF &rect)
  : Rect(rect)
{
}


//----------------------------------------------------------------------------
pqCirclePointMarkerInternal::pqCirclePointMarkerInternal(const QRectF &rect)
  : Rect(rect)
{
}


//----------------------------------------------------------------------------
pqDiamondPointMarkerInternal::pqDiamondPointMarkerInternal()
  : Diamond()
{
}


//----------------------------------------------------------------------------
void pqPointMarker::drawMarker(QPainter &painter)
{
  painter.drawPoint(0, 0);
}


//----------------------------------------------------------------------------
pqCrossPointMarker::pqCrossPointMarker(const QSize &size)
{
  this->Internal = new pqCrossPointMarkerInternal(QRectF(
      -size.width() * 0.5, -size.height() * 0.5, size.width(), size.height()));
}

pqCrossPointMarker::~pqCrossPointMarker()
{
  delete this->Internal;
}

void pqCrossPointMarker::drawMarker(QPainter &painter)
{
  painter.drawLine(this->Internal->Rect.topLeft(),
      this->Internal->Rect.bottomRight());
  painter.drawLine(this->Internal->Rect.topRight(),
      this->Internal->Rect.bottomLeft());
}


//----------------------------------------------------------------------------
pqPlusPointMarker::pqPlusPointMarker(const QSize &size)
{
  this->Internal = new pqPlusPointMarkerInternal(QRectF(
      -size.width() * 0.5, -size.height() * 0.5, size.width(), size.height()));
}

pqPlusPointMarker::~pqPlusPointMarker()
{
  delete this->Internal;
}

void pqPlusPointMarker::drawMarker(QPainter &painter)
{
  painter.drawLine(QPointF(0, this->Internal->Rect.top()),
      QPointF(0, this->Internal->Rect.bottom()));
  painter.drawLine(QPointF(this->Internal->Rect.left(), 0),
      QPointF(this->Internal->Rect.right(), 0));
}


//----------------------------------------------------------------------------
pqSquarePointMarker::pqSquarePointMarker(const QSize &size)
{
  this->Internal = new pqSquarePointMarkerInternal(QRectF(
      -size.width() * 0.5, -size.height() * 0.5, size.width(), size.height()));
}

pqSquarePointMarker::~pqSquarePointMarker()
{
  delete this->Internal;
}

void pqSquarePointMarker::drawMarker(QPainter &painter)
{
  painter.drawRect(this->Internal->Rect);
}


//----------------------------------------------------------------------------
pqCirclePointMarker::pqCirclePointMarker(const QSize &size)
{
  this->Internal = new pqCirclePointMarkerInternal(QRectF(
      -size.width() * 0.5, -size.height() * 0.5, size.width(), size.height()));
}

pqCirclePointMarker::~pqCirclePointMarker()
{
  delete this->Internal;
}

void pqCirclePointMarker::drawMarker(QPainter &painter)
{
  painter.drawEllipse(this->Internal->Rect);
}


//----------------------------------------------------------------------------
pqDiamondPointMarker::pqDiamondPointMarker(const QSize &size)
{
  this->Internal = new pqDiamondPointMarkerInternal();

  // Set up the diamond polygon to fit in the given size.
  int halfHeight = size.height() / 2;
  int halfWidth = size.width() / 2;
  this->Internal->Diamond.append(QPointF(0, -halfHeight));
  this->Internal->Diamond.append(QPointF(halfWidth, 0));
  this->Internal->Diamond.append(QPointF(0, halfHeight));
  this->Internal->Diamond.append(QPointF(-halfWidth, 0));
  this->Internal->Diamond.append(QPointF(0, -halfHeight));
}

pqDiamondPointMarker::~pqDiamondPointMarker()
{
  delete this->Internal;
}

void pqDiamondPointMarker::drawMarker(QPainter &painter)
{
  painter.drawPolygon(this->Internal->Diamond);
}


