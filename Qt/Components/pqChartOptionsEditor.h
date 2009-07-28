/*=========================================================================

   Program: ParaView
   Module:    pqChartOptionsEditor.h

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

/// \file pqChartOptionsEditor.h
/// \date 7/20/2007

#ifndef _pqChartOptionsEditor_h
#define _pqChartOptionsEditor_h


#include "pqComponentsExport.h"
#include "pqOptionsContainer.h"
#include "vtkQtChartAxisLayer.h" // Needed for enum
#include "vtkQtChartAxis.h" // Needed for enum
#include "vtkQtChartAxisOptions.h" // Needed for enum
#include "vtkQtChartLegend.h" // Needed for enum
#include "pqChartValue.h"

class pqChartOptionsEditorForm;
class pqChartValue;
class QColor;
class QFont;
class QLabel;
class QString;
class QStringList;


/// \class pqChartOptionsEditor
/// \brief
///   The pqChartOptionsEditor class is the user interface for setting
///   the chart options.
class PQCOMPONENTS_EXPORT pqChartOptionsEditor : public pqOptionsContainer
{
  Q_OBJECT

public:
  pqChartOptionsEditor(QWidget *parent=0);
  virtual ~pqChartOptionsEditor();

  virtual bool eventFilter(QObject *object, QEvent *e);

  /// \name pqOptionsContainer Methods
  //@{
  virtual void setPage(const QString &page);
  virtual QStringList getPageList();
  //@}

  /// \name Chart Title Parameters
  //@{
  void getTitle(QString &title) const;
  void setTitle(const QString &title);
  const QFont &getTitleFont() const;
  void setTitleFont(const QFont &newFont);
  void getTitleColor(QColor &color) const;
  void setTitleColor(const QColor &color);
  int getTitleAlignment() const;
  void setTitleAlignment(int alignment);
  //@}

  /// \name Chart Legend Parameters
  //@{
  bool isLegendShowing() const;
  void setLegendShowing(bool legendShowing);
  vtkQtChartLegend::LegendLocation getLegendLocation() const;
  void setLegendLocation(vtkQtChartLegend::LegendLocation location);
  vtkQtChartLegend::ItemFlow getLegendFlow() const;
  void setLegendFlow(vtkQtChartLegend::ItemFlow flow);
  //@}

  /// \name General Axis Parameters
  //@{
  bool isAxisShowing(vtkQtChartAxis::AxisLocation location) const;
  void setAxisShowing(vtkQtChartAxis::AxisLocation location, bool axisShowing);
  bool isAxisGridShowing(vtkQtChartAxis::AxisLocation location) const;
  void setAxisGridShowing(vtkQtChartAxis::AxisLocation location,
      bool gridShowing);
  vtkQtChartAxisOptions::AxisGridColor getAxisGridType(
      vtkQtChartAxis::AxisLocation location) const;
  void setAxisGridType(vtkQtChartAxis::AxisLocation location,
      vtkQtChartAxisOptions::AxisGridColor color);
  const QColor &getAxisColor(vtkQtChartAxis::AxisLocation location) const;
  void setAxisColor(vtkQtChartAxis::AxisLocation location, const QColor &color);
  const QColor &getAxisGridColor(vtkQtChartAxis::AxisLocation location) const;
  void setAxisGridColor(vtkQtChartAxis::AxisLocation location,
      const QColor &color);
  //@}

  /// \name Axis Label Parameters
  //@{
  bool areAxisLabelsShowing(vtkQtChartAxis::AxisLocation location) const;
  void setAxisLabelsShowing(vtkQtChartAxis::AxisLocation location,
      bool labelsShowing);
  const QFont &getAxisLabelFont(vtkQtChartAxis::AxisLocation location) const;
  void setAxisLabelFont(vtkQtChartAxis::AxisLocation location,
      const QFont &newFont);
  const QColor &getAxisLabelColor(vtkQtChartAxis::AxisLocation location) const;
  void setAxisLabelColor(vtkQtChartAxis::AxisLocation location,
      const QColor &color);
  pqChartValue::NotationType getAxisLabelNotation(
      vtkQtChartAxis::AxisLocation location) const;
  void setAxisLabelNotation(vtkQtChartAxis::AxisLocation location,
      pqChartValue::NotationType notation);
  int getAxisLabelPrecision(vtkQtChartAxis::AxisLocation location) const;
  void setAxisLabelPrecision(vtkQtChartAxis::AxisLocation location,
      int precision);
  //@}

  /// \name Axis Layout Parameters
  //@{
  bool isUsingLogScale(vtkQtChartAxis::AxisLocation location) const;
  void setAxisScale(vtkQtChartAxis::AxisLocation location, bool useLogScale);
  vtkQtChartAxisLayer::AxisBehavior getAxisBehavior(
      vtkQtChartAxis::AxisLocation location) const;
  void setAxisBehavior(vtkQtChartAxis::AxisLocation location,
      vtkQtChartAxisLayer::AxisBehavior behavior);
  void getAxisMinimum(vtkQtChartAxis::AxisLocation location,
      pqChartValue &minimum) const;
  void setAxisMinimum(vtkQtChartAxis::AxisLocation location,
      const pqChartValue &minimum);
  void getAxisMaximum(vtkQtChartAxis::AxisLocation location,
      pqChartValue &maximum) const;
  void setAxisMaximum(vtkQtChartAxis::AxisLocation location,
      const pqChartValue &maximum);
  void getAxisLabels(vtkQtChartAxis::AxisLocation location,
      QStringList &list) const;
  void setAxisLabels(vtkQtChartAxis::AxisLocation location,
      const QStringList &list);
  //@}

  /// \name Axis Title Parameters
  //@{
  const QString &getAxisTitle(vtkQtChartAxis::AxisLocation location) const;
  void setAxisTitle(vtkQtChartAxis::AxisLocation location, const QString &title);
  const QFont &getAxisTitleFont(vtkQtChartAxis::AxisLocation location) const;
  void setAxisTitleFont(vtkQtChartAxis::AxisLocation location,
      const QFont &newFont);
  const QColor &getAxisTitleColor(vtkQtChartAxis::AxisLocation location) const;
  void setAxisTitleColor(vtkQtChartAxis::AxisLocation location,
      const QColor &color);
  int getAxisTitleAlignment(vtkQtChartAxis::AxisLocation location) const;
  void setAxisTitleAlignment(vtkQtChartAxis::AxisLocation location,
      int alignment);
  //@}

signals:
  /// \name Chart Title Changes
  //@{
  void titleChanged(const QString &title);
  void titleFontChanged(const QFont &newFont);
  void titleColorChanged(const QColor &color);
  void titleAlignmentChanged(int alignment);
  //@}

  /// \name Chart Legend Changes
  //@{
  void showLegendChanged(bool legendShowing);
  void legendLocationChanged(vtkQtChartLegend::LegendLocation location);
  void legendFlowChanged(vtkQtChartLegend::ItemFlow flow);
  //@}

  /// \name General Axis Changes
  //@{
  void showAxisChanged(vtkQtChartAxis::AxisLocation location, bool axisShowing);
  void showAxisGridChanged(vtkQtChartAxis::AxisLocation location,
      bool gridShowing);
  void axisGridTypeChanged(vtkQtChartAxis::AxisLocation location,
      vtkQtChartAxisOptions::AxisGridColor color);
  void axisColorChanged(vtkQtChartAxis::AxisLocation location,
      const QColor &color);
  void axisGridColorChanged(vtkQtChartAxis::AxisLocation location,
      const QColor &color);
  //@}

  /// \name Axis Label Changes
  //@{
  void showAxisLabelsChanged(vtkQtChartAxis::AxisLocation location,
      bool labelsShowing);
  void axisLabelFontChanged(vtkQtChartAxis::AxisLocation location,
      const QFont &newFont);
  void axisLabelColorChanged(vtkQtChartAxis::AxisLocation location,
      const QColor &color);
  void axisLabelNotationChanged(vtkQtChartAxis::AxisLocation location,
      pqChartValue::NotationType notation);
  void axisLabelPrecisionChanged(vtkQtChartAxis::AxisLocation location,
      int precision);
  //@}

  /// \name Axis Layout Changes
  //@{
  void axisScaleChanged(vtkQtChartAxis::AxisLocation location, bool useLogScale);
  void axisBehaviorChanged(vtkQtChartAxis::AxisLocation location,
      vtkQtChartAxisLayer::AxisBehavior behavior);
  void axisMinimumChanged(vtkQtChartAxis::AxisLocation location,
      const pqChartValue &minimum);
  void axisMaximumChanged(vtkQtChartAxis::AxisLocation location,
      const pqChartValue &maximum);
  void axisLabelsChanged(vtkQtChartAxis::AxisLocation location,
      const QStringList &list);
  //@}

  /// \name Axis Title Changes
  //@{
  void axisTitleChanged(vtkQtChartAxis::AxisLocation location,
      const QString &title);
  void axisTitleFontChanged(vtkQtChartAxis::AxisLocation location,
      const QFont &newFont);
  void axisTitleColorChanged(vtkQtChartAxis::AxisLocation location,
      const QColor &color);
  void axisTitleAlignmentChanged(vtkQtChartAxis::AxisLocation location,
      int alignment);
  //@}

private slots:
  void pickTitleFont();
  void convertLegendLocation(int index);
  void convertLegendFlow(int index);
  void setAxisShowing(bool axisShowing);
  void setAxisGridShowing(bool gridShowing);
  void setGridColorType(int index);
  void setAxisColor(const QColor &color);
  void setGridColor(const QColor &color);
  void setAxisLabelsShowing(bool labelsShowing);
  void pickAxisLabelFont();
  void setLabelColor(const QColor &color);
  void setLabelNotation(int index);
  void setLabelPrecision(int precision);
  void setUsingLogScale(bool usingLogScale);
  void changeLayoutPage(bool checked);
  void setAxisMinimum(const QString &text);
  void setAxisMaximum(const QString &text);
  void addAxisLabel();
  void updateAxisLabels();
  void updateRemoveButton();
  void removeSelectedLabels();
  void showRangeDialog();
  void generateAxisLabels();
  void setAxisTitle(const QString &text);
  void pickAxisTitleFont();
  void setAxisTitleColor(const QColor &color);
  void setAxisTitleAlignment(int alignment);

private:
  void loadAxisPage();
  void loadAxisLayoutPage();
  void loadAxisTitlePage();
  void updateDescription(QLabel *label, const QFont &newFont);

private:
  pqChartOptionsEditorForm *Form; ///< Stores the UI data.
};

#endif
