/*=========================================================================

   Program: ParaView
   Module:    pqChartWidget.h

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

/// \file pqChartWidget.h
/// \date 11/21/2006

#ifndef _pqChartWidget_h
#define _pqChartWidget_h


#include "QtChartExport.h"
#include <QAbstractScrollArea>

#include "pqChartAxis.h" // Needed for enum

class pqChartArea;
class pqChartLegend;
class pqChartTitle;
class QGridLayout;
class QHBoxLayout;
class QPrinter;
class QString;
class QStringList;
class QVBoxLayout;


class QTCHART_EXPORT pqChartWidget : public QAbstractScrollArea
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart widget instance.
  /// \param parent The parent widget.
  pqChartWidget(QWidget *parent=0);
  virtual ~pqChartWidget();

  pqChartTitle *getTitle() const {return this->Title;}
  void setTitle(pqChartTitle *title);

  pqChartLegend *getLegend() const {return this->Legend;}
  void setLegend(pqChartLegend *legend);

  pqChartArea *getChartArea() const {return this->Charts;}

  pqChartTitle *getAxisTitle(pqChartAxis::AxisLocation axis) const;
  void setAxisTitle(pqChartAxis::AxisLocation axis, pqChartTitle *title);

  /// \brief
  ///   Gets the preferred size of the chart.
  /// \return
  ///   The preferred size of the chart.
  virtual QSize sizeHint() const;

public slots:
  void printChart(QPrinter &printer);

  void saveChart(const QStringList &files);

  void saveChart(const QString &filename);

private:
  pqChartTitle *Title;       ///< Stores the chart title.
  pqChartLegend *Legend;     ///< Stores the chart legend.
  pqChartArea *Charts;       ///< Stores the chart area.
  pqChartTitle *LeftTitle;   ///< Stores the left axis title.
  pqChartTitle *TopTitle;    ///< Stores the top axis title.
  pqChartTitle *RightTitle;  ///< Stores the right axis title.
  pqChartTitle *BottomTitle; ///< Stores the bottom axis title.
  QVBoxLayout *TitleLayout;  ///< Layout for the chart title.
  QGridLayout *LegendLayout; ///< Layout for the chart legend.
  QVBoxLayout *TopLayout;    ///< Layout for the top and bottom titles.
  QHBoxLayout *ChartLayout;  ///< Layout for the chart and other titles.
};

#endif
