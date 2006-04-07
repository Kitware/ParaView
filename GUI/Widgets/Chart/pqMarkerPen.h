/*=========================================================================

   Program:   ParaQ
   Module:    pqMarkerPen.h

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

#ifndef _pqMarkerPen_h
#define _pqMarkerPen_h

#include "QtChartExport.h"

#include <QBrush>
#include <QPen>
#include <QRect>

class QLine;
class QLineF;
class QPainter;
class QPoint;
class QPointF;
class QPolygon;
class QPolygonF;
class QSize;

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqMarkerPen

/// Abstract interface for a pen that can be used to draw line segments with optional "markers", typically for line graphs
class QTCHART_EXPORT pqMarkerPen
{
public:
  virtual ~pqMarkerPen();

  /// Returns the pen that will be used to draw lines (without the markers)
  QPen getPen();

  //@{
  /**
  Methods for drawing lines and points with markers.  These methods are intentionally modelled on the
  corresponding QPainter methods.  Note that the line-drawing methods will draw a marker at each vertex
  except the last - allowing you to chain multiple line-drawing calls together or draw a closed loop
  without drawing redundant or overlapping markers.  That means that you must explicitly draw the marker
  for the last vertex in a line, e.g. to draw a line with markers at each end:
  
    pen.drawLine(p1, p2);
    pen.drawPoint(p2);
  
  to draw a closed loop:
  
    pen.drawPolyline(points);
    
  to draw an open loop:
  
    pen.drawPolyline(points);
    pen.drawPoint(points[points.size() - 1]);
  
  */
  
  void drawLine(QPainter& painter, const QLineF& line);
  void drawLine(QPainter& painter, const QLine& line);
  void drawLine(QPainter& painter, const QPoint& p1, const QPoint& p2);
  void drawLine(QPainter& painter, const QPointF& p1, const QPointF& p2);
  void drawLine(QPainter& painter, int x1, int y1, int x2, int y2);
//  void drawLines(QPainter& painter, const QLineF* lines, int lineCount);
//  void drawLines(QPainter& painter, const QLine* lines, int lineCount);
//  void drawLines(QPainter& painter, const QPointF* pointPairs, int lineCount);
//  void drawLines(QPainter& painter, const QPoint* pointPairs, int lineCount);
//  void drawLines(QPainter& painter, const QVector<QPointF>& pointPairs);
//  void drawLines(QPainter& painter, const QVector<QPoint>& pointPairs);
//  void drawLines(QPainter& painter, const QVector<QLineF>& lines);
//  void drawLines(QPainter& painter, const QVector<QLine>& lines);
  void drawPoint(QPainter& painter, const QPointF& position);
  void drawPoint(QPainter& painter, const QPoint& position);
  void drawPoint(QPainter& painter, int x, int y);
  void drawPoints(QPainter& painter, const QPointF* points, int pointCount);
  void drawPoints(QPainter& painter, const QPoint* points, int pointCount);
  void drawPoints(QPainter& painter, const QPolygonF& points);
  void drawPoints(QPainter& painter, const QPolygon& points);
  void drawPolyline(QPainter& painter, const QPointF* points, int pointCount);
  void drawPolyline(QPainter& painter, const QPoint* points, int pointCount);
  void drawPolyline(QPainter& painter, const QPolygonF& points);
  void drawPolyline(QPainter& painter, const QPolygon& points);
  //@}

protected:
  pqMarkerPen(const QPen& pen);

private:
  /// Internal implementation detail
  void drawMarker(QPainter& painter, const QPoint& point);
  /// Internal implementation detail
  void drawMarker(QPainter& painter, const QPointF& point);
  
  /// Called by the implementation to setup a painter - derivatives should implement this and set the painter state as-needed before drawMarker() is called one-to-many times
  virtual void setupPainter(QPainter& painter) = 0;
  /// Called by the implementation to draw a marker - derivatives should implement this and draw a marker centered on the origin - the given painter will already be initialized correctly
  virtual void drawMarker(QPainter& painter) = 0;
  
  /// Stores the internal pen
  const QPen Pen;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqNullMarkerPen

/// Concrete implementation of pqMarkerPen that does not draw markers
class QTCHART_EXPORT pqNullMarkerPen :
  public pqMarkerPen
{
public:
  pqNullMarkerPen(const QPen& pen);
  
private:
  void setupPainter(QPainter& painter);
  void drawMarker(QPainter& painter);
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqCrossMarkerPen

/// Concrete implementation of pqMarkerPen that draws small crosses ("X") as markers
class QTCHART_EXPORT pqCrossMarkerPen :
  public pqMarkerPen
{
public:
  pqCrossMarkerPen(const QPen& pen, const QSize& size, const QPen& outline);
  
private:
  void setupPainter(QPainter& painter);
  void drawMarker(QPainter& painter);
  
  const QRectF Rect;
  const QPen Outline;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqPlusMarkerPen

/// Concrete implementation of pqMarkerPen that draws small plus-signs ("+") as markers
class QTCHART_EXPORT pqPlusMarkerPen :
  public pqMarkerPen
{
public:
  pqPlusMarkerPen(const QPen& pen, const QSize& size, const QPen& outline);
  
private:
  void setupPainter(QPainter& painter);
  void drawMarker(QPainter& painter);
  
  const QRectF Rect;
  const QPen Outline;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqSquareMarkerPen

/// Concrete implementation of pqMarkerPen that draws squares as markers
class QTCHART_EXPORT pqSquareMarkerPen :
  public pqMarkerPen
{
public:
  pqSquareMarkerPen(const QPen& pen, const QSize& size, const QPen& outline, const QBrush& interior);
  
private:
  void setupPainter(QPainter& painter);
  void drawMarker(QPainter& painter);
  
  const QRectF Rect;
  const QPen Outline;
  const QBrush Interior;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqCircleMarkerPen

/// Concrete implementation of pqMarkerPen that draws circles as markers
class QTCHART_EXPORT pqCircleMarkerPen :
  public pqMarkerPen
{
public:
  pqCircleMarkerPen(const QPen& pen, const QSize& size, const QPen& outline, const QBrush& interior);
  
private:
  void setupPainter(QPainter& painter);
  void drawMarker(QPainter& painter);
  
  const QRectF Rect;
  const QPen Outline;
  const QBrush Interior;
};

#endif
