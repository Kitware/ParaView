/*!
 * \file pqLinearRamp.h
 *
 * \brief
 *   The pqLinearRamp class is used to draw a linear ramp function.
 *
 * \author Mark Richardson
 * \date   August 3, 2005
 */

#ifndef _pqLinearRamp_h
#define _pqLinearRamp_h

#include "pqChartExport.h"
#include "pqLinePlot.h"
#include "pqChartCoordinate.h" // Needed for endpoint members.

class pqChartValue;


/// \class pqLinearRamp
/// \brief
///   The pqLinearRamp class is used to draw a linear ramp function.
class QTCHART_EXPORT pqLinearRamp : public pqLinePlot
{
public:
  /// \brief
  ///   Creates a new linear ramp function.
  /// \param parent The parent object.
  pqLinearRamp(QObject *parent=0);
  virtual ~pqLinearRamp() {}

  /// \name pqLinePlot Methods
  //@{
  virtual bool isPolyLine() const {return true;}

  virtual int getCoordinateCount() const {return 2;}
  virtual bool getCoordinate(int index, pqChartCoordinate &coord) const;

  virtual void getMaxX(pqChartValue &value) const;
  virtual void getMinX(pqChartValue &value) const;

  virtual void getMaxY(pqChartValue &value) const;
  virtual void getMinY(pqChartValue &value) const;
  //@}

  /// \brief
  ///   Sets the coordinates for both ends of the line.
  /// \param p1 The first end coordinate.
  /// \param p2 The second end coordinate.
  void setEndPoints(const pqChartCoordinate &p1, const pqChartCoordinate &p2);

  /// \brief
  ///   Sets the first end coordinate of the line.
  /// \param p1 The first end coordinate.
  void setEndPoint1(const pqChartCoordinate &p1);

  /// \brief
  ///   Sets the second end coordinate of the line.
  /// \param p2 The second end coordinate.
  void setEndPoint2(const pqChartCoordinate &p2);

private:
  pqChartCoordinate Point1; ///< Stores the first end coordinate.
  pqChartCoordinate Point2; ///< Stores the second end coordinate.
};

#endif
