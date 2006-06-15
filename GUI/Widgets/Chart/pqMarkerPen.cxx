/*=========================================================================

   Program:   ParaQ
   Module:    pqMarkerPen.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqMarkerPen.h"

#include <QPainter>

#include <vtkstd/algorithm>

//////////////////////////////////////////////////////////////////////////////
// pqMarkerPen::pqImplementation

class pqMarkerPen::pqImplementation
{
public:
  pqImplementation(const QPen& pen, unsigned int marker_interval) :
    Pen(pen),
    MarkerInterval(marker_interval ? marker_interval : 1),
    MarkerIndex(0)
  {
  }
  
  /// Internal implementation detail
  void intervalDrawMarker(QPainter& painter, const QPoint& point)
  {
  }
  
  /// Internal implementation detail
  void intervalDrawMarker(QPainter& painter, const QPointF& point)
  {
  }
  
  /// Stores the internal pen
  const QPen Pen;
  /// Stores the interval at which markers will be drawn
  const unsigned int MarkerInterval;
  /// Index that is used to keep track of which markers should be drawn
  unsigned int MarkerIndex;
};

//////////////////////////////////////////////////////////////////////////////
// pqMarkerPen

pqMarkerPen::pqMarkerPen(const QPen& pen, unsigned int marker_interval) :
  Implementation(new pqImplementation(pen, marker_interval))
{
}

pqMarkerPen::~pqMarkerPen()
{
  delete this->Implementation;
}

void pqMarkerPen::resetMarkers(unsigned int offset)
{
  this->Implementation->MarkerIndex = offset;
}

QPen pqMarkerPen::getPen()
{
  return this->Implementation->Pen;
}

void pqMarkerPen::drawLine(QPainter& painter, const QLineF& line)
{
  painter.save();

  painter.setPen(this->Implementation->Pen);
  painter.drawLine(line);

  this->setupPainter(painter);
  this->intervalDrawMarker(painter, line.p1());

  painter.restore();
}

void pqMarkerPen::drawLine(QPainter& painter, const QLine& line)
{
  painter.save();

  painter.setPen(this->Implementation->Pen);
  painter.drawLine(line);

  this->setupPainter(painter);
  this->intervalDrawMarker(painter, line.p1());

  painter.restore();
}

void pqMarkerPen::drawLine(QPainter& painter, const QPoint& p1, const QPoint& p2)
{
  painter.save();

  painter.setPen(this->Implementation->Pen);
  painter.drawLine(p1, p2);

  this->setupPainter(painter);
  this->intervalDrawMarker(painter, p1);

  painter.restore();
}

void pqMarkerPen::drawLine(QPainter& painter, const QPointF& p1, const QPointF& p2)
{
  painter.save();

  painter.setPen(this->Implementation->Pen);
  painter.drawLine(p1, p2);

  this->setupPainter(painter);
  this->intervalDrawMarker(painter, p1);

  painter.restore();
}

void pqMarkerPen::drawLine(QPainter& painter, int x1, int y1, int x2, int y2)
{
  painter.save();

  painter.setPen(this->Implementation->Pen);
  painter.drawLine(x1, y1, x2, y2);

  this->setupPainter(painter);
  this->intervalDrawMarker(painter, QPoint(x1, y1));

  painter.restore();
}

void pqMarkerPen::drawPoint(QPainter& painter, const QPointF& position)
{
  painter.save();

  this->setupPainter(painter);
  this->intervalDrawMarker(painter, position);

  painter.restore();
}

void pqMarkerPen::drawPoint(QPainter& painter, const QPoint& position)
{
  painter.save();
  
  this->setupPainter(painter);
  this->intervalDrawMarker(painter, position);

  painter.restore();
}

void pqMarkerPen::drawPoint(QPainter& painter, int x, int y)
{
  painter.save();
  
  this->setupPainter(painter);
  this->intervalDrawMarker(painter, QPoint(x, y));
  
  painter.restore();
}

void pqMarkerPen::drawPoints(QPainter& painter, const QPointF* points, int pointCount)
{
  painter.save();

  this->setupPainter(painter);
  for(int i = 0; i != pointCount; ++i)
    {
    painter.save();
    this->intervalDrawMarker(painter, points[i]);
    painter.restore();
    }

  painter.restore();
}

void pqMarkerPen::drawPoints(QPainter& painter, const QPoint* points, int pointCount)
{
  painter.save();

  this->setupPainter(painter);
  for(int i = 0; i != pointCount; ++i)
    {
    painter.save();
    this->intervalDrawMarker(painter, points[i]);
    painter.restore();
    }

  painter.restore();
}

void pqMarkerPen::drawPoints(QPainter& painter, const QPolygonF& points)
{
  painter.save();

  this->setupPainter(painter);
  for(int i = 0; i != points.size(); ++i)
    {
    painter.save();
    this->intervalDrawMarker(painter, points[i]);
    painter.restore();
    }

  painter.restore();
}

void pqMarkerPen::drawPoints(QPainter& painter, const QPolygon& points)
{
  painter.save();

  this->setupPainter(painter);
  for(int i = 0; i != points.size(); ++i)
    {
    painter.save();
    this->intervalDrawMarker(painter, points[i]);
    painter.restore();
    }

  painter.restore();
}

void pqMarkerPen::drawPolyline(QPainter& painter, const QPointF* points, int pointCount)
{
  painter.save();
  
  painter.setPen(this->Implementation->Pen);
  this->safeDrawPolyline(painter, points, pointCount);

  this->setupPainter(painter);
  for(int i = 0; i < pointCount - 1; ++i)
    {
    painter.save();
    this->intervalDrawMarker(painter, points[i]);
    painter.restore();
    }
    
  painter.restore();
}

void pqMarkerPen::drawPolyline(QPainter& painter, const QPoint* points, int pointCount)
{
  painter.save();
  
  painter.setPen(this->Implementation->Pen);
  this->safeDrawPolyline(painter, points, pointCount);
  
  this->setupPainter(painter);
  for(int i = 0; i < pointCount - 1; ++i)
    {
    painter.save();
    this->intervalDrawMarker(painter, points[i]);
    painter.restore();
    }
    
  painter.restore();
}

void pqMarkerPen::drawPolyline(QPainter& painter, const QPolygonF& points)
{
  painter.save();

  painter.setPen(this->Implementation->Pen);
  this->safeDrawPolyline(painter, &points[0], points.size());

  this->setupPainter(painter);
  for(int i = 0; i < points.size() - 1; ++i)
    {
    painter.save();
    this->intervalDrawMarker(painter, points[i]);
    painter.restore();
    }

  painter.restore();
}

void pqMarkerPen::drawPolyline(QPainter& painter, const QPolygon& points)
{
  painter.save();
  
  painter.setPen(this->Implementation->Pen);
  this->safeDrawPolyline(painter, &points[0], points.size());

  this->setupPainter(painter);
  for(int i = 0; i < points.size() - 1; ++i)
    {
    painter.save();
    this->intervalDrawMarker(painter, points[i]);
    painter.restore();
    }
    
  painter.restore();
}

void pqMarkerPen::safeDrawPolyline(QPainter& painter, const QPointF* points, int pointCount)
{
  // This is a workaround for crashes we've seen on X drawing polylines with large numbers
  // of points - in theory, both Qt and X should hide these details, in practice, they don't :(
  for(int i = 0; i < pointCount; i += 100)
    {
    painter.drawPolyline(&points[i], vtkstd::min(100, pointCount - i));
    }
}

void pqMarkerPen::safeDrawPolyline(QPainter& painter, const QPoint* points, int pointCount)
{
  // This is a workaround for crashes we've seen on X drawing polylines with large numbers
  // of points - in theory, both Qt and X should hide these details, in practice, they don't :(
  for(int i = 0; i < pointCount; i += 100)
    {
    painter.drawPolyline(&points[i], vtkstd::min(100, pointCount - i));
    }
}

void pqMarkerPen::intervalDrawMarker(QPainter& painter, const QPoint& point)
{
  if(0 != (this->Implementation->MarkerIndex++ % this->Implementation->MarkerInterval))
    return;
    
  painter.translate(point);
  this->drawMarker(painter);
}

void pqMarkerPen::intervalDrawMarker(QPainter& painter, const QPointF& point)
{
  if(0 != (this->Implementation->MarkerIndex++ % this->Implementation->MarkerInterval))
    return;
    
  painter.translate(point);
  this->drawMarker(painter);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqNullMarkerPen

pqNullMarkerPen::pqNullMarkerPen(const QPen& pen) :
  pqMarkerPen(pen, 0)
{
}

void pqNullMarkerPen::setupPainter(QPainter& /*painter*/)
{
}

