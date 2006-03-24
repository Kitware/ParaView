/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

/*!
 * \file pqChartAxis.h
 *
 * \brief
 *   The pqChartAxis class is used to display an orthogonal
 *   chart axis.
 *
 * \author Mark Richardson
 * \date   May 12, 2005
 */

#ifndef _pqChartAxis_h
#define _pqChartAxis_h


#include "pqChartExport.h"
#include "pqChartValue.h" // Needed for min, max, etc. members.

#include <QObject>
#include <QRect>  // Needed for bounds member.
#include <QFont>  // Needed for font member.
#include <QColor> // Needed for grid and axis members.

class pqChartAxisData;
class pqChartLabel;
class QPainter;
class QFontMetrics;


/// \class pqChartAxis
/// \brief
///   The pqChartAxis class is used to draw an orthogonal chart
///   axis.
///
/// The axis object stores the drawing parameters for an axis as well
/// as a map between pixel and value coordinates. The pixel/value
/// mapping must have the pixel and value ranges set before it is
/// valid. This mapping can be used to determine the pixel sizes of
/// the chart data.
///
/// The axis can be layed out in one of two ways. The first method
/// is a fixed interval layout. The second method selects a suitable
/// interval based on the pixel range.
///
/// If the fixed interval method is used, it is possible for the
/// drawing area to be too small to display all the intervals. In that
/// case, some of the intervals will not be drawn. Even if all the
/// intervals of a fixed axis can be drawn, some of the labels might
/// overlap. To avoid this problem, some labels are skipped in the
/// drawing process.
///
/// The suitable interval layout method uses a list of predetermined
/// intervals that are mapped to the value range based on the label
/// precision. The interval will always leave a bit of padding for
/// the value range. This helps the chart look better and ensures the
/// data can be displayed.
class QTCHART_EXPORT pqChartAxis : public QObject
{
  Q_OBJECT

public:
  enum AxisLocation {
    Top,
    Left,
    Right,
    Bottom
  };

  enum AxisScale {
    Linear,
    Logarithmic
  };

  enum AxisLayout {
    FixedInterval,
    BestInterval
  };

  enum AxisGridColor {
    Lighter,
    Specified
  };

public:
  /// \brief
  ///   Creates an axis object with a specified location.
  /// \param location Where on the chart the axis will be drawn.
  /// \param parent The parent object.
  pqChartAxis(AxisLocation location, QObject *parent=0);
  virtual ~pqChartAxis();

  /// \name Pixel to Value Mapping
  //@{
  /// \brief
  ///   Sets the axis value range.
  /// \param min The minimum value.
  /// \param max The maximum value.
  void setValueRange(const pqChartValue &min, const pqChartValue &max);

  /// \brief
  ///   Gets the axis value range.
  /// \return 
  ///   The difference between the minimum and maximum values.
  pqChartValue getValueRange() const;

  /// \brief
  ///   Sets the minimum value for the axis.
  /// \param min The minimum value.
  void setMinValue(const pqChartValue &min);

  /// \brief
  ///   Gets the minimum value for the axis.
  /// \return
  ///   The minimum value for the axis.
  /// \sa pqChartAxis::getTrueMinValue()
  const pqChartValue &getMinValue() const {return this->ValueMin;}

  /// \brief
  ///   Gets the true minimum value for the axis.
  ///
  /// The true minimum is the actual minimum for the chart data values.
  /// When the chart is layed out, the minimum returned by the
  /// \c getMaxValue method might not be the same as the true minimum.
  ///
  /// \return
  ///   The true minimum value for the axis.
  /// \sa pqChartAxis::getMinValue()
  const pqChartValue &getTrueMinValue() const {return this->TrueMin;}

  /// \brief
  ///   Sets the maximum value for the axis.
  /// \param max The maximum value.
  void setMaxValue(const pqChartValue &max);

