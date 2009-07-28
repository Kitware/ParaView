/*=========================================================================

   Program: ParaView
   Module:    pqActiveChartOptions.cxx

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

/// \file pqActiveChartOptions.cxx
/// \date 7/27/2007

#include "pqActiveChartOptions.h"

#include "pqApplicationCore.h"
#include "pqBarChartOptionsEditor.h"
#include "pqBarChartOptionsHandler.h"
#include "pqBarChartView.h"
#include "pqBoxChartOptionsEditor.h"
#include "pqBoxChartOptionsHandler.h"
#include "pqChartOptionsEditor.h"
#include "pqChartOptionsHandler.h"
#include "pqLineChartView.h"
#include "pqLineChartOptionsEditor.h"
#include "pqLineChartOptionsHandler.h"
#include "pqOptionsDialog.h"
#include "pqStackedChartOptionsEditor.h"
#include "pqStackedChartOptionsHandler.h"
#include "pqUndoStack.h"

#include <QString>
#include <QVariant>
#include <QWidget>

#include "vtkSMProxy.h"


//----------------------------------------------------------------------------
pqActiveChartOptions::pqActiveChartOptions(QObject *parentObject)
  : pqActiveViewOptions(parentObject)
{
  this->Dialog = 0;
  this->Chart = new pqChartOptionsHandler();
  this->BarChart = new pqBarChartOptionsHandler();
  this->LineChart = new pqLineChartOptionsHandler();
  this->StackedChart = new pqStackedChartOptionsHandler();
  this->BoxChart = new pqBoxChartOptionsHandler();
}

pqActiveChartOptions::~pqActiveChartOptions()
{
  delete this->Chart;
  delete this->BarChart;
  delete this->LineChart;
  delete this->StackedChart;
  delete this->BoxChart;
}

void pqActiveChartOptions::showOptions(pqView *view, const QString &page,
    QWidget *widgetParent)
{
  // Create the chart options dialog if necessary.
  if(!this->Dialog)
    {
    this->Dialog = new pqOptionsDialog(widgetParent);
    this->Dialog->setObjectName("ActiveChartOptions");
    pqChartOptionsEditor *options = new pqChartOptionsEditor();
    this->Chart->setOptions(options);
    this->Dialog->addOptions(options);

    this->connect(this->Dialog, SIGNAL(finished(int)),
        this, SLOT(finishDialog(int)));
    this->connect(this->Dialog, SIGNAL(destroyed()),
        this, SLOT(cleanupDialog()));
    this->connect(this->Dialog, SIGNAL(aboutToApplyChanges()),
        this, SLOT(openUndoSet()));
    this->connect(this->Dialog, SIGNAL(appliedChanges()),
        this, SLOT(closeUndoSet()));

    // Listen for chart options changes.
    this->connect(options, SIGNAL(titleChanged(const QString &)),
        this, SLOT(setTitleModified()));
    this->connect(options, SIGNAL(titleFontChanged(const QFont &)),
        this, SLOT(setTitleFontModified()));
    this->connect(options, SIGNAL(titleColorChanged(const QColor &)),
        this, SLOT(setTitleColorModified()));
    this->connect(options, SIGNAL(titleAlignmentChanged(int)),
        this, SLOT(setTitleAlignmentModified()));
    this->connect(options, SIGNAL(showLegendChanged(bool)),
        this, SLOT(setShowLegendModified()));
    this->connect(options,
        SIGNAL(legendLocationChanged(vtkQtChartLegend::LegendLocation)),
        this, SLOT(setLegendLocationModified()));
    this->connect(options, SIGNAL(legendFlowChanged(vtkQtChartLegend::ItemFlow)),
        this, SLOT(setLegendFlowModified()));
    this->connect(options,
        SIGNAL(showAxisChanged(vtkQtChartAxis::AxisLocation, bool)),
        this, SLOT(setShowAxisModified()));
    this->connect(options,
        SIGNAL(showAxisGridChanged(vtkQtChartAxis::AxisLocation, bool)),
        this, SLOT(setShowAxisGridModified()));
    this->connect(options,
        SIGNAL(axisGridTypeChanged(vtkQtChartAxis::AxisLocation, vtkQtChartAxisOptions::AxisGridColor)),
        this, SLOT(setAxisGridTypeModified()));
    this->connect(options,
        SIGNAL(axisColorChanged(vtkQtChartAxis::AxisLocation, const QColor &)),
        this, SLOT(setAxisColorModified()));
    this->connect(options,
        SIGNAL(axisGridColorChanged(vtkQtChartAxis::AxisLocation, const QColor &)),
        this, SLOT(setAxisGridColorModified()));
    this->connect(options,
        SIGNAL(showAxisLabelsChanged(vtkQtChartAxis::AxisLocation, bool)),
        this, SLOT(setShowAxisLabelsModified()));
    this->connect(options,
        SIGNAL(axisLabelFontChanged(vtkQtChartAxis::AxisLocation, const QFont &)),
        this, SLOT(setAxisLabelFontModified()));
    this->connect(options,
        SIGNAL(axisLabelColorChanged(vtkQtChartAxis::AxisLocation, const QColor &)),
        this, SLOT(setAxisLabelColorModified()));
    this->connect(options,
        SIGNAL(axisLabelNotationChanged(vtkQtChartAxis::AxisLocation, pqChartValue::NotationType)),
        this, SLOT(setAxisLabelNotationModified()));
    this->connect(options,
        SIGNAL(axisLabelPrecisionChanged(vtkQtChartAxis::AxisLocation, int)),
        this, SLOT(setAxisLabelPrecisionModified()));
    this->connect(options,
        SIGNAL(axisScaleChanged(vtkQtChartAxis::AxisLocation, bool)),
        this, SLOT(setAxisScaleModified()));
    this->connect(options,
        SIGNAL(axisBehaviorChanged(vtkQtChartAxis::AxisLocation, vtkQtChartAxisLayer::AxisBehavior)),
        this, SLOT(setAxisBehaviorModified()));
    this->connect(options,
        SIGNAL(axisMinimumChanged(vtkQtChartAxis::AxisLocation, const pqChartValue &)),
        this, SLOT(setAxisMinimumModified()));
    this->connect(options,
        SIGNAL(axisMaximumChanged(vtkQtChartAxis::AxisLocation, const pqChartValue &)),
        this, SLOT(setAxisMaximumModified()));
    this->connect(options,
        SIGNAL(axisLabelsChanged(vtkQtChartAxis::AxisLocation, const QStringList &)),
        this, SLOT(setAxisLabelsModified()));
    this->connect(options,
        SIGNAL(axisTitleChanged(vtkQtChartAxis::AxisLocation, const QString &)),
        this, SLOT(setAxisTitleModified()));
    this->connect(options,
        SIGNAL(axisTitleFontChanged(vtkQtChartAxis::AxisLocation, const QFont &)),
        this, SLOT(setAxisTitleFontModified()));
    this->connect(options,
        SIGNAL(axisTitleColorChanged(vtkQtChartAxis::AxisLocation, const QColor &)),
        this, SLOT(setAxisTitleColorModified()));
    this->connect(options,
        SIGNAL(axisTitleAlignmentChanged(vtkQtChartAxis::AxisLocation, int)),
        this, SLOT(setAxisTitleAlignmentModified()));
    }

  this->changeView(view);
  if(this->Chart->getView())
    {
    if(page.isEmpty())
      {
      this->Dialog->setCurrentPage("General");
      }
    else
      {
      this->Dialog->setCurrentPage(page);
      }

    this->Dialog->setResult(0);
    this->Dialog->show();
    }
}

void pqActiveChartOptions::changeView(pqView *view)
{
  if(this->Dialog)
    {
    pqBarChartView *barChart = qobject_cast<pqBarChartView *>(view);
    pqLineChartView *lineChart = qobject_cast<pqLineChartView *>(view);
    //pqStackedChartView *stackedChart = qobject_cast<pqStackedChartView *>(view);
    //pqBoxChartView *boxChart = qobject_cast<pqBoxChartView *>(view);
    if(barChart || lineChart /*|| stackedChart || boxChart*/)
      {
      this->Chart->setView(view);
      }
    else
      {
      this->Dialog->setWindowTitle("Chart Options");
      this->Chart->setView(0);
      }

    pqBarChartOptionsEditor *barOptions = this->BarChart->getOptions();
    if(barChart)
      {
      // Set the dialog title.
      this->Dialog->setWindowTitle("Bar Chart Options");

      // Make sure the bar chart options are added.
      if(!barOptions)
        {
        // Set up the dialog for the extra bar chart options.
        barOptions = new pqBarChartOptionsEditor();
        this->BarChart->setOptions(barOptions);
        this->Dialog->addOptions("Bar Chart", barOptions);
        this->BarChart->setView(barChart);

        // Listen for bar chart option changes.
        this->connect(barOptions, SIGNAL(helpFormatChanged(const QString &)),
            this, SLOT(setBarHelpFormatModified()));
        this->connect(barOptions,
            SIGNAL(outlineStyleChanged(vtkQtBarChartOptions::OutlineStyle)),
            this, SLOT(setBarOutlineStyleModified()));
        this->connect(barOptions, SIGNAL(barGroupFractionChanged(float)),
            this, SLOT(setBarGroupFractionModified()));
        this->connect(barOptions, SIGNAL(barWidthFractionChanged(float)),
            this, SLOT(setBarWidthFractionModified()));
        }
      }
    else if(barOptions)
      {
      // Remove the bar chart options from the dialog.
      this->Dialog->removeOptions(barOptions);
      this->BarChart->setOptions(0);
      this->BarChart->setView(0);
      delete barOptions;
      }

    pqLineChartOptionsEditor *lineOptions = this->LineChart->getOptions();
    if(lineChart)
      {
      // Set the dialog title.
      this->Dialog->setWindowTitle("Line Chart Options");

      // Make sure the line chart options are added.
      if(!lineOptions)
        {
        // Set up the dialog for the extra line chart options.
        lineOptions = new pqLineChartOptionsEditor();
        this->LineChart->setOptions(lineOptions);
        this->Dialog->addOptions("Line Chart", lineOptions);
        this->LineChart->setView(lineChart);

        // Listen for line chart option changes.
        this->connect(lineOptions, SIGNAL(helpFormatChanged(const QString &)),
            this, SLOT(setLineHelpFormatModified()));
        }
      }
    else if(lineOptions)
      {
      // Remove the line chart options from the dialog.
      this->Dialog->removeOptions(lineOptions);
      this->LineChart->setOptions(0);
      this->LineChart->setView(0);
      delete lineOptions;
      }

    pqStackedChartOptionsEditor *stackedOptions =
        this->StackedChart->getOptions();
    /*if(stackedChart)
      {
      // Set the dialog title.
      this->Dialog->setWindowTitle("Stacked Chart Options");

      // Make sure the stacked chart options are added.
      if(!stackedOptions)
        {
        // Set up the dialog for the extra stacked chart options.
        stackedOptions = new pqStackedChartOptionsEditor();
        this->StackedChart->setOptions(stackedOptions);
        this->Dialog->addOptions("Stacked Chart", stackedOptions);
        this->StackedChart->setView(stackedChart);

        // Listen for stacked chart option changes.
        this->connect(stackedOptions,
            SIGNAL(helpFormatChanged(const QString &)),
            this, SLOT(setStackedHelpFormatModified()));
        this->connect(stackedOptions, SIGNAL(normalizationChanged(bool)),
            this, SLOT(setStackedNormalizationModified()));
        this->connect(stackedOptions, SIGNAL(gradientChanged(bool)),
            this, SLOT(setStackedGradientModified()));
        }
      }
    else*/ if(stackedOptions)
      {
      // Remove the stacked chart options from the dialog.
      this->Dialog->removeOptions(stackedOptions);
      this->StackedChart->setOptions(0);
      this->StackedChart->setView(0);
      delete stackedOptions;
      }

    pqBoxChartOptionsEditor *boxOptions = this->BoxChart->getOptions();
    /*if(boxChart)
      {
      // Set the dialog title.
      this->Dialog->setWindowTitle("Statistical Box Chart Options");

      // Make sure the box chart options are added.
      if(!boxOptions)
        {
        // Set up the dialog for the extra box chart options.
        boxOptions = new pqBoxChartOptionsEditor();
        this->BoxChart->setOptions(boxOptions);
        this->Dialog->addOptions("Statistical Box Chart", boxOptions);
        this->BoxChart->setView(boxChart);

        // Listen for box chart option changes.
        this->connect(boxOptions, SIGNAL(helpFormatChanged(const QString &)),
            this, SLOT(setBoxHelpFormatModified()));
        this->connect(boxOptions, SIGNAL(outlierFormatChanged(const QString &)),
            this, SLOT(setBoxOutlierFormatModified()));
        this->connect(boxOptions,
            SIGNAL(outlineStyleChanged(vtkQtStatisticalBoxChartOptions::OutlineStyle)),
            this, SLOT(setBoxOutlineStyleModified()));
        this->connect(boxOptions, SIGNAL(boxWidthFractionChanged(float)),
            this, SLOT(setBoxWidthFractionModified()));
        }
      }
    else*/ if(boxOptions)
      {
      // Remove the bar chart options from the dialog.
      this->Dialog->removeOptions(boxOptions);
      this->BoxChart->setOptions(0);
      this->BoxChart->setView(0);
      delete boxOptions;
      }
    }
}

