/*=========================================================================

   Program: ParaView
   Module:    pqChartOptionsEditor.h

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

/// \file pqChartOptionsEditor.h
/// \date 7/20/2007

#ifndef _pqChartOptionsEditor_h
#define _pqChartOptionsEditor_h


#include "pqComponentsExport.h"
#include "pqOptionsContainer.h"
#include "pqChartArea.h" // Needed for enum
#include "pqChartAxis.h" // Needed for enum
#include "pqChartAxisOptions.h" // Needed for enum
#include "pqChartLegend.h" // Needed for enum

class pqChartOptionsEditorForm;
class pqChartValue;
class QColor;
class QFont;
class QLabel;
class QString;
class QStringList;


class PQCOMPONENTS_EXPORT pqChartOptionsEditor : public pqOptionsContainer
{
  Q_OBJECT

public:
  pqChartOptionsEditor(QWidget *parent=0);
  virtual ~pqChartOptionsEditor();

  /// \name pqOptionsContainer Methods
  //@{
  virtual void setPage(const QString &page);
  virtual void getPageList(QStringList &pages);
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
  pqChartLegend::LegendLocation getLegendLocation() const;
  void setLegendLocation(pqChartLegend::LegendLocation location);
  pqChartLegend::ItemFlow getLegendFlow() const;
  void setLegendFlow(pqChartLegend::ItemFlow flow);
  //@}

  /// \name General Axis Parameters
  //@{
  bool isAxisShowing(pqChartAxis::AxisLocation location) const;
  void setAxisShowing(pqChartAxis::AxisLocation location, bool axisShowing);
  bool isAxisGridShowing(pqChartAxis::AxisLocation location) const;
  void setAxisGridShowing(pqChartAxis::AxisLocation location,
      bool gridShowing);
  pqChartAxisOptions::AxisGridColor getAxisGridType(
      pqChartAxis::AxisLocation location) const;
  void setAxisGridType(pqChartAxis::AxisLocation location,
      pqChartAxisOptions::AxisGridColor color);
  const QColor &getAxisColor(pqChartAxis::AxisLocation location) const;
  void setAxisColor(pqChartAxis::AxisLocation location, const QColor &color);
  const QColor &getAxisGridColor(pqChartAxis::AxisLocation location) const;
  void setAxisGridColor(pqChartAxis::AxisLocation location,
      const QColor &color);
  //@}

  /// \name Axis Label Parameters
  //@{
  bool areAxisLabelsShowing(pqChartAxis::AxisLocation location) const;
  void setAxisLabelsShowing(pqChartAxis::AxisLocation location,
      bool labelsShowing);
  const QFont &getAxisLabelFont(pqChartAxis::AxisLocation location) const;
  void setAxisLabelFont(pqChartAxis::AxisLocation location,
      const QFont &newFont);
  const QColor &getAxisLabelColor(pqChartAxis::AxisLocation location) const;
  void setAxisLabelColor(pqChartAxis::AxisLocation location,
      const QColor &color);
  pqChartValue::NotationType getAxisLabelNotation(
      pqChartAxis::AxisLocation location) const;
  void setAxisLabelNotation(pqChartAxis::AxisLocation location,
      pqChartValue::NotationType notation);
  int getAxisLabelPrecision(pqChartAxis::AxisLocation location) const;
  void setAxisLabelPrecision(pqChartAxis::AxisLocation location,
      int precision);
  //@}

  /// \name Axis Layout Parameters
  //@{
  bool isUsingLogScale(pqChartAxis::AxisLocation location) const;
  void setAxisScale(pqChartAxis::AxisLocation location, bool useLogScale);
  pqChartArea::AxisBehavior getAxisBehavior(
      pqChartAxis::AxisLocation location) const;
  void setAxisBehavior(pqChartAxis::AxisLocation location,
      pqChartArea::AxisBehavior behavior);
  void getAxisMinimum(pqChartAxis::AxisLocation location,
      pqChartValue &minimum) const;
  void setAxisMinimum(pqChartAxis::AxisLocation location,
      const pqChartValue &minimum);
  void getAxisMaximum(pqChartAxis::AxisLocation location,
      pqChartValue &maximum) const;
  void setAxisMaximum(pqChartAxis::AxisLocation location,
      const pqChartValue &maximum);
  void getAxisLabels(pqChartAxis::AxisLocation location,
      QStringList &list) const;
  void setAxisLabels(pqChartAxis::AxisLocation location,
      const QStringList &list);
  //@}

  /// \name Axis Title Parameters
  //@{
  const QString &getAxisTitle(pqChartAxis::AxisLocation location) const;
  void setAxisTitle(pqChartAxis::AxisLocation location, const QString &title);
  const QFont &getAxisTitleFont(pqChartAxis::AxisLocation location) const;
  void setAxisTitleFont(pqChartAxis::AxisLocation location,
      const QFont &newFont);
  const QColor &getAxisTitleColor(pqChartAxis::AxisLocation location) const;
  void setAxisTitleColor(pqChartAxis::AxisLocation location,
      const QColor &color);
  int getAxisTitleAlignment(pqChartAxis::AxisLocation location) const;
  void setAxisTitleAlignment(pqChartAxis::AxisLocation location,
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
  void legendLocationChanged(pqChartLegend::LegendLocation location);
  void legendFlowChanged(pqChartLegend::ItemFlow flow);
  //@}

  /// \name General Axis Changes
  //@{
  void showAxisChanged(pqChartAxis::AxisLocation location, bool axisShowing);
  void showAxisGridChanged(pqChartAxis::AxisLocation location,
      bool gridShowing);
  void axisGridTypeChanged(pqChartAxis::AxisLocation location,
      pqChartAxisOptions::AxisGridColor color);
  void axisColorChanged(pqChartAxis::AxisLocation location,
      const QColor &color);
  void axisGridColorChanged(pqChartAxis::AxisLocation location,
      const QColor &color);
  //@}

  /// \name Axis Label Changes
  //@{
  void showAxisLabelsChanged(pqChartAxis::AxisLocation location,
      bool labelsShowing);
  void axisLabelFontChanged(pqChartAxis::AxisLocation location,
      const QFont &newFont);
  void axisLabelColorChanged(pqChartAxis::AxisLocation location,
      const QColor &color);
  void axisLabelNotationChanged(pqChartAxis::AxisLocation location,
      pqChartValue::NotationType notation);
  void axisLabelPrecisionChanged(pqChartAxis::AxisLocation location,
      int precision);
  //@}

  /// \name Axis Layout Changes
  //@{
  void axisScaleChanged(pqChartAxis::AxisLocation location, bool useLogScale);
  void axisBehaviorChanged(pqChartAxis::AxisLocation location,
      pqChartArea::AxisBehavior behavior);
  void axisMinimumChanged(pqChartAxis::AxisLocation location,
      const pqChartValue &minimum);
  void axisMaximumChanged(pqChartAxis::AxisLocation location,
      const pqChartValue &maximum);
  void axisLabelsChanged(pqChartAxis::AxisLocation location,
      const QStringList &list);
  //@}

  /// \name Axis Title Changes
  //@{
  void axisTitleChanged(pqChartAxis::AxisLocation location,
      const QString &title);
  void axisTitleFontChanged(pqChartAxis::AxisLocation location,
      const QFont &newFont);
  void axisTitleColorChanged(pqChartAxis::AxisLocation location,
      const QColor &color);
  void axisTitleAlignmentChanged(pqChartAxis::AxisLocation location,
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
  void removeSelectedLabels();
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
