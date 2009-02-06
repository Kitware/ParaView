/*=========================================================================

   Program: ParaView
   Module:    pqChartViewPropertyHandler.cxx

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

========================================================================*/

#include "pqChartViewPropertyHandler.h"

#include "pqSMAdaptor.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkQtChartViewBase.h"

#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartWidget.h"

#include "vtkSMProperty.h"
#include "vtkSMProxy.h"


pqChartViewPropertyHandler::pqChartViewPropertyHandler(
  vtkQtChartViewBase* chartView, vtkSMProxy* proxy, QObject* parentObject)
  : QObject(parentObject)
{
  this->ChartView = chartView;
  this->Proxy = proxy;
}

void pqChartViewPropertyHandler::setDefaultPropertyValues()
{
  // Load defaults for the properties that need them.
  int i = 0;
  QList<QVariant> values;
  for(i = 0; i < 4; i++)
    {
    values.append(QVariant((double)0.0));
    values.append(QVariant((double)0.0));
    values.append(QVariant((double)0.0));
    }

  pqSMAdaptor::setMultipleElementProperty(
      this->Proxy->GetProperty("AxisLabelColor"), values);
  pqSMAdaptor::setMultipleElementProperty(
      this->Proxy->GetProperty("AxisTitleColor"), values);
  values.clear();
  for(i = 0; i < 4; i++)
    {
    if(i < 2)
      {
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      }
    else
      {
      // Use a different color for the right and top axis.
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.5));
      }
    }

  pqSMAdaptor::setMultipleElementProperty(
      this->Proxy->GetProperty("AxisColor"), values);
  values.clear();
  for(i = 0; i < 4; i++)
    {
    QColor grid = Qt::lightGray;
    values.append(QVariant((double)grid.redF()));
    values.append(QVariant((double)grid.greenF()));
    values.append(QVariant((double)grid.blueF()));
    }

  pqSMAdaptor::setMultipleElementProperty(
      this->Proxy->GetProperty("AxisGridColor"), values);
  QFont chartFont = this->ChartView->GetChartWidget()->font();
  values.clear();
  values.append(chartFont.family());
  values.append(QVariant(chartFont.pointSize()));
  values.append(QVariant(chartFont.bold() ? 1 : 0));
  values.append(QVariant(chartFont.italic() ? 1 : 0));
  pqSMAdaptor::setMultipleElementProperty(
      this->Proxy->GetProperty("ChartTitleFont"), values);
  for(i = 0; i < 3; i++)
    {
    values.append(chartFont.family());
    values.append(QVariant(chartFont.pointSize()));
    values.append(QVariant(chartFont.bold() ? 1 : 0));
    values.append(QVariant(chartFont.italic() ? 1 : 0));
    }

  pqSMAdaptor::setMultipleElementProperty(
      this->Proxy->GetProperty("AxisLabelFont"), values);
  pqSMAdaptor::setMultipleElementProperty(
      this->Proxy->GetProperty("AxisTitleFont"), values);
}