void pqActiveChartOptions::closeOptions()
{
  if(this->Dialog && this->Chart->getView())
    {
    this->Dialog->accept();
    this->Chart->setView(0);
    this->BarChart->setView(0);
    this->LineChart->setView(0);
    this->StackedChart->setView(0);
    this->BoxChart->setView(0);
    }
}

void pqActiveChartOptions::finishDialog(int result)
{
  if(result != QDialog::Accepted)
    {
    this->Dialog->setApplyNeeded(false);
    }

  emit this->optionsClosed(this);
}

void pqActiveChartOptions::cleanupDialog()
{
  // If the dialog was deleted, the chart options will be deleted as
  // well, which will clean up the chart connections.
  this->Dialog = 0;
  this->Chart->setOptions(0);
  this->Chart->setView(0);
  this->BarChart->setOptions(0);
  this->BarChart->setView(0);
  this->LineChart->setOptions(0);
  this->LineChart->setView(0);
  this->StackedChart->setOptions(0);
  this->StackedChart->setView(0);
  this->BoxChart->setOptions(0);
  this->BoxChart->setView(0);
}

void pqActiveChartOptions::openUndoSet()
{
  pqUndoStack *stack = pqApplicationCore::instance()->getUndoStack();
  if(stack)
    {
    stack->beginUndoSet("Chart Options");
    }
}