  /// \brief
  ///   Gets the maximum value for the axis.
  /// \return
  ///   The maximum value for the axis.
  /// \sa pqChartAxis::getTrueMaxValue()
  const pqChartValue &getMaxValue() const {return this->ValueMax;}

  /// \brief
  ///   Gets the true maximum value for the axis.
  ///
  /// The true maximum is the actual maximum for the chart data values.
  /// When the chart is layed out, the maximum returned by the
  /// \c getMaxValue method might not be the same as the true maximum.
  ///
  /// \return
  ///   The true maximum value for the axis.
  /// \sa pqChartAxis::getMaxValue()
  const pqChartValue &getTrueMaxValue() const {return this->TrueMax;}

  /// \brief
  ///   Gets the axis pixel range.
  ///
  /// The pixel range is always returned as a positive number.
  ///
  /// \return 
  ///   The difference between the minimum and maximum pixels.
  int getPixelRange() const;

  /// \brief
  ///   Gets the minimum pixel location for the axis.
  /// \return
  ///   The minimum pixel location for the axis.
  int getMinPixel() const {return this->PixelMin;}

  /// \brief
  ///   Gets the maximum pixel location for the axis.
  /// \return
  ///   The maximum pixel location for the axis.
  int getMaxPixel() const {return this->PixelMax;}

  /// \brief
  ///   Maps a value to a pixel.
  /// \param value The data value.
  /// \return
  ///   The pixel location for the given value.
  /// \sa pqChartAxis::isValid(),
  ///     pqChartAxis::getValueFor(int)
  int getPixelFor(const pqChartValue &value) const;

  /// \brief
  ///   Maps an index to a pixel.
  ///
  /// This method only returns valid pixel coordinates after the
  /// axis has been laid out.
  ///
  /// \param index The index of the axis tick mark.
  /// \return
  ///   The pixel location for the given index.
  /// \sa pqChartAxis::getPixelFor(const pqChartValue &)
  int getPixelForIndex(int index) const;

  /// \brief
  ///   Maps a pixel to a value.
  /// \param pixel The pixel location.
  /// \return
  ///   The value equivalent to the pixel location.
  /// \sa pqChartAxis::isValid(),
  ///     pqChartAxis::getPixelFor(const pqChartValue &)
  pqChartValue getValueFor(int pixel);

  /// \brief
  ///   Maps an index to a value.
  ///
  /// This method only returns valid values after the axis has
  /// been laid out.
  ///
  /// \param index The index of the axis tick mark.
  /// \return
  ///   The value for the given index.
  /// \sa pqChartAxis::getValueFor(int)
  pqChartValue getValueForIndex(int index) const;

  /// \brief
  ///   Used to determine if the pixel/value mapping is valid.
  /// \return
  ///   True if the pixel/value mapping is valid.
  bool isValid() const;

  /// \brief
  ///   Used to determine if zero is in the value range.
  /// \return
  ///   True if zero is in the value range.
  bool isZeroInRange() const;
  //@}

  /// \brief
  ///   Sets the axis color.
  ///
  /// If the grid color is tied to the axis color, the grid
  /// color will also be set.
  ///
  /// \param color The new axis color.
  /// \sa pqChartAxis::setGridColorType(AxisGridColor)
  void setAxisColor(const QColor &color);

  /// \brief
  ///   Gets the axis color.
  /// \return
  ///   The axis color.
  const QColor &getAxisColor() const {return this->AxisColor;}

  /// \brief
  ///   Sets the axis grid color.
  ///
  /// If the axis grid color type is \c Lighter, calling this
  /// method will not change the grid color.
  ///
  /// \param color The new axis grid color.
  void setGridColor(const QColor &color);

  /// \brief
  ///   Gets the axis grid color.
  /// \return
  ///   The axis grid color.
  const QColor &getGridColor() const {return this->GridColor;}

  /// \brief
  ///   Sets the axis grid color type.
  ///
  /// The axis grid color type determines if the grid color is
  /// tied to the axis color. If the grid color type is \c Lighter,
  /// the grid color will be a lighter version of the axis color.
  ///
  /// \param type The new axis grid color type.
  void setGridColorType(AxisGridColor type);