void pqChartViewPropertyHandler::connectProperties(
  vtkEventQtSlotConnect* connector)
{
  // Listen for title property changes.
  connector->Connect(this->Proxy->GetProperty("ChartTitle"),
    vtkCommand::ModifiedEvent, this, SLOT(updateTitle()));
  connector->Connect(this->Proxy->GetProperty("ChartTitleFont"),
    vtkCommand::ModifiedEvent, this, SLOT(updateTitleFont()));
  connector->Connect(this->Proxy->GetProperty("ChartTitleColor"),
    vtkCommand::ModifiedEvent, this, SLOT(updateTitleColor()));
  connector->Connect(this->Proxy->GetProperty("ChartTitleAlignment"),
    vtkCommand::ModifiedEvent, this, SLOT(updateTitleAlignment()));

  // Listen for axis title property changes.
  connector->Connect(this->Proxy->GetProperty("AxisTitle"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisTitle()));
  connector->Connect(this->Proxy->GetProperty("AxisTitleFont"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisTitleFont()));
  connector->Connect(this->Proxy->GetProperty("AxisTitleColor"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisTitleColor()));
  connector->Connect(this->Proxy->GetProperty("AxisTitleAlignment"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisTitleAlignment()));

  // Listen for legend property changes.
  connector->Connect(this->Proxy->GetProperty("ShowLegend"),
    vtkCommand::ModifiedEvent, this, SLOT(updateLegendVisibility()));
  connector->Connect(this->Proxy->GetProperty("LegendLocation"),
    vtkCommand::ModifiedEvent, this, SLOT(updateLegendLocation()));
  connector->Connect(this->Proxy->GetProperty("LegendFlow"),
    vtkCommand::ModifiedEvent, this, SLOT(updateLegendFlow()));

  // Listen for axis drawing property changes.
  connector->Connect(this->Proxy->GetProperty("ShowAxis"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisVisibility()));
  connector->Connect(this->Proxy->GetProperty("AxisColor"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisColor()));
  connector->Connect(this->Proxy->GetProperty("ShowAxisGrid"),
    vtkCommand::ModifiedEvent, this, SLOT(updateGridVisibility()));
  connector->Connect(this->Proxy->GetProperty("AxisGridType"),
    vtkCommand::ModifiedEvent, this, SLOT(updateGridColorType()));
  connector->Connect(this->Proxy->GetProperty("AxisGridColor"),
    vtkCommand::ModifiedEvent, this, SLOT(updateGridColor()));
  connector->Connect(this->Proxy->GetProperty("ShowAxisLabels"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisLabelVisibility()));
  connector->Connect(this->Proxy->GetProperty("AxisLabelFont"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisLabelFont()));
  connector->Connect(this->Proxy->GetProperty("AxisLabelColor"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisLabelColor()));
  connector->Connect(this->Proxy->GetProperty("AxisLabelPrecision"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisLabelPrecision()));
  connector->Connect(this->Proxy->GetProperty("AxisLabelNotation"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisLabelNotation()));

  // Listen for axis layout property changes.
  connector->Connect(this->Proxy->GetProperty("AxisScale"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisScale()));
  connector->Connect(this->Proxy->GetProperty("AxisBehavior"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisBehavior()));
  connector->Connect(this->Proxy->GetProperty("AxisMinimum"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisRange()));
  connector->Connect(this->Proxy->GetProperty("AxisMaximum"),
    vtkCommand::ModifiedEvent, this, SLOT(updateAxisRange()));
  connector->Connect(this->Proxy->GetProperty("LeftAxisLabels"),
    vtkCommand::ModifiedEvent, this, SLOT(updateLeftAxisLabels()));
  connector->Connect(this->Proxy->GetProperty("BottomAxisLabels"),
    vtkCommand::ModifiedEvent, this, SLOT(updateBottomAxisLabels()));
  connector->Connect(this->Proxy->GetProperty("RightAxisLabels"),
    vtkCommand::ModifiedEvent, this, SLOT(updateRightAxisLabels()));
  connector->Connect(this->Proxy->GetProperty("TopAxisLabels"),
    vtkCommand::ModifiedEvent, this, SLOT(updateTopAxisLabels()));
}

void pqChartViewPropertyHandler::disconnectProperties(vtkEventQtSlotConnect*)
{
  // TODO
}

void pqChartViewPropertyHandler::updateTitle()
{
  this->ChartView->SetTitle(pqSMAdaptor::getElementProperty(
    this->Proxy->GetProperty("ChartTitle")).toString().toAscii().data());
}

void pqChartViewPropertyHandler::updateTitleFont()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->Proxy->GetProperty("ChartTitleFont"));
  if(values.size() == 4)
    {
    this->ChartView->SetTitleFont(
      values[0].toString().toAscii().data(), values[1].toInt(),
      values[2].toInt() != 0, values[3].toInt() != 0);
    }
}

