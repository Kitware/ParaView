/*!
 * \file pqHistogramColor.h
 *
 * \brief
 *   The pqHistogramColor class is used to control the bar colors on
 *   a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   May 18, 2005
 */

#ifndef _pqHistogramColor_h
#define _pqHistogramColor_h

#include "pqChartExport.h"
#include <QColor> // Needed for QColor return type.


/// \class pqHistogramColor
/// \brief
///   The pqHistogramColor class is used to control the bar colors on
///   a pqHistogramChart.
///
/// The \c getColor method is used to get the color for a specific
/// histogram bar. The default implementation returns a color from
/// red to blue based on the bar's index. To get a different color
/// scheme, create a new class that inherits from pqHistogramColor
/// and overload the \c getColor method. The following example shows
/// how to make all the histogram bars the same color.
/// \code
/// class OneColor : public pqHistogramColor
/// {
///    public:
///       OneColor();
///       virtual ~OneColor() {}
///
///       virtual QColor getColor(int index, int total) const;
///       void setColor(QColor color) {this->color = color;}
///
///    private:
///       QColor color;
/// };
///
/// OneColor::OneColor()
///    : color(Qt::red)
/// {
/// }
///
/// QColor OneColor::getColor(int, int) const
/// {
///    return this->color;
/// }
/// \endcode
class QTCHART_EXPORT pqHistogramColor
{
public:
  pqHistogramColor() {}
  virtual ~pqHistogramColor() {}

  /// \brief
  ///   Called to get the color for a specified histogram bar.
  ///
  /// The default implementation returns a color from red to blue
  /// based on the index of the bar. Overload this method to change
  /// the default behavior.
  ///
  /// \param index The index of the bar.
  /// \param total The total number of bars in the histogram.
  /// \return
  ///   The color to paint the specified bar.
  virtual QColor getColor(int index, int total) const;
};

#endif