  /// \brief
  ///   Gets the axis grid color type.
  /// \return
  ///   The axis grid color type.
  /// \sa pqChartAxis::setGridColorType(AxisGridColor)
  AxisGridColor getGridColorType() const {return this->GridType;}

  /// \brief
  ///   Sets the axis tick label color.
  ///
  /// \param color The new axis tick label color.
  void setTickLabelColor(const QColor &color);

  /// \brief
  ///   Gets the axis tick label color.
  /// \return
  ///   The axis tick label color.
  const QColor &getTickLabelColor() const {return this->TickLabelColor;}

  /// \name Drawing Parameters
  //@{
  /// \brief
  ///   Sets the font for the axis tick labels.
  /// \param font The font to use.
  void setTickLabelFont(const QFont &font);

  /// \brief
  ///   Gets the font for the axis tick labels.
  /// \return
  ///   The font used to draw the axis tick labels.
  QFont getTickLabelFont() const {return this->TickLabelFont;}

  /// \brief
  ///   Sets the decimal precision of the axis labels.
  /// \param precision The number of decimal places to use.
  /// \sa pqChartValue::getString()
  void setPrecision(int precision);

  /// \brief
  ///   Gets the decimal precision of the axis labels.
  /// \return
  ///   The decimal precision of the axis labels.
  /// \sa pqChartValue::getString()
  int getPrecision() const {return this->Precision;}

  /// Returns the (optional) axis label
  pqChartLabel& getLabel() const { return *this->Label; }

  /// \brief
  ///   Sets whether or not the axis should be visible.
  /// \param visible True if the axis should be visible.
  void setVisible(bool visible);

  /// \brief
  ///   Gets whether or not the axis is visible.
  /// \return
  ///   True if the axis is visible.
  bool isVisible() const {return this->Visible;}

  /// \brief
  ///   Sets whether or not the grid should be visible.
  /// \param visible True if the grid should be visible.
  void setGridVisible(bool visible) {this->GridVisible = visible;}

  /// \brief
  ///   Gets whether or not the grid is visible.
  /// \return
  ///   True if the grid is visible.
  bool isGridVisible() const {return this->GridVisible;}
  //@}

  /// \name Layout Methods
  //@{
  /// \brief
  ///   Sets the axis layout type.
  /// \param layout The new layout type.
  /// \sa pqChartAxis::setNumberOfIntervals(int),
  ///     pqChartAxis::layoutAxis(const QRect &)
  void setLayoutType(AxisLayout layout) {this->Layout = layout;}

  /// \brief
  ///   Gets the axis layout type.
  /// \return
  ///   The current axis layout type.
  AxisLayout getLayoutType() const {return this->Layout;}

  /// \brief
  ///   Sets the axis scale type.
  /// \param scale The new axis scale type.
  void setScaleType(AxisScale scale) {this->Scale = scale;}

  /// \brief
  ///   Gets the axis scale type.
  /// \return
  ///   The current axis scale type.
  AxisScale getScaleType() const {return this->Scale;}

  /// \brief
  ///   Sets whether or not extra padding is used for the maximum.
  ///
  /// This setting only affects the \c BestInterval layout. When
  /// extra padding is turned on and the maximum value is a multiple
  /// of the determined interval, an extra interval will be added.
  /// The extra interval padding can be used to make the chart look
  /// more pleasing in some cases.
  ///
  /// \param on True if extra padding should be used for the maximum.
  void setExtraMaxPadding(bool on) {this->ExtraMaxPadding = on;}

  /// \brief
  ///   Gets whether the maximum will use extra padding.
  /// \return
  ///   True if the maximum will use extra padding.
  /// \sa pqChartAxis::setExtraMaxPadding(bool)
  bool isMaxExtraPadded() const {return this->ExtraMaxPadding;}

