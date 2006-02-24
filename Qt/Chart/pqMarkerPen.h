#ifndef _pqMarkerPen_h
#define _pqMarkerPen_h

#include "pqChartExport.h"

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
  
//  void drawConvexPolygon(QPainter& painter, const QPointF* points, int pointCount);
//  void drawConvexPolygon(QPainter& painter, const QPoint* points, int pointCount);
//  void drawConvexPolygon(QPainter& painter, const QPolygonF& polygon);
//  void drawConvexPolygon(QPainter& painter, const QPolygon& polygon);
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
//  void drawPolygon(QPainter& painter, const QPointF* points, int pointCount, Qt::FillRule fillRule = Qt::OddEvenFill);
//  void drawPolygon(QPainter& painter, const QPoint* points, int pointCount, Qt::FillRule fillRule = Qt::OddEvenFill);
//  void drawPolygon(QPainter& painter, const QPolygonF& points, Qt::FillRule fillRule = Qt::OddEvenFill);
//  void drawPolygon(QPainter& painter, const QPolygon& points, Qt::FillRule fillRule = Qt::OddEvenFill);
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
