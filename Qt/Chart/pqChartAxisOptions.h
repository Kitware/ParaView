/*=========================================================================

   Program: ParaView
   Module:    pqChartAxisOptions.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

/// \file pqChartAxisOptions.h
/// \date 1/31/2007

#ifndef _pqChartAxisOptions_h
#define _pqChartAxisOptions_h


#include "QtChartExport.h"
#include <QObject>

#include "pqChartValue.h" // Needed for enum
#include <QColor> // Needed for member variable
#include <QFont>  // Needed for member variable


/// \class pqChartAxisOptions
/// \brief
///   The pqChartAxisOptions class stores the drawing options for a
///   chart axis.
class QTCHART_EXPORT pqChartAxisOptions : public QObject
{
  Q_OBJECT

public:
  enum AxisGridColor {
    Lighter = 0, ///< The grid color is based on the axis color.
    Specified    ///< The grid color is specified.
  };

public:
  /// \brief
  ///   Creates a chart axis options instance.
  /// \param parent The parent object.
  pqChartAxisOptions(QObject *parent=0);

  /// \brief
  ///   Makes a copy of another axis options instance.
  /// \param other The axis options to copy.
  pqChartAxisOptions(const pqChartAxisOptions &other);
  virtual ~pqChartAxisOptions() {}

  /// \brief
  ///   Gets whether or not the axis is visible.
  /// \return
  ///   True if the axis is visible.
  bool isVisible() const {return this->Visible;}

  /// \brief
  ///   Sets whether or not the axis should be visible.
  /// \param visible True if the axis should be visible.
  void setVisible(bool visible);

  /// \brief
  ///   Gets whether or not the axis labels are visible.
  /// \return
  ///   True if the axis labels are visible.
  bool areLabelsVisible() const {return this->ShowLabels;}

  /// \brief
  ///   Sets whether or not the axis labels should be visible.
  /// \param visible True if the axis labels should be visible.
  void setLabelsVisible(bool visible);

  /// \brief
  ///   Gets whether or not the axis grid is visible.
  /// \return
  ///   True if the axis grid is visible.
  bool isGridVisible() const {return this->ShowGrid;}

  /// \brief
  ///   Sets whether or not the axis grid should be visible.
  /// \param visible True if the axis grid should be visible.
  void setGridVisible(bool visible);

  /// \brief
  ///   Gets the axis color.
  /// \return
  ///   The axis color.
  const QColor &getAxisColor() const {return this->AxisColor;}

  /// \brief
  ///   Sets the axis color.
  ///
  /// If the grid color is tied to the axis color, the grid
  /// color will also be set.
  ///
  /// \param color The new axis color.
  /// \sa pqChartAxisOptions::setGridColorType(AxisGridColor)
  void setAxisColor(const QColor &color);

  /// \brief
  ///   Gets the color of the axis labels.
  /// \return
  ///   The color of the axis labels.
  const QColor &getLabelColor() const {return this->LabelColor;}

  /// \brief
  ///   Sets the color of the axis labels.
  /// \param color The new axis label color.
  void setLabelColor(const QColor &color);

  /// \brief
  ///   Gets the font used to draw the axis labels.
  /// \return
  ///   The font used to draw the axis labels.
  const QFont &getLabelFont() const {return this->LabelFont;}

  /// \brief
  ///   Sets the font used to draw the axis labels.
  /// \param font The font to use.
  void setLabelFont(const QFont &font);

  /// \brief
  ///   Gets the decimal precision of the axis labels.
  /// \return
  ///   The decimal precision of the axis labels.
  /// \sa pqChartValue::getString()
  int getPrecision() const {return this->Precision;}

  /// \brief
  ///   Sets the decimal precision of the axis labels.
  /// \param precision The number of decimal places to use.
  /// \sa pqChartValue::getString()
  void setPrecision(int precision);

  /// \brief
  ///   Gets the notation type for the axis labels.
  /// \return
  ///   The notation type for the axis labels.
  /// \sa pqChartValue::getString()
  pqChartValue::NotationType getNotation() const {return this->Notation;}

  /// \brief
  ///   Sets the notation type for the axis labels.
  /// \param notation The new axis notation type.
  /// \sa pqChartValue::getString()
  void setNotation(pqChartValue::NotationType notation);

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
  /// \sa pqChartAxisOptions::setGridColorType(AxisGridColor)
  AxisGridColor getGridColorType() const {return this->GridType;}

  /// \brief
  ///   Gets the axis grid color.
  ///
  /// If the grid color type is \c Lighter, the color returned will be
  /// a lighter version of the axis color. Otherwise, the specified
  /// color will be returned.
  ///
  /// \return
  ///   The axis grid color.
  /// \sa pqChartAxisOptions::setGridColorType(AxisGridColor)
  QColor getGridColor() const;

  /// \brief
  ///   Sets the axis grid color.
  ///
  /// If the axis grid color type is \c Lighter, calling this method
  /// will not change the color used for drawing the grid. It will
  /// still set the specified grid color in case the type changes.
  ///
  /// \param color The new axis grid color.
  void setGridColor(const QColor &color);

  /// \brief
  ///   Makes a copy of another axis options instance.
  /// \param other The axis options to copy.
  /// \return
  ///   A reference to the object being assigned.
  pqChartAxisOptions &operator=(const pqChartAxisOptions &other);

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
  /// Emitted when the axis or label visibility changes.
  void visibilityChanged();

  /// Emitted when the axis or label color changes.
  void colorChanged();

  /// Emitted when the label font changes.
  void fontChanged();

  /// Emitted when the precision or notation changes.
  void presentationChanged();

  /// Emitted when the grid color or visibility changes.
  void gridChanged();

private:
  /// Stores the axis label notation type.
  pqChartValue::NotationType Notation;

  /// Stores the grid color type (lighter or specified).
  AxisGridColor GridType;

  QColor AxisColor;  ///< Stores the axis color.
  QColor GridColor;  ///< Stores the specified grid color.
  QColor LabelColor; ///< Stores the color for the axis labels.
  QFont LabelFont;   ///< Stores the font for the axis labels.
  int Precision;     ///< Stores axis label precision.
  bool Visible;      ///< True if the axis should be drawn.
  bool ShowLabels;   ///< True if the labels should be drawn.
  bool ShowGrid;     ///< True if the grid should be drawn.
};

#endif