  /// \brief
  ///   Sets whether or not extra padding is used for the minimum.
  /// \param on True if extra padding should be used for the minimum.
  /// \sa pqChartAxis::setExtraMaxPadding(bool)
  void setExtraMinPadding(bool on) {this->ExtraMinPadding = on;}

  /// \brief
  ///   Gets whether the minimum will use extra padding.
  /// \return
  ///   True if the minimum will use extra padding.
  /// \sa pqChartAxis::setExtraMaxPadding(bool)
  bool isMinExtraPadded() const {return this->ExtraMinPadding;}

  /// \brief
  ///   Sets the number of intervals for the axis.
  ///
  /// This method is needed for the fixed interval layout. After
  /// the number of intervals is set, the fixed layout can be
  /// calculated.
  ///
  /// \param intervals The number of intervals.
  /// \sa pqChartAxis::setLayoutType(AxisLayout)
  void setNumberOfIntervals(int intervals);

  /// \brief
  ///   Gets the number of intervals on the axis.
  /// \return
  ///   The number of intervals on the axis.
  int getNumberOfIntervals() const {return this->Intervals;}

  /// \brief
  ///   Gets the maximum label width.
  ///
  /// The width is only valid after it has been calculated.
  ///
  /// \return
  ///   The maximum label width.
  /// \sa pqChartAxis::calculateMaxWidth()
  int getMaxWidth() const {return this->WidthMax;}

  /// \brief
  ///   Sets the neighboring axes if any.
  ///
  /// The neighboring axes are used when laying out the axis. The
  /// axis pixel positions are adjusted to accound for the space
  /// requirements of its neighbors.
  ///
  /// \param atMin The axis at the minimum value end.
  /// \param atMax The axis at the maximum value end.
  void setNeigbors(const pqChartAxis *atMin,
      const pqChartAxis *atMax);

  /// Returns the width (for vertical axes) or height (for horizontal axes) of this axis.  Used to simplify layout for neighbors
  const int getLayoutThickness() const;

  /// \brief
  ///   Used to layout the axis.
  ///
  /// The axis will be layed out in the drawing area based on the
  /// axis parameters such as font and location. If the axis layout
  /// type is \c BestInterval, The interval will also be calculated.
  ///
  /// \param area The rectangle for the entire drawing area.
  /// \sa pqChartAxis::calculateInterval(),
  ///     pqChartAxis::calculateFixedLayout()
  void layoutAxis(const QRect &area);

  /// \brief
  ///   Gets the skip interval for a fixed interval layout.
  /// \return
  ///   The skip interval for a fixed interval layout.
  int getSkipInterval() const {return this->Skip;}

  /// \brief
  ///   Gets the number of intervals showing for a fixed interval
  ///   layout.
  /// \return
  ///   The number of intervals showing for a fixed interval layout.
  int getNumberShowing() const {return this->Count;}
  //@}

  /// \name Drawing Methods
  //@{
  /// \brief
  ///   Draws the chart axis.
  ///
  /// If the \c grid or \c gridColor parameters are null, the axis
  /// grid will not be drawn. This can override the grid visible
  /// setting.
  ///
  /// \param p The painter to use.
  /// \param area The area that needs to be repainted.
  /// \sa pqChartAxis::drawAxisLine(QPainter *)
  void drawAxis(QPainter *p, const QRect &area);

  /// \brief
  ///   Draws just the axis line.
  ///
  /// The axis color should already be set in the painter before
  /// calling this method. This method is used mainly to ensure the
  /// axis line is on top. When drawing multiple axes, the grid
  /// lines can overwrite an adjacent axis.
  ///
  /// \param p The painter to draw the axis with.
  void drawAxisLine(QPainter *p);
  //@}