void pqActiveChartOptions::closeUndoSet()
{
  pqUndoStack *stack = pqApplicationCore::instance()->getUndoStack();
  if(stack)
    {
    stack->endUndoSet();
    }

  pqView *view = this->Chart->getView();
  if(view)
    {
    view->getProxy()->UpdateVTKObjects();
    view->render();
    }
}

void pqActiveChartOptions::setTitleModified()
{
  this->Chart->setModified(pqChartOptionsHandler::TitleModified);
}

void pqActiveChartOptions::setTitleFontModified()
{
  this->Chart->setModified(pqChartOptionsHandler::TitleFontModified);
}

void pqActiveChartOptions::setTitleColorModified()
{
  this->Chart->setModified(pqChartOptionsHandler::TitleColorModified);
}

void pqActiveChartOptions::setTitleAlignmentModified()
{
  this->Chart->setModified(pqChartOptionsHandler::TitleAlignmentModified);
}

void pqActiveChartOptions::setShowLegendModified()
{
  this->Chart->setModified(pqChartOptionsHandler::ShowLegendModified);
}

void pqActiveChartOptions::setLegendLocationModified()
{
  this->Chart->setModified(pqChartOptionsHandler::LegendLocationModified);
}

void pqActiveChartOptions::setLegendFlowModified()
{
  this->Chart->setModified(pqChartOptionsHandler::LegendFlowModified);
}