void pqChartViewPropertyHandler::updateTitleColor()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->Proxy->GetProperty("ChartTitleColor"));
  if(values.size() == 3)
    {
    this->ChartView->SetTitleColor(values[0].toDouble(),
      values[1].toDouble(), values[2].toDouble());
    }
}

void pqChartViewPropertyHandler::updateTitleAlignment()
{
  this->ChartView->SetTitleAlignment(
    pqSMAdaptor::getElementProperty(
    this->Proxy->GetProperty("ChartTitleAlignment")).toInt());
}

void pqChartViewPropertyHandler::updateAxisTitle()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisTitle"));
  for(int i = 0; i < 4 && i < values.size(); ++i)
    {
    this->ChartView->SetAxisTitle(i,
      values[i].toString().toAscii().data());
    }
}

void pqChartViewPropertyHandler::updateAxisTitleFont()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisTitleFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    this->ChartView->SetAxisTitleFont(i,
      values[j].toString().toAscii().data(), values[j + 1].toInt(),
      values[j + 2].toInt() != 0, values[j + 3].toInt() != 0);
    }
}

void pqChartViewPropertyHandler::updateAxisTitleColor()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisTitleColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->ChartView->SetAxisTitleColor(i, values[j].toDouble(),
      values[j + 1].toDouble(), values[j + 2].toDouble());
    }
}

void pqChartViewPropertyHandler::updateAxisTitleAlignment()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisTitleAlignment"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetAxisTitleAlignment(i, values[i].toInt());
    }
}

void pqChartViewPropertyHandler::updateLegendVisibility()
{
  this->ChartView->SetLegendVisibility(
    pqSMAdaptor::getElementProperty(
    this->Proxy->GetProperty("ShowLegend")).toInt() != 0);
}

void pqChartViewPropertyHandler::updateLegendLocation()
{
  this->ChartView->SetLegendLocation(
    pqSMAdaptor::getElementProperty(
    this->Proxy->GetProperty("LegendLocation")).toInt());
}

void pqChartViewPropertyHandler::updateLegendFlow()
{
  this->ChartView->SetLegendFlow(
    pqSMAdaptor::getElementProperty(
    this->Proxy->GetProperty("LegendFlow")).toInt());
}

void pqChartViewPropertyHandler::updateAxisVisibility()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("ShowAxis"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetAxisVisibility(i, values[i].toInt() != 0);
    }
}

void pqChartViewPropertyHandler::updateAxisColor()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->ChartView->SetAxisColor(i, values[j].toDouble(),
      values[j + 1].toDouble(), values[j + 2].toDouble());
    }
}

void pqChartViewPropertyHandler::updateGridVisibility()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("ShowAxisGrid"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetGridVisibility(i, values[i].toInt() != 0);
    }
}

void pqChartViewPropertyHandler::updateGridColorType()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisGridType"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetGridColorType(i, values[i].toInt());
    }
}

void pqChartViewPropertyHandler::updateGridColor()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisGridColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->ChartView->SetGridColor(i, values[j].toDouble(),
      values[j + 1].toDouble(), values[j + 2].toDouble());
    }
}

void pqChartViewPropertyHandler::updateAxisLabelVisibility()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("ShowAxisLabels"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetAxisLabelVisibility(i,
      values[i].toInt() != 0);
    }
}

void pqChartViewPropertyHandler::updateAxisLabelFont()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisLabelFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    this->ChartView->SetAxisLabelFont(i,
      values[j].toString().toAscii().data(), values[j + 1].toInt(),
      values[j + 2].toInt() != 0, values[j + 3].toInt() != 0);
    }
}

void pqChartViewPropertyHandler::updateAxisLabelColor()
{
  int i, j;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisLabelColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->ChartView->SetAxisLabelColor(i, values[j].toDouble(),
      values[j + 1].toDouble(), values[j + 2].toDouble());
    }
}