  /// \brief
  ///   Creates a lighter color from the given color.
  ///
  /// The \c QColor::light method does not work for black. This
  /// function uses a 3D equation in rgb space to compute the
  /// lighter color, which works for all colors including black.
  /// the factor determines how light the new color will be. The
  /// factor is used to find the point between the current color
  /// and white.
  ///
  /// \param color The starting color.
  /// \param factor A percentage (0.0 to 1.0) of the distance from
  ///   the given color to white.
  /// \return
  ///   The new lighter color.
  static QColor lighter(const QColor color, float factor=0.7);

signals:
  /// \brief
  ///   Called when the axis needs to be layed out again.
  void layoutNeeded();

  /// \brief
  ///   Called when the axis needs to be repainted.
  void repaintNeeded();

private:
  /// \brief
  ///   Determines the maximum label width for the axis.
  ///
  /// The maximum label width is determined using the font
  /// metrics for the longest length label. The minimum and
  /// maximum are used to fing the longest length. To account
  /// for non-fixed pitch fonts, a string of 8's is used to
  /// get the font width.
  void calculateMaxWidth();

  /// \brief
  ///   Used to calculate a suitable linear interval for the axis.
  ///
  /// The suitable interval layout method uses a list of
  /// predetermined intervals that are mapped to the value range
  /// based on the label precision. The interval will always leave
  /// a bit of padding for the value range. This helps the chart
  /// look better and ensures the data can be displayed.
  void calculateInterval();

  /// \brief
  ///   Used to calculate suitable log intervals for the axis.
  ///
  /// The suitable interval layout for a logarithmic scale axis will
  /// always pad the min and max to a power of ten.
  void calculateLogInterval();

  /// \brief
  ///   Used to set up a fixed interval layout for the axis.
  ///
  /// If the drawing area is too small to display all the intervals,
  /// only a portion of the intervals will be shown. In that case
  /// the \c getNumberShowing method will return a number less
  /// than the total. If the drawing area is too small to show all
  /// the labels without overlapping them, the skip interval will
  /// be set. The tick mark for the intervals without a label will
  /// be drawn shorter than those with a label.
  ///
  /// \sa pqChartAxis::setNumberOfIntervals(int)
  void calculateFixedLayout();

  /// Cleans up the allocated axis tick mark data.
  void cleanData();

public:
  QRect Bounds;             ///< Stores the axis bounding rectangle.

private:
  AxisLocation Location;    ///< Stores the location of the axis.
  AxisScale Scale;          ///< Stores the axis type.
  AxisLayout Layout;        ///< Stores the axis layout type.
  AxisGridColor GridType;   ///< Stores the grid color type.
  QColor AxisColor;         ///< Stores the axis color.
  QColor GridColor;         ///< Stores the grid color.
  QColor TickLabelColor;    ///< Stores the color for the axis tick labels.
  QFont TickLabelFont;      ///< Stores the font for the axis tick labels.
  pqChartLabel* Label;      ///< Stores the axis label
  pqChartAxisData *Data;    ///< Used to draw the axis and grid.
  const pqChartAxis *AtMin; ///< A pointer to the axis at the minimum.
  const pqChartAxis *AtMax; ///< A pointer to the axis at the maximum.
  pqChartValue ValueMin;    ///< Stores the minimum value.
  pqChartValue ValueMax;    ///< Stores the maximum value.
  pqChartValue TrueMin;     ///< Stores the true minimum value.
  pqChartValue TrueMax;     ///< Stores the true maximum value.
  int Intervals;            ///< Stores the number of intervals.
  int PixelMin;             ///< Stores the minimum pixel.
  int PixelMax;             ///< Stores the maximum pixel.
  int Precision;            ///< Stores axis label precision.
  int WidthMax;             ///< Stores the maximum label width.
  int Count;                ///< Stores the number of intervals showing.
  int Skip;                 ///< Used when drawing small tick marks.
  bool Visible;             ///< True if the axis should be drawn.
  bool GridVisible;         ///< True if the axis grid should be drawn.
  bool ExtraMaxPadding;     ///< Used for best interval layout.
  bool ExtraMinPadding;     ///< Used for best interval layout.

  static double MinLogValue; ///< Stores the log scale minimum.
};

#endif
