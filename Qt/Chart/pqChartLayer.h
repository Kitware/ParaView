/*=========================================================================

   Program: ParaView
   Module:    pqChartLayer.h

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

/// \file pqChartLayer.h
/// \date 11/7/2006

#ifndef _pqChartLayer_h
#define _pqChartLayer_h


#include "QtChartExport.h"
#include <QObject>

class pqChartArea;
class pqChartAxis;
class pqChartContentsSpace;
class pqChartValue;
class QPainter;
class QRect;


/// \class pqChartLayer
/// \brief
///   The pqChartLayer class is the base class for all chart layers.
class QTCHART_EXPORT pqChartLayer : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Builds a chart layer object.
  /// \param parent The parent object.
  pqChartLayer(QObject *parent=0);
  virtual ~pqChartLayer() {}

  /// \brief
  ///   Gets the chart's value range for the given axis.
  /// \param axis Which axis to get the range for.
  /// \param min Used to return the minimum value.
  /// \param max Used to return the maximum value.
  /// \param padMin Used to return the min padding preference.
  /// \param padMax Used to return the max padding preference.
  /// \return
  ///   False if the chart does not use the given axis.
  virtual bool getAxisRange(const pqChartAxis *axis, pqChartValue &min,
      pqChartValue &max, bool &padMin, bool &padMax) const;

  /// \brief
  ///   Gets whether or not the chart prefers to control the axis
  ///   labels for the given axis.
  /// \param axis The axis in question.
  /// \return
  ///   True if the chart prefers to control the axis labels.
  virtual bool isAxisControlPreferred(const pqChartAxis *axis) const;

  /// \brief
  ///   Used to generate the axis labels for an axis.
  ///
  /// This method is only called if the \c isAxisControlPreferred
  /// method returns true. This method is called during the chart area
  /// layout before the axis layout is called.
  ///
  /// \param axis The axis to create labels for.
  virtual void generateAxisLabels(pqChartAxis *axis);

  /// \brief
  ///   Used to layout the chart layer.
  ///
  /// The chart axes must be layed out before this method is called.
  /// This method must be called before the chart can be drawn.
  ///
  /// \param area The area the chart should occupy.
  /// \sa pqChartLayer::drawChart(QPainter &, const QRect &)
  virtual void layoutChart(const QRect &area)=0;

  /// \brief
  ///   Draws the background for the chart.
  ///
  /// This method is used to draw on the chart background behind all
  /// the layers.
  ///
  /// \param painter The painter to use.
  /// \param area The area that needs to be painted.
  /// \sa pqChartLayer::drawChart(QPainter &, const QRect &)
  virtual void drawBackground(QPainter &painter, const QRect &area);

  /// \brief
  ///   Used to draw the chart layer.
  ///
  /// The chart needs to be layed out before it can be drawn.
  /// Separating the layout and drawing functions improves the
  /// repainting performance.
  ///
  /// \param painter The painter to use.
  /// \param area The area that needs to be painted.
  /// \sa pqChartLayer::layoutChart(const QRect &)
  virtual void drawChart(QPainter &painter, const QRect &area)=0;

  /// \brief
  ///   Gets the chart area containing the layer.
  /// \return
  ///   A pointer to the chart area.
  pqChartArea *getChartArea() const {return this->ChartArea;}

  /// \brief
  ///   Sets the chart area that contains the layer.
  /// \note
  ///   The chart area will call this method when the layer is added.
  /// \param area The chart area.
  virtual void setChartArea(pqChartArea *area) {this->ChartArea = area;}

  /// \brief
  ///   Gets the layer's contents space object.
  /// \return
  ///   A pointer to the layer's contents space object.
  pqChartContentsSpace *getContentsSpace() const;

signals:
  /// Emitted when the layer layout needs to be calculated.
  void layoutNeeded();

  /// Emitted when the layer needs to be repainted.
  void repaintNeeded();

  /// \brief
  ///   Emitted when the axis range for the layer has changed.
  /// \note
  ///   This signal should be emitted before the \c layoutNeeded
  ///   signal to be effective.
  void rangeChanged();

private:
  pqChartArea *ChartArea; ///< Stores the chart area.
};

#endif
