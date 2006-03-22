/*!
 * \file pqLinePlot.cxx
 *
 * \brief
 *   The pqLinePlot class is used to draw a piecewise linear
 *   function.
 *
 * \author Mark Richardson
 * \date   August 22, 2005
 */

#include "pqChartAxis.h"
#include "pqChartValue.h"
#include "pqMarkerPen.h"
#include "pqLinePlot.h"

#include <QHelpEvent>
#include <QPolygon>
#include <QToolTip>

#include <vtkType.h>

/////////////////////////////////////////////////////////////////////////
// pqLinePlot::pqImplementation

class pqLinePlot::pqImplementation
{
public:
  pqImplementation(pqMarkerPen* pen) :
    Pen(pen)
  {
  }
  
  ~pqImplementation()
  {
    delete Pen;
  }

  void setCoordinates(const pqChartCoordinateList& list)
  {
    this->WorldCoords = list;
    this->ScreenCoords.clear();
    
    this->WorldMin = pqChartCoordinate(VTK_DOUBLE_MAX, VTK_DOUBLE_MAX);
    this->WorldMax = pqChartCoordinate(-VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX);
    
    for(pqChartCoordinateList::Iterator coords = this->WorldCoords.begin(); coords != this->WorldCoords.end(); ++coords)
      {
      this->WorldMin.X = vtkstd::min(this->WorldMin.X, coords->X);
      this->WorldMin.Y = vtkstd::min(this->WorldMin.Y, coords->Y);
      this->WorldMax.X = vtkstd::max(this->WorldMax.X, coords->X);
      this->WorldMax.Y = vtkstd::max(this->WorldMax.Y, coords->Y);
      }
  }
  
  /// Stores plot data in world coordinates
  pqChartCoordinateList WorldCoords;
  /// Caches plot data in screen coordinates
  QPolygon ScreenCoords;
  /// Stores the minimum world coordinate values
  pqChartCoordinate WorldMin;
  /// Stores the maximum world coordinate values
  pqChartCoordinate WorldMax;
  /// Stores the pen used for drawing
  pqMarkerPen* const Pen;
};

////////////////////////////////////////////////////////////////////////////
// pqLinePlot

pqLinePlot::pqLinePlot(pqMarkerPen* pen, const pqChartCoordinateList& coords) :
  pqAbstractPlot(),
  Implementation(new pqImplementation(pen))
{
  this->Implementation->setCoordinates(coords);
}

pqLinePlot::pqLinePlot(pqMarkerPen* pen, const pqChartCoordinate& p1, const pqChartCoordinate& p2) :
  pqAbstractPlot(),
  Implementation(new pqImplementation(pen))
{
  pqChartCoordinateList coords;
  coords.pushBack(p1);
  coords.pushBack(p2);
  this->Implementation->setCoordinates(coords);
}

pqLinePlot::~pqLinePlot()
{
  delete this->Implementation;
}

const pqChartCoordinate pqLinePlot::getMinimum() const
{
  return this->Implementation->WorldMin;
}

const pqChartCoordinate pqLinePlot::getMaximum() const
{
  return this->Implementation->WorldMax;
}

void pqLinePlot::layoutPlot(const pqChartAxis& XAxis, const pqChartAxis& YAxis)
{
  this->Implementation->ScreenCoords.resize(this->Implementation->WorldCoords.getSize());
  for(int i = 0; i != this->Implementation->WorldCoords.getSize(); ++i)
    {
    this->Implementation->ScreenCoords[i].rx() = XAxis.getPixelFor(this->Implementation->WorldCoords[i].X);
    this->Implementation->ScreenCoords[i].ry() = YAxis.getPixelFor(this->Implementation->WorldCoords[i].Y);
    }
}

void pqLinePlot::drawPlot(QPainter& painter, const QRect& /*area*/, const pqChartAxis& /*XAxis*/, const pqChartAxis& /*YAxis*/)
{
  this->Implementation->Pen->drawPolyline(painter, this->Implementation->ScreenCoords);
  if(this->Implementation->ScreenCoords.size())
    this->Implementation->Pen->drawPoint(painter, this->Implementation->ScreenCoords.back());
}

const double pqLinePlot::getDistance(const QPoint& coords) const
{
  double distance = VTK_DOUBLE_MAX;
  for(int i = 0; i != this->Implementation->ScreenCoords.size(); ++i)
    distance = vtkstd::min(distance, static_cast<double>((this->Implementation->ScreenCoords[i] - coords).manhattanLength()));
  return distance;
}

void pqLinePlot::showChartTip(QHelpEvent& event) const
{
  double tip_distance = VTK_DOUBLE_MAX;
  pqChartCoordinate tip_coordinate;
  for(int i = 0; i != this->Implementation->ScreenCoords.size(); ++i)
    {
    const double distance = (this->Implementation->ScreenCoords[i] - event.pos()).manhattanLength();
    if(distance < tip_distance)
      {
      tip_distance = distance;
      tip_coordinate = this->Implementation->WorldCoords[i];
      }
    }

  if(tip_distance < VTK_DOUBLE_MAX)    
    QToolTip::showText(event.globalPos(), QString("%1, %2").arg(tip_coordinate.X.getDoubleValue()).arg(tip_coordinate.Y.getDoubleValue()));
}