void pqChartViewPropertyHandler::updateAxisLabelPrecision()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisLabelPrecision"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetAxisLabelPrecision(i, values[i].toInt());
    }
}

void pqChartViewPropertyHandler::updateAxisLabelNotation()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisLabelNotation"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetAxisLabelNotation(i, values[i].toInt());
    }
}

void pqChartViewPropertyHandler::updateAxisScale()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisScale"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetAxisScale(i, values[i].toInt());
    }
}

void pqChartViewPropertyHandler::updateAxisBehavior()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisBehavior"));
  for(int i = 0; i < 4 && i < values.size(); i++)
    {
    this->ChartView->SetAxisBehavior(i, values[i].toInt());
    }
}

void pqChartViewPropertyHandler::updateAxisRange()
{
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisMinimum"));
  QList<QVariant> maxValues = pqSMAdaptor::getMultipleElementProperty(
    this->Proxy->GetProperty("AxisMaximum"));
  for(int i = 0; i < 4 && i < values.size() && i < maxValues.size(); i++)
    {
    this->ChartView->SetAxisRange(i, values[i].toDouble(),
      maxValues[i].toDouble());
    }
}

void pqChartViewPropertyHandler::updateLeftAxisLabels()
{
  vtkQtChartArea* area = this->ChartView->GetChartArea();
  if(area->getAxisLayer()->getAxisBehavior(vtkQtChartAxis::Left) ==
    vtkQtChartAxisLayer::FixedInterval)
    {
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->Proxy->GetProperty("LeftAxisLabels"));
    vtkQtChartAxis* axis = this->ChartView->GetAxis(0);
    vtkQtChartAxisModel* model = axis->getModel();
    model->startModifyingData();
    model->removeAllLabels();
    QList<QVariant>::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      model->addLabel(*iter);
      }

    model->finishModifyingData();
    }
}

void pqChartViewPropertyHandler::updateBottomAxisLabels()
{
  vtkQtChartArea* area = this->ChartView->GetChartArea();
  if(area->getAxisLayer()->getAxisBehavior(vtkQtChartAxis::Bottom) ==
    vtkQtChartAxisLayer::FixedInterval)
    {
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->Proxy->GetProperty("BottomAxisLabels"));
    vtkQtChartAxis* axis = this->ChartView->GetAxis(1);
    vtkQtChartAxisModel* model = axis->getModel();
    model->startModifyingData();
    model->removeAllLabels();
    QList<QVariant>::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      model->addLabel(*iter);
      }

    model->finishModifyingData();
    }
}

void pqChartViewPropertyHandler::updateRightAxisLabels()
{
  vtkQtChartArea* area = this->ChartView->GetChartArea();
  if(area->getAxisLayer()->getAxisBehavior(vtkQtChartAxis::Right) ==
    vtkQtChartAxisLayer::FixedInterval)
    {
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->Proxy->GetProperty("RightAxisLabels"));
    vtkQtChartAxis* axis = this->ChartView->GetAxis(2);
    vtkQtChartAxisModel* model = axis->getModel();
    model->startModifyingData();
    model->removeAllLabels();
    QList<QVariant>::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      model->addLabel(*iter);
      }

    model->finishModifyingData();
    }
}

void pqChartViewPropertyHandler::updateTopAxisLabels()
{
  vtkQtChartArea* area = this->ChartView->GetChartArea();
  if(area->getAxisLayer()->getAxisBehavior(vtkQtChartAxis::Top) ==
    vtkQtChartAxisLayer::FixedInterval)
    {
    QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      this->Proxy->GetProperty("TopAxisLabels"));
    vtkQtChartAxis* axis = this->ChartView->GetAxis(3);
    vtkQtChartAxisModel* model = axis->getModel();
    model->startModifyingData();
    model->removeAllLabels();
    QList<QVariant>::Iterator iter = values.begin();
    for( ; iter != values.end(); ++iter)
      {
      model->addLabel(*iter);
      }

    model->finishModifyingData();
    }
}


