/*!
 * \file pqLinePlot.h
 *
 * \brief
 *   The pqLinePlot class is the drawing interface to a function.
 *
 * \author Mark Richardson
 * \date   August 2, 2005
 */

#ifndef _pqLinePlot_h
#define _pqLinePlot_h

#include "pqChartExport.h"
#include <QObject>
#include <QColor> // Needed for color member.

class pqChartCoordinate;
class pqChartValue;


/// \class pqLinePlot
/// \brief
///   The pqLinePlot class is the drawing interface to a function.
///
/// The pqLineChart uses this interface to draw functions. In order
/// to have a function show up on the line chart, a new class must
/// be created that inherits from %pqLinePlot.
class QTCHART_EXPORT pqLinePlot : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line plot instance.
  /// \param parent The parent object.
  pqLinePlot(QObject *parent=0);
  virtual ~pqLinePlot() {}

  /// \name Data Query Methods
  //@{
  /// \brief
  ///   Used to determine if the points are all connected.
  ///
  /// The return value of this method is used to switch between
  /// drawing the points as a polyline or as line segments. When
  /// the function plot is not a polyline, the points are
  /// connected in pairs.
  ///
  /// \return
  ///   True if all points are connected sequentially.
  virtual bool isPolyLine() const = 0;

  /// \brief
  ///   Gets the total number of coordinates for the plot.
  /// \return
  ///   The number of coordinates in the plot.
  virtual int getCoordinateCount() const = 0;

  /// \brief
  ///   Gets the specified coordinate.
  ///
  /// The owning line chart uses this method to get all the
  /// coordinates that need to be drawn. The index is relative
  /// to the total number of coordinates. The coordinates are
  /// converted to pixel coordinates when the chart layout is
  /// calculated.
  ///
  /// \param index The index of the desired coordinate.
  /// \param coord Used to return the coordinate.
  /// \return
  ///   True if a valid coordinate was returned.
  /// \sa pqLinePlot::setModified(bool)
  virtual bool getCoordinate(int index, pqChartCoordinate &coord) const = 0;

  /// \brief
  ///   Gets the maximum x-axis value for the plot.
  /// \param value Used to return the value.
  virtual void getMaxX(pqChartValue &value) const = 0;

  /// \brief
  ///   Gets the minimum x-axis value for the plot.
  /// \param value Used to return the value.
  virtual void getMinX(pqChartValue &value) const = 0;

  /// \brief
  ///   Gets the maximum y-axis value for the plot.
  /// \param value Used to return the value.
  virtual void getMaxY(pqChartValue &value) const = 0;

  /// \brief
  ///   Gets the minimum y-axis value for the plot.
  /// \param value Used to return the value.
  virtual void getMinY(pqChartValue &value) const = 0;

  /// \brief
  ///   Sets whether of not the plot has been modified.
  ///
  /// The plot should only be considered modifed when the data
  /// has changed. Changing the drawing parameters will already
  /// signal the owning line chart that a repaint is needed.
  /// When this method is called, the owning chart must
  /// recalculate the layout before repainting the chart.
  ///
  /// \param modified True if the plot has changed.
  void setModified(bool modified);

  /// \brief
  ///   Gets whether or not the plot has been modified.
  bool isModified() const {return this->Modified;}
  //@}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Sets the color used to draw the line plot.
  /// \param color The new color to draw the line plot.
  void setColor(const QColor &color);

  /// \brief
  ///   Gets the color used to draw the line plot.
  /// \return
  ///   The color used to draw the line plot.
  const QColor &getColor() const {return this->Color;}

  /// \brief
  ///   Sets the width used to draw the line plot.
  /// \param width The new line width of the plot.
  void setWidth(int width);

  /// \brief
  ///   Gets the width used to draw the line plot.
  /// \return
  ///   The drawing width of the line plot.
  int getWidth() const {return this->Thickness;}
  //@}

signals:
  /// \brief
  ///   Called when the line plot has been modified.
  ///
  /// The modified signal is sent when the data has changed or
  /// when any of the drawing parameters have changed.
  ///
  /// \param plot A pointer to the modified line plot.
  void plotModified(const pqLinePlot *plot);

private:
  QColor Color;  ///< Stores the line plot's color.
  int Thickness; ///< Stores the line thickness.
  bool Modified; ///< True if the data has changed.
};

#endif
