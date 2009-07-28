/*=========================================================================

   Program: ParaView
   Module:    pqChartGridLayer.h

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

/// \file pqChartGridLayer.h
/// \date 2/8/2007

#ifndef _pqChartGridLayer_h
#define _pqChartGridLayer_h


#include "QtChartExport.h"
#include "pqChartLayer.h"

class pqChartAxis;
class QRect;


/// \class pqChartGridLayer
/// \brief
///   The pqChartGridLayer class draws the axis grid lines in the
///   chart area.
class QTCHART_EXPORT pqChartGridLayer : public pqChartLayer
{
public:
  /// \brief
  ///   Builds a chart grid layer.
  /// \param parent The parent object.
  pqChartGridLayer(QObject *parent=0);
  virtual ~pqChartGridLayer();

  /// \name pqChartLayer Methods
  //@{
  virtual void layoutChart(const QRect &area);
  virtual void drawChart(QPainter &painter, const QRect &area);
  virtual void setChartArea(pqChartArea *area);
  //@}

private:
  /// \brief
  ///   Draws the grid lines for the given axis.
  /// \param painter The painter to use.
  /// \param axis The chart axis to use.
  void drawAxisGrid(QPainter &painter, const pqChartAxis *axis);

private:
  QRect *Bounds;                 ///< Stores the grid bounds.
  const pqChartAxis *LeftAxis;   ///< Stores the left axis.
  const pqChartAxis *TopAxis;    ///< Stores the top axis.
  const pqChartAxis *RightAxis;  ///< Stores the right axis.
  const pqChartAxis *BottomAxis; ///< Stores the bottom axis.
};

#endif
