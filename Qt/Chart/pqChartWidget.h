/*=========================================================================

   Program: ParaView
   Module:    pqChartWidget.h

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

/// \file pqChartWidget.h
/// \date 11/21/2006

#ifndef _pqChartWidget_h
#define _pqChartWidget_h


#include "QtChartExport.h"
#include <QWidget>

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


/// \class pqChartWidget
/// \brief
///   The pqChartWidget class is a container for the chart widgets.
///
/// The main charting widget is the chart area. It holds the chart
/// layers. The title and legend widgets are arranged around the
/// chart area. A title can be added for each axis as well as an
/// overall title for the chart.
///
/// The main chart area is created and owned by the chart widget. The
/// other widgets should be created and passed in.
class QTCHART_EXPORT pqChartWidget : public QWidget
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart widget instance.
  /// \param parent The parent widget.
  pqChartWidget(QWidget *parent=0);
  virtual ~pqChartWidget();

  /// \brief
  ///   Gets the overall title for the chart.
  /// \return
  ///   A pointer to the overall title for the chart.
  pqChartTitle *getTitle() const {return this->Title;}

  /// \brief
  ///   Sets the overall title for the chart.
  /// \param title The new chart title.
  void setTitle(pqChartTitle *title);

  /// \brief
  ///   Gets the chart legend.
  /// \return
  ///   A pointer to the chart legend.
  pqChartLegend *getLegend() const {return this->Legend;}

  /// \brief
  ///   Sets the chart legend.
  /// \param legend The new chart legend.
  void setLegend(pqChartLegend *legend);

  /// \brief
  ///   Gets the main chart area.
  /// \return
  ///   A pointer to the main chart area.
  pqChartArea *getChartArea() const {return this->Charts;}

  /// \brief
  ///   Gets the title for the given axis location.
  /// \param axis The axis location on the chart.
  /// \return
  ///   A pointer to the axis title.
  pqChartTitle *getAxisTitle(pqChartAxis::AxisLocation axis) const;

  /// \brief
  ///   Sets the title for the given axis location.
  /// \param axis The axis location on the chart.
  /// \param title The new axis title.
  void setAxisTitle(pqChartAxis::AxisLocation axis, pqChartTitle *title);

  /// \brief
  ///   Gets the preferred size of the chart.
  /// \return
  ///   The preferred size of the chart.
  virtual QSize sizeHint() const;

public slots:
  /// \brief
  ///   Prints the chart using the given printer.
  /// \param printer The printer to use.
  void printChart(QPrinter &printer);

  /// \brief
  ///   Saves a screenshot of the chart to the given files.
  /// \param files The list of files to write.
  void saveChart(const QStringList &files);

  /// \brief
  ///   Saves a screenshot of the chart to the given file.
  /// \param filename The name of the file to write.
  void saveChart(const QString &filename);

signals:
  /// \brief
  ///   Emitted when a new chart title has been set.
  /// \param title The new chart title.
  void newChartTitle(pqChartTitle *title);

  /// \brief
  ///   Emitted when a new chart legend has been set.
  /// \param legend The new chart legend.
  void newChartLegend(pqChartLegend *legend);

  /// \brief
  ///   Emitted when a new axis title has been set.
  /// \param axis The axis location.
  /// \param title The new axis title.
  void newAxisTitle(pqChartAxis::AxisLocation axis, pqChartTitle *title);

private slots:
  /// Moves the legend when the location changes.
  void changeLegendLocation();

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
