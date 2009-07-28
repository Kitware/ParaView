/*=========================================================================

   Program: ParaView
   Module:    pqLineChartWidget.h

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

/// \file pqLineChartWidget.h
/// \date 9/27/2005

#ifndef _pqLineChartWidget_h
#define _pqLineChartWidget_h


#include "QtChartExport.h"

class pqChartWidget;
class pqLineChart;
class QWidget;

/// \class pqLineChartWidget
/// \brief
///   The pqLineChartWidget class is used to set up an interactive
///   line chart.
class QTCHART_EXPORT pqLineChartWidget
{
public:
  pqLineChartWidget() {}
  ~pqLineChartWidget() {}

  /// \brief
  ///   Creates a chart widget with a line chart layer.
  /// \param parent The parent of the chart widget.
  /// \param lineChart Used to return the line chart layer.
  /// \return
  ///   A pointer to a newly allocated chart widget.
  static pqChartWidget *createLineChart(QWidget *parent=0,
      pqLineChart **lineChart=0);
};

#endif
