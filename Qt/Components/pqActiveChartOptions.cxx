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
#include "pqChartOptionsEditor.h"
#include "pqChartOptionsHandler.h"
#include "pqOptionsDialog.h"
#include "pqPlotView.h"
#include "pqUndoStack.h"

#include <QString>
#include <QVariant>
#include <QWidget>

#include "vtkSMProxy.h"


//----------------------------------------------------------------------------
pqActiveChartOptions::pqActiveChartOptions(QObject *parentObject)
  : pqActiveViewOptions(parentObject)
{
  this->Chart = new pqChartOptionsHandler();
  this->BarChart = new pqBarChartOptionsHandler();
  this->Dialog = 0;
}

pqActiveChartOptions::~pqActiveChartOptions()
{
  delete this->Chart;
  delete this->BarChart;
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
        SIGNAL(legendLocationChanged(pqChartLegend::LegendLocation)),
        this, SLOT(setLegendLocationModified()));
    this->connect(options, SIGNAL(legendFlowChanged(pqChartLegend::ItemFlow)),
        this, SLOT(setLegendFlowModified()));
    this->connect(options,
        SIGNAL(showAxisChanged(pqChartAxis::AxisLocation, bool)),
        this, SLOT(setShowAxisModified()));
    this->connect(options,
        SIGNAL(showAxisGridChanged(pqChartAxis::AxisLocation, bool)),
        this, SLOT(setShowAxisGridModified()));
    this->connect(options,
        SIGNAL(axisGridTypeChanged(pqChartAxis::AxisLocation, pqChartAxisOptions::AxisGridColor)),
        this, SLOT(setAxisGridTypeModified()));
    this->connect(options,
        SIGNAL(axisColorChanged(pqChartAxis::AxisLocation, const QColor &)),
        this, SLOT(setAxisColorModified()));
    this->connect(options,
        SIGNAL(axisGridColorChanged(pqChartAxis::AxisLocation, const QColor &)),
        this, SLOT(setAxisGridColorModified()));
    this->connect(options,
        SIGNAL(showAxisLabelsChanged(pqChartAxis::AxisLocation, bool)),
        this, SLOT(setShowAxisLabelsModified()));
    this->connect(options,
        SIGNAL(axisLabelFontChanged(pqChartAxis::AxisLocation, const QFont &)),
        this, SLOT(setAxisLabelFontModified()));
    this->connect(options,
        SIGNAL(axisLabelColorChanged(pqChartAxis::AxisLocation, const QColor &)),
        this, SLOT(setAxisLabelColorModified()));
    this->connect(options,
        SIGNAL(axisLabelNotationChanged(pqChartAxis::AxisLocation, pqChartValue::NotationType)),
        this, SLOT(setAxisLabelNotationModified()));
    this->connect(options,
        SIGNAL(axisLabelPrecisionChanged(pqChartAxis::AxisLocation, int)),
        this, SLOT(setAxisLabelPrecisionModified()));
    this->connect(options,
        SIGNAL(axisScaleChanged(pqChartAxis::AxisLocation, bool)),
        this, SLOT(setAxisScaleModified()));
    this->connect(options,
        SIGNAL(axisBehaviorChanged(pqChartAxis::AxisLocation, pqChartArea::AxisBehavior)),
        this, SLOT(setAxisBehaviorModified()));
    this->connect(options,
        SIGNAL(axisMinimumChanged(pqChartAxis::AxisLocation, const pqChartValue &)),
        this, SLOT(setAxisMinimumModified()));
    this->connect(options,
        SIGNAL(axisMaximumChanged(pqChartAxis::AxisLocation, const pqChartValue &)),
        this, SLOT(setAxisMaximumModified()));
    this->connect(options,
        SIGNAL(axisLabelsChanged(pqChartAxis::AxisLocation, const QStringList &)),
        this, SLOT(setAxisLabelsModified()));
    this->connect(options,
        SIGNAL(axisTitleChanged(pqChartAxis::AxisLocation, const QString &)),
        this, SLOT(setAxisTitleModified()));
    this->connect(options,
        SIGNAL(axisTitleFontChanged(pqChartAxis::AxisLocation, const QFont &)),
        this, SLOT(setAxisTitleFontModified()));
    this->connect(options,
        SIGNAL(axisTitleColorChanged(pqChartAxis::AxisLocation, const QColor &)),
        this, SLOT(setAxisTitleColorModified()));
    this->connect(options,
        SIGNAL(axisTitleAlignmentChanged(pqChartAxis::AxisLocation, int)),
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
    pqPlotView *plotView = qobject_cast<pqPlotView *>(view);
    pqBarChartView *barChart = qobject_cast<pqBarChartView *>(view);
    if(plotView || barChart)
      {
      this->Chart->setView(view);
      }
    else
      {
      this->Dialog->setWindowTitle("Chart Options");
      this->Chart->setView(0);
      }

    if(plotView)
      {
      this->Dialog->setWindowTitle("Chart Options");
      }

    if(barChart)
      {
      // Set the dialog title.
      this->Dialog->setWindowTitle("Bar Chart Options");

      // Set up the dialog for the extra bar chart options.
      pqBarChartOptionsEditor *barOptions = new pqBarChartOptionsEditor();
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
    else if(this->BarChart->getOptions())
      {
      // Remove the bar chart options from the dialog.
      pqBarChartOptionsEditor *barOptions = this->BarChart->getOptions();
      this->Dialog->removeOptions(barOptions);
      this->BarChart->setOptions(0);
      this->BarChart->setView(0);
      delete barOptions;
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


