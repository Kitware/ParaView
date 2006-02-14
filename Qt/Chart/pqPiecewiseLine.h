/*!
 * \file pqPiecewiseLine.h
 *
 * \brief
 *   The pqPiecewiseLine class is used to draw a piecewise linear
 *   function.
 *
 * \author Mark Richardson
 * \date   August 22, 2005
 */

#ifndef Q_PIECEWISE_LINE_H
#define Q_PIECEWISE_LINE_H

#include "pqChartExport.h"
#include "pqLinePlot.h"

class pqChartCoordinate;
class pqChartCoordinateList;
class pqChartValue;
class pqPiecewiseLineData;


/// \class pqPiecewiseLine
/// \brief
///   The pqPiecewiseLine class is used to draw a piecewise linear
///   function.
class QTCHART_EXPORT pqPiecewiseLine : public pqLinePlot
{
public:
  /// \brief
  ///   Creates a new piecewise linear function.
  /// \param parent The parent object.
  pqPiecewiseLine(QObject *parent=0);
  virtual ~pqPiecewiseLine();

  /// \name pqLinePlot Methods
  //@{
  virtual bool isPolyLine() const {return true;}

  virtual int getCoordinateCount() const;
  virtual bool getCoordinate(int index, pqChartCoordinate &coord) const;

  virtual void getMaxX(pqChartValue &value) const;
  virtual void getMinX(pqChartValue &value) const;

  virtual void getMaxY(pqChartValue &value) const;
  virtual void getMinY(pqChartValue &value) const;
  
  virtual void showTooltip(int index, QHelpEvent& event) const;
  //@}

  /// \brief
  ///   Sets the list of coordinates for the plot.
  /// \param list The new list of coordinates.
  void setCoordinates(const pqChartCoordinateList &list);

private:
  pqPiecewiseLineData *Data; ///< Stores the list of coordinates.
};

#endif