void pqActiveChartOptions::setShowAxisModified()
{
  this->Chart->setModified(pqChartOptionsHandler::ShowAxisModified);
}

void pqActiveChartOptions::setShowAxisGridModified()
{
  this->Chart->setModified(pqChartOptionsHandler::ShowAxisGridModified);
}

void pqActiveChartOptions::setAxisGridTypeModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisGridTypeModified);
}

void pqActiveChartOptions::setAxisColorModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisColorModified);
}

void pqActiveChartOptions::setAxisGridColorModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisGridColorModified);
}

void pqActiveChartOptions::setShowAxisLabelsModified()
{
  this->Chart->setModified(pqChartOptionsHandler::ShowAxisLabelsModified);
}

void pqActiveChartOptions::setAxisLabelFontModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisLabelFontModified);
}

void pqActiveChartOptions::setAxisLabelColorModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisLabelColorModified);
}

void pqActiveChartOptions::setAxisLabelNotationModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisLabelNotationModified);
}

void pqActiveChartOptions::setAxisLabelPrecisionModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisLabelPrecisionModified);
}

void pqActiveChartOptions::setAxisScaleModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisScaleModified);
}

void pqActiveChartOptions::setAxisBehaviorModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisBehaviorModified);
}

void pqActiveChartOptions::setAxisMinimumModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisMinimumModified);
}

