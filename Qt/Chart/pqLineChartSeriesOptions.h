/*=========================================================================

   Program: ParaView
   Module:    pqLineChartSeriesOptions.h

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

/// \file pqLineChartSeriesOptions.h
/// \date 9/18/2006

#ifndef _pqLineChartSeriesOptions_h
#define _pqLineChartSeriesOptions_h


#include "QtChartExport.h"
#include <QObject>

class pqLineChartSeriesOptionsInternal;
class pqPointMarker;
class QBrush;
class QPainter;
class QPen;


/// \class pqLineChartSeriesOptions
/// \brief
///   The pqLineChartSeriesOptions class stores the drawing options for
///   a line chart plot.
class QTCHART_EXPORT pqLineChartSeriesOptions : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a line chart plot options object.
  /// \param parent The parent object.
  pqLineChartSeriesOptions(QObject *parent=0);

  /// \brief
  ///   Creates a copy of a line chart plot options object.
  /// \param other The plot options to copy.
  pqLineChartSeriesOptions(const pqLineChartSeriesOptions &other);
  virtual ~pqLineChartSeriesOptions();

  /// \brief
  ///   Gets whether or not the options depend on the point sequence.
  ///
  /// Initially, the options do not depend on the point sequence.
  ///
  /// \return
  ///   True if the options depend on the point sequence.
  bool isSequenceDependent() const;

  /// \brief
  ///   Sets whether or not the options depend on the point sequence.
  /// \param dependent True if the options should depend on the sequence.
  void setSequenceDependent(bool dependent);

  /// \brief
  ///   Gets the pen for a sequence.
  /// \param pen Used to return the sequence pen.
  /// \param sequence The index of the sequence in the series.
  void getPen(QPen &pen, int sequence=0) const;

  /// \brief
  ///   Sets the pen to use when drawing the point sequence.
  /// \param pen The pen to use when drawing the sequence.
  /// \param sequence The index of the sequence in the series.
  void setPen(const QPen &pen, int sequence=0);

  /// \brief
  ///   Gets the brush for a sequence.
  /// \param brush Used to return the sequence brush.
  /// \param sequence The index of the sequence in the series.
  void getBrush(QBrush &brush, int sequence=0) const;

  /// \brief
  ///   Sets the brush to use when filling sequence points.
  /// \param brush The brush to use when filling sequence points.
  /// \param sequence The index of the sequence in the series.
  void setBrush(const QBrush &brush, int sequence=0);

  /// \brief
  ///   Gets the point marker for a sequence.
  /// \param sequence The index of the sequence in the series.
  /// \return
  ///   A pointer to the point marker for a sequence.
  pqPointMarker *getMarker(int sequence=0) const;

  /// \brief
  ///   Sets the marker to use when drawing sequence points.
  /// \param marker The marker to use when drawing sequence points.
  /// \param sequence The index of the sequence in the series.
  void setMarker(pqPointMarker *marker, int sequence=0);

  /// \brief
  ///   Sets up the painter for drawing a point sequence.
  ///
  /// The pen and brush for a given sequence are set on the painter.
  ///
  /// \param painter The painter to set up.
  /// \param sequence The index of the sequence in the plot.
  void setupPainter(QPainter &painter, int sequence=0) const;

  /// \brief
  ///   Copies the line plot options from another options object.
  /// \param other The options to copy.
  /// \return
  ///   A reference to this object.
  pqLineChartSeriesOptions &operator=(const pqLineChartSeriesOptions &other);

signals:
  /// Emitted when the drawing options for a plot have changed.
  void optionsChanged();

private:
  pqLineChartSeriesOptionsInternal *Internal; ///< Stores the options.
};

#endif
