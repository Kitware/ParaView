/*=========================================================================

   Program: ParaView
   Module:    pqChartSeriesOptionsGenerator.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include <Qt> // Needed for return value.

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
    Spectrum = 0, ///< 8 different hues.
    Warm,         ///< 6 warm colors (red to yellow).
    Cool,         ///< 7 cool colors (green to purple).
    Blues,        ///< 7 different blues.
    WildFlower,   ///< 7 colors from blue to magenta.
    Citrus,       ///< 6 colors from green to orange.
    Custom        ///< User specified color scheme.
    };

public:
  pqChartSeriesOptionsGenerator(ColorScheme scheme=Spectrum);
  virtual ~pqChartSeriesOptionsGenerator();

  int getNumberOfColors() const;
  int getNumberOfStyles() const;

  void getColor(int index, QColor &color) const;
  Qt::PenStyle getPenStyle(int index) const;

  virtual void getSeriesColor(int index, QColor &color) const;
  virtual void getSeriesPen(int index, QPen &pen) const;

  ColorScheme getColorScheme() const {return this->Scheme;}
  void setColorScheme(ColorScheme scheme);

  void clearColors();
  void addColor(const QColor &color);
  void insertColor(int index, const QColor &color);
  void setColor(int index, const QColor &color);
  void removeColor(int index);

  void clearPenStyles();
  void addPenStyle(Qt::PenStyle style);
  void insertPenStyle(int index, Qt::PenStyle style);
  void setPenStyle(int index, Qt::PenStyle style);
  void removePenStyle(int index);

private:
  /// Stores the list of colors and pen styles.
  pqChartSeriesOptionsGeneratorInternal *Internal;
  ColorScheme Scheme; ///< Stores the current color scheme.
};

#endif
