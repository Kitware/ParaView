/*=========================================================================

   Program: ParaView
   Module:    pqChartSeriesOptionsGenerator.h

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

/// \file pqChartSeriesOptionsGenerator.h
/// \date 4/26/2007

#ifndef _pqChartSeriesOptionsGenerator_h
#define _pqChartSeriesOptionsGenerator_h


#include "QtChartExport.h"
#include <QObject> // Needed for return value.

class pqChartSeriesOptionsGeneratorInternal;
class QColor;
class QPen;


/// \class pqChartSeriesOptionsGenerator
/// \brief
///   The pqChartSeriesOptionsGenerator class is used to generate
///   drawing options for a chart.
class QTCHART_EXPORT pqChartSeriesOptionsGenerator
{
public:
  enum ColorScheme
    {
    Spectrum = 0, ///< 7 different hues.
    Warm,         ///< 6 warm colors (red to yellow).
    Cool,         ///< 7 cool colors (green to purple).
    Blues,        ///< 7 different blues.
    WildFlower,   ///< 7 colors from blue to magenta.
    Citrus,       ///< 6 colors from green to orange.
    Custom        ///< User specified color scheme.
    };

public:
  /// \brief
  ///   Creates an options generator instance.
  /// \param scheme The initial color scheme.
  pqChartSeriesOptionsGenerator(ColorScheme scheme=Spectrum);
  virtual ~pqChartSeriesOptionsGenerator();

  /// \brief
  ///   Gets the number of colors in the color list.
  /// \return
  ///   The number of colors in the color list.
  int getNumberOfColors() const;

  /// \brief
  ///   Gets the number of pen styles in the style list.
  /// \return
  ///   The number of pen styles in the style list.
  int getNumberOfStyles() const;

  /// \brief
  ///   Gets a color from the color list.
  ///
  /// This method provides access to the list of colors. The index
  /// must be in the color list range in order to return a valid
  /// color.
  ///
  /// \param index The list index for the color.
  /// \param color Used to return the color.
  /// \sa pqChartSeriesOptionsGenerator::getSeriesColor(int, QColor &)
  void getColor(int index, QColor &color) const;

  /// \brief
  ///   Gets a pen style from the pen styles list.
  ///
  /// This method provides access to the list of styles. If the index
  /// is out of range, a default style will be returned.
  ///
  /// \param index The list index for the style.
  /// \return
  ///   The pen style for the given index.
  /// \sa pqChartSeriesOptionsGenerator::getSeriesPen(int, QPen &)
  Qt::PenStyle getPenStyle(int index) const;

  virtual void getSeriesColor(int index, QColor &color) const;
  virtual void getSeriesPen(int index, QPen &pen) const;

  /// \brief
  ///   Gets the current color scheme.
  /// \return
  ///   The current color scheme.
  ColorScheme getColorScheme() const {return this->Scheme;}

  /// \brief
  ///   Sets the color scheme.
  ///
  /// The color scheme will automatically be changed to \c Custom if
  /// the color or style lists are modified.
  ///
  /// \param scheme The new color scheme.
  void setColorScheme(ColorScheme scheme);

  /// Clears the list of colors.
  void clearColors();

  /// \brief
  ///   Adds a color to the list of colors.
  /// \param color The new color to add.
  void addColor(const QColor &color);

  /// \brief
  ///   Inserts a new color into the list of colors.
  /// \param index Where to insert the new color.
  /// \param color The new color to insert.
  void insertColor(int index, const QColor &color);

  /// \brief
  ///   Sets the color for the given index.
  ///
  /// This method does nothing if the index is out of range.
  ///
  /// \param index Which color to modify.
  /// \param color The new color.
  void setColor(int index, const QColor &color);

  /// \brief
  ///   Removes the color for the given index.
  /// \param index Which color to remove from the list.
  void removeColor(int index);

  /// Clears the list of pen styles.
  void clearPenStyles();

  /// \brief
  ///   Adds a pen style to the list of pen style.
  /// \param style The new pen style to add.
  void addPenStyle(Qt::PenStyle style);

  /// \brief
  ///   Inserts a new pen style into the list of pen style.
  /// \param index Where to insert the new pen style.
  /// \param style The new pen style to insert.
  void insertPenStyle(int index, Qt::PenStyle style);

  /// \brief
  ///   Sets the pen style for the given index.
  ///
  /// This method does nothing if the index is out of range.
  ///
  /// \param index Which pen style to modify.
  /// \param style The new pen style.
  void setPenStyle(int index, Qt::PenStyle style);

  /// \brief
  ///   Removes the pen style for the given index.
  /// \param index Which pen style to remove from the list.
  void removePenStyle(int index);

private:
  /// Stores the list of colors and pen styles.
  pqChartSeriesOptionsGeneratorInternal *Internal;
  ColorScheme Scheme; ///< Stores the current color scheme.
};

#endif
