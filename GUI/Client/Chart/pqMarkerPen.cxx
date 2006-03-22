#include "pqMarkerPen.h"
#include <QPainter>

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqMarkerPen

pqMarkerPen::pqMarkerPen(const QPen& pen) :
  Pen(pen)
{
}

pqMarkerPen::~pqMarkerPen()
{
}

QPen pqMarkerPen::getPen()
{
  return this->Pen;
}

void pqMarkerPen::drawLine(QPainter& painter, const QLineF& line)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawLine(line);
  this->drawMarker(painter, line.p1());
  painter.restore();
}

void pqMarkerPen::drawLine(QPainter& painter, const QLine& line)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawLine(line);
  this->drawMarker(painter, line.p1());
  painter.restore();
}

void pqMarkerPen::drawLine(QPainter& painter, const QPoint& p1, const QPoint& p2)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawLine(p1, p2);
  this->drawMarker(painter, p1);
  painter.restore();
}

void pqMarkerPen::drawLine(QPainter& painter, const QPointF& p1, const QPointF& p2)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawLine(p1, p2);
  this->drawMarker(painter, p1);
  painter.restore();
}

void pqMarkerPen::drawLine(QPainter& painter, int x1, int y1, int x2, int y2)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawLine(x1, y1, x2, y2);
  this->drawMarker(painter, QPoint(x1, y1));
  painter.restore();
}

void pqMarkerPen::drawPoint(QPainter& painter, const QPointF& position)
{
  painter.save();
  this->setupPainter(painter);
  painter.translate(position);
  this->drawMarker(painter);
  painter.restore();
}

void pqMarkerPen::drawPoint(QPainter& painter, const QPoint& position)
{
  painter.save();
  this->setupPainter(painter);
  painter.translate(position);
  this->drawMarker(painter);
  painter.restore();
}

void pqMarkerPen::drawPoint(QPainter& painter, int x, int y)
{
  painter.save();
  this->setupPainter(painter);
  painter.translate(QPoint(x, y));
  this->drawMarker(painter);
  painter.restore();
}

void pqMarkerPen::drawPoints(QPainter& painter, const QPointF* points, int pointCount)
{
  painter.save();
  this->setupPainter(painter);
  for(int i = 0; i != pointCount; ++i)
    {
    painter.save();
    painter.translate(points[i]);
    this->drawMarker(painter);
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
    painter.translate(points[i]);
    this->drawMarker(painter);
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
    painter.translate(points[i]);
    this->drawMarker(painter);
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
    painter.translate(points[i]);
    this->drawMarker(painter);
    painter.restore();
    }
  painter.restore();
}

void pqMarkerPen::drawPolyline(QPainter& painter, const QPointF* points, int pointCount)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawPolyline(points, pointCount);
  this->setupPainter(painter);
  for(int i = 0; i < pointCount - 1; ++i)
    {
    painter.save();
    painter.translate(points[i]);
    this->drawMarker(painter);
    painter.restore();
    }
  painter.restore();
}

void pqMarkerPen::drawPolyline(QPainter& painter, const QPoint* points, int pointCount)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawPolyline(points, pointCount);
  this->setupPainter(painter);
  for(int i = 0; i < pointCount - 1; ++i)
    {
    painter.save();
    painter.translate(points[i]);
    this->drawMarker(painter);
    painter.restore();
    }
  painter.restore();
}

void pqMarkerPen::drawPolyline(QPainter& painter, const QPolygonF& points)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawPolyline(points);
  this->setupPainter(painter);
  for(int i = 0; i < points.size() - 1; ++i)
    {
    painter.save();
    painter.translate(points[i]);
    this->drawMarker(painter);
    painter.restore();
    }
  painter.restore();
}

void pqMarkerPen::drawPolyline(QPainter& painter, const QPolygon& points)
{
  painter.save();
  painter.setPen(this->Pen);
  painter.drawPolyline(points);
  this->setupPainter(painter);
  for(int i = 0; i < points.size() - 1; ++i)
    {
    painter.save();
    painter.translate(points[i]);
    this->drawMarker(painter);
    painter.restore();
    }
  painter.restore();
}

void pqMarkerPen::drawMarker(QPainter& painter, const QPoint& point)
{
  this->setupPainter(painter);
  painter.translate(point);
  this->drawMarker(painter);
}

void pqMarkerPen::drawMarker(QPainter& painter, const QPointF& point)
{
  this->setupPainter(painter);
  painter.translate(point);
  this->drawMarker(painter);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// pqNullMarkerPen

pqNullMarkerPen::pqNullMarkerPen(const QPen& pen) :
  pqMarkerPen(pen)
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

pqCrossMarkerPen::pqCrossMarkerPen(const QPen& pen, const QSize& size, const QPen& outline) :
  pqMarkerPen(pen),
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

pqPlusMarkerPen::pqPlusMarkerPen(const QPen& pen, const QSize& size, const QPen& outline) :
  pqMarkerPen(pen),
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

pqSquareMarkerPen::pqSquareMarkerPen(const QPen& pen, const QSize& size, const QPen& outline, const QBrush& interior) :
  pqMarkerPen(pen),
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

pqCircleMarkerPen::pqCircleMarkerPen(const QPen& pen, const QSize& size, const QPen& outline, const QBrush& interior) :
  pqMarkerPen(pen),
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