void pqActiveChartOptions::setAxisMaximumModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisMaximumModified);
}

void pqActiveChartOptions::setAxisLabelsModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisLabelsModified);
}

void pqActiveChartOptions::setAxisTitleModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisTitleModified);
}

void pqActiveChartOptions::setAxisTitleFontModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisTitleFontModified);
}

void pqActiveChartOptions::setAxisTitleColorModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisTitleColorModified);
}

void pqActiveChartOptions::setAxisTitleAlignmentModified()
{
  this->Chart->setModified(pqChartOptionsHandler::AxisTitleAlignmentModified);
}

void pqActiveChartOptions::setBarHelpFormatModified()
{
  this->BarChart->setModified(pqBarChartOptionsHandler::HelpFormatModified);
}

void pqActiveChartOptions::setBarOutlineStyleModified()
{
  this->BarChart->setModified(pqBarChartOptionsHandler::OutlineStyleModified);
}

void pqActiveChartOptions::setBarGroupFractionModified()
{
  this->BarChart->setModified(pqBarChartOptionsHandler::GroupFractionModified);
}

void pqActiveChartOptions::setBarWidthFractionModified()
{
  this->BarChart->setModified(pqBarChartOptionsHandler::WidthFractionModified);
}

void pqActiveChartOptions::setLineHelpFormatModified()
{
  this->LineChart->setModified(pqLineChartOptionsHandler::HelpFormatModified);
}

void pqActiveChartOptions::setStackedHelpFormatModified()
{
  this->StackedChart->setModified(
    pqStackedChartOptionsHandler::HelpFormatModified);
}

void pqActiveChartOptions::setStackedNormalizationModified()
{
  this->StackedChart->setModified(
    pqStackedChartOptionsHandler::NormalizationModified);
}

void pqActiveChartOptions::setStackedGradientModified()
{
  this->StackedChart->setModified(
    pqStackedChartOptionsHandler::GradientModified);
}

void pqActiveChartOptions::setBoxHelpFormatModified()
{
  this->BoxChart->setModified(pqBoxChartOptionsHandler::HelpFormatModified);
}

void pqActiveChartOptions::setBoxOutlierFormatModified()
{
  this->BoxChart->setModified(pqBoxChartOptionsHandler::OutlierFormatModified);
}

void pqActiveChartOptions::setBoxOutlineStyleModified()
{
  this->BoxChart->setModified(pqBoxChartOptionsHandler::OutlineStyleModified);
}

void pqActiveChartOptions::setBoxWidthFractionModified()
{
  this->BoxChart->setModified(pqBoxChartOptionsHandler::WidthFractionModified);
}


