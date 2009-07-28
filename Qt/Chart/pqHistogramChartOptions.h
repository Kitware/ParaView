/*=========================================================================

   Program: ParaView
   Module:    pqHistogramChartOptions.h

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

/// \file pqHistogramChartOptions.h
/// \date 2/14/2007

#ifndef _pqHistogramChartOptions_h
#define _pqHistogramChartOptions_h


#include "QtChartExport.h"
#include <QObject>

#include "pqHistogramColor.h" // Needed for static member
#include <QColor> // Needed for member


/// \class pqHistogramChartOptions
/// \brief
///   The pqHistogramChartOptions class stores the drawing options for
///   a histogram chart.
///
/// The default settings are as follows:
///   \li highlight style: \c Fill
///   \li outline style: \c Darker
///   \li selection background: \c LightBlue
class QTCHART_EXPORT pqHistogramChartOptions : public QObject
{
  Q_OBJECT

public:
  enum OutlineStyle
    {
    Darker, ///< Draws the bin outline in a darker color.
    Black   ///< Draws a black bin outline.
    };

  enum HighlightStyle
    {
    Outline, ///< The bin outline is highlighted.
    Fill     ///< The bin interior is highlighted.
    };

public:
  /// \brief
  ///   Creates a histogram chart options instance.
  /// \param parent The parent object.
  pqHistogramChartOptions(QObject *parent=0);

  /// \brief
  ///   Makes a copy of another histogram options instance.
  /// \param other The histogram options to copy.
  pqHistogramChartOptions(const pqHistogramChartOptions &other);
  virtual ~pqHistogramChartOptions() {}

  /// \brief
  ///   Gets the highlight style for the histogram bars.
  /// \return
  ///   The current highlight style.
  HighlightStyle getHighlightStyle() const {return this->Style;}

  /// \brief
  ///   Sets the highlight style for the histogram bars.
  ///
  /// The default style is \c Fill.
  ///
  /// \param style The highlight style to use.
  void setHighlightStyle(HighlightStyle style);

  /// \brief
  ///   Gets the outline style for the histogram bars.
  /// \return
  ///   The current outline style.
  OutlineStyle getOutlineStyle() const {return this->OutlineType;}

  /// \brief
  ///   Sets the outline style for the histogram bars.
  ///
  /// The default style is \c Darker.
  ///
  /// \param style The outline style to use.
  void setBinOutlineStyle(OutlineStyle style);

  /// \brief
  ///   Gets the highlight background color.
  /// \return
  ///   The current highlight background color.
  const QColor &getHighlightColor() const {return this->Highlight;}

  /// \brief
  ///   Sets the highlight background color.
  /// \param color The color for the highlight background.
  void setHighlightColor(const QColor &color);

  /// \brief
  ///   Gets the histogram color scheme.
  /// \return
  ///   A pointer to the histogram color scheme.
  pqHistogramColor *getColorScheme() const {return this->Colors;}

  /// \brief
  ///   Sets the histogram color scheme.
  /// \param scheme The color scheme interface to use.
  void setColorScheme(pqHistogramColor *scheme);

  /// \brief
  ///   Makes a copy of another histogram options instance.
  /// \param other The histogram options to copy.
  /// \return
  ///   A reference to the object being assigned.
  pqHistogramChartOptions &operator=(const pqHistogramChartOptions &other);

signals:
  /// Emitted when any of the histogram options have changed.
  void optionsChanged();

public:
  /// Defines the default highlight background.
  static const QColor LightBlue;

private:
  HighlightStyle Style;     ///< Stores the highlight style.
  OutlineStyle OutlineType; ///< Stores the outline style.
  QColor Highlight;         ///< Stores the highlight background color.
  pqHistogramColor *Colors; ///< A pointer to the bar color scheme.

  /// Stores the default color scheme.
  static pqHistogramColor ColorScheme;
};

#endif