void pqNullMarkerPen::drawMarker(QPainter& /*painter*/)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqCrossMarkerPen

pqCrossMarkerPen::pqCrossMarkerPen(const QPen& pen, const QSize& size, const QPen& outline, unsigned int marker_interval) :
  pqMarkerPen(pen, marker_interval),
  Rect(-size.width() * 0.5, -size.height() * 0.5, size.width(), size.height()),
  Outline(outline)
{
}

void pqCrossMarkerPen::setupPainter(QPainter& painter)
{
  painter.setPen(this->Outline);
}

void pqCrossMarkerPen::drawMarker(QPainter& painter)
{
  painter.drawLine(this->Rect.topLeft(), this->Rect.bottomRight());
  painter.drawLine(this->Rect.topRight(), this->Rect.bottomLeft());
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqPlusMarkerPen

pqPlusMarkerPen::pqPlusMarkerPen(const QPen& pen, const QSize& size, const QPen& outline, unsigned int marker_interval) :
  pqMarkerPen(pen, marker_interval),
  Rect(-size.width() * 0.5, -size.height() * 0.5, size.width(), size.height()),
  Outline(outline)
{
}

void pqPlusMarkerPen::setupPainter(QPainter& painter)
{
  painter.setPen(this->Outline);
}

void pqPlusMarkerPen::drawMarker(QPainter& painter)
{
  painter.drawLine(QPointF(0, this->Rect.top()), QPointF(0, this->Rect.bottom()));
  painter.drawLine(QPointF(this->Rect.left(), 0), QPointF(this->Rect.right(), 0));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqSquareMarkerPen

pqSquareMarkerPen::pqSquareMarkerPen(const QPen& pen, const QSize& size, const QPen& outline, const QBrush& interior, unsigned int marker_interval) :
  pqMarkerPen(pen, marker_interval),
  Rect(-size.width() * 0.5, -size.height() * 0.5, size.width(), size.height()),
  Outline(outline),
  Interior(interior)
{
}

void pqSquareMarkerPen::setupPainter(QPainter& painter)
{
  painter.setPen(this->Outline);
  painter.setBrush(this->Interior);
}

void pqSquareMarkerPen::drawMarker(QPainter& painter)
{
  painter.drawRect(this->Rect);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqCircleMarkerPen

pqCircleMarkerPen::pqCircleMarkerPen(const QPen& pen, const QSize& size, const QPen& outline, const QBrush& interior, unsigned int marker_interval) :
  pqMarkerPen(pen, marker_interval),
  Rect(-size.width() * 0.5, -size.height() * 0.5, size.width(), size.height()),
  Outline(outline),
  Interior(interior)
{
}

void pqCircleMarkerPen::setupPainter(QPainter& painter)
{
  painter.setPen(this->Outline);
  painter.setBrush(this->Interior);
}

void pqCircleMarkerPen::drawMarker(QPainter& painter)
{
  painter.drawEllipse(this->Rect);
}
