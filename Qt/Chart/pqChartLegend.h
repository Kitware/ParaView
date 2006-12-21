/*=========================================================================

   Program: ParaView
   Module:    pqChartLegend.h

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

#ifndef _pqChartLegend_h
#define _pqChartLegend_h

#include "QtChartExport.h"

#include <QObject>
#include <QRect>

class QPainter;
class pqChartLabel;
class pqMarkerPen;

/// Encapsulates a legend "box" that identifies data displayed in a chart (e.g. individual line plots)
class QTCHART_EXPORT pqChartLegend :
  public QObject
{
  Q_OBJECT

public:
  pqChartLegend(QObject* parent = 0);
  ~pqChartLegend();

  /// Removes all entries from the legend
  void clear();
  /// Adds a line-plot entry to the legend (pqChartLegend takes ownership of the pen and label objects)
  void addEntry(pqMarkerPen* pen, pqChartLabel* label);

  /// Returns the legend's preferred size, based on font and orientation
  const QRect getSizeRequest();
  /// Sets the bounds within which the legend will be drawn
  void setBounds(const QRect& bounds);

  /// Renders the legend using the given painter and the stored legend bounds 
  void draw(QPainter& painter, const QRect& area);

signals:
  /// Called when the legend needs to be layed-out again.
  void layoutNeeded();

  /// Called when the legend needs to be repainted.
  void repaintNeeded();

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
