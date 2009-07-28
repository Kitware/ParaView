/*=========================================================================

   Program: ParaView
   Module:    pqChartOptionsHandler.cxx

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

#include "pqChartOptionsHandler.h"

#include "pqChartOptionsEditor.h"
#include "pqSMAdaptor.h"
#include "pqView.h"

#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartLegend.h"
#include "vtkSMProxy.h"

#include <QVariant>


pqChartOptionsHandler::pqChartOptionsHandler()
{
  this->ModifiedData = 0;
  this->Options = 0;
  this->View = 0;
}

void pqChartOptionsHandler::setOptions(pqChartOptionsEditor *options)
{
  this->Options = options;
  if(this->Options)
    {
    this->Options->setApplyHandler(this);
    }
}

void pqChartOptionsHandler::setView(pqView *chart)
{
  this->View = chart;
  this->initializeOptions();
}

void pqChartOptionsHandler::setModified(
    pqChartOptionsHandler::ModifiedFlag flag)
{
  this->ModifiedData |= flag;
  this->Options->sendChangesAvailable();
}

void pqChartOptionsHandler::applyChanges()
{
  if(this->ModifiedData == 0 || !this->Options || !this->View)
    {
    return;
    }

  int i = 0;
  vtkQtChartAxis::AxisLocation axes[] =
    {
    vtkQtChartAxis::Left,
    vtkQtChartAxis::Bottom,
    vtkQtChartAxis::Right,
    vtkQtChartAxis::Top
    };

  const char *labelProperties[] =
    {
    "LeftAxisLabels",
    "BottomAxisLabels",
    "RightAxisLabels",
    "TopAxisLabels"
    };

  vtkSMProxy *proxy = this->View->getProxy();
  if(this->ModifiedData & TitleModified)
    {
    QString text;
    this->Options->getTitle(text);
    pqSMAdaptor::setElementProperty(proxy->GetProperty("ChartTitle"), text);
    }

  if(this->ModifiedData & TitleFontModified)
    {
    QList<QVariant> values;
    QFont titleFont = this->Options->getTitleFont();
    values.append(QVariant(titleFont.family()));
    values.append(QVariant(titleFont.pointSize()));
    values.append(QVariant(titleFont.bold() ? 1 : 0));
    values.append(QVariant(titleFont.italic() ? 1 : 0));
    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("ChartTitleFont"), values);
    }

  if(this->ModifiedData & TitleColorModified)
    {
    QColor color;
    QList<QVariant> values;
    this->Options->getTitleColor(color);
    values.append(QVariant((double)color.redF()));
    values.append(QVariant((double)color.greenF()));
    values.append(QVariant((double)color.blueF()));
    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("ChartTitleColor"), values);
    }

  if(this->ModifiedData & TitleAlignmentModified)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("ChartTitleAlignment"),
        QVariant(this->Options->getTitleAlignment()));
    }

  else if(this->ModifiedData & ShowLegendModified)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("ShowLegend"),
        QVariant(this->Options->isLegendShowing() ? 1 : 0));
    }

  if(this->ModifiedData & LegendLocationModified)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("LegendLocation"),
        QVariant((int)this->Options->getLegendLocation()));
    }

  if(this->ModifiedData & LegendFlowModified)
    {
    pqSMAdaptor::setElementProperty(proxy->GetProperty("LegendFlow"),
        QVariant((int)this->Options->getLegendFlow()));
    }

  if(this->ModifiedData & ShowAxisModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant(this->Options->isAxisShowing(axes[i]) ? 1 : 0));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("ShowAxis"), values);
    }

  if(this->ModifiedData & ShowAxisGridModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant(
          this->Options->isAxisGridShowing(axes[i]) ? 1 : 0));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("ShowAxisGrid"), values);
    }

  if(this->ModifiedData & AxisGridTypeModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant((int)this->Options->getAxisGridType(axes[i])));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisGridType"), values);
    }

  if(this->ModifiedData & AxisColorModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      QColor color = this->Options->getAxisColor(axes[i]);
      values.append(QVariant((double)color.redF()));
      values.append(QVariant((double)color.greenF()));
      values.append(QVariant((double)color.blueF()));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisColor"), values);
    }

  if(this->ModifiedData & AxisGridColorModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      QColor color = this->Options->getAxisGridColor(axes[i]);
      values.append(QVariant((double)color.redF()));
      values.append(QVariant((double)color.greenF()));
      values.append(QVariant((double)color.blueF()));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisGridColor"), values);
    }

  if(this->ModifiedData & ShowAxisLabelsModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant(
          this->Options->areAxisLabelsShowing(axes[i]) ? 1 : 0));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("ShowAxisLabels"), values);
    }

  if(this->ModifiedData & AxisLabelFontModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      QFont labelFont = this->Options->getAxisLabelFont(axes[i]);
      values.append(QVariant(labelFont.family()));
      values.append(QVariant(labelFont.pointSize()));
      values.append(QVariant(labelFont.bold() ? 1 : 0));
      values.append(QVariant(labelFont.italic() ? 1 : 0));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisLabelFont"), values);
    }

  if(this->ModifiedData & AxisLabelColorModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      QColor color = this->Options->getAxisLabelColor(axes[i]);
      values.append(QVariant((double)color.redF()));
      values.append(QVariant((double)color.greenF()));
      values.append(QVariant((double)color.blueF()));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisLabelColor"), values);
    }

  if(this->ModifiedData & AxisLabelNotationModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant(
          (int)this->Options->getAxisLabelNotation(axes[i])));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisLabelNotation"), values);
    }

  if(this->ModifiedData & AxisLabelPrecisionModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant(
          this->Options->getAxisLabelPrecision(axes[i])));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisLabelPrecision"), values);
    }

  if(this->ModifiedData & AxisScaleModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant(
          this->Options->isUsingLogScale(axes[i]) ? 1 : 0));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisScale"), values);
    }

  if(this->ModifiedData & AxisBehaviorModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant((int)this->Options->getAxisBehavior(axes[i])));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisBehavior"), values);
    }

  if ((this->ModifiedData & AxisMinimumModified) ||
    (this->ModifiedData & AxisMaximumModified))
    {
    pqChartValue minimum;
    pqChartValue maximum;
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      this->Options->getAxisMinimum(axes[i], minimum);
      values.append(QVariant(minimum.getDoubleValue()));
      this->Options->getAxisMaximum(axes[i], maximum);
      values.append(QVariant(maximum.getDoubleValue()));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisRange"), values);
    }

  if(this->ModifiedData & AxisLabelsModified)
    {
    for(i = 0; i < 4; i++)
      {
      QStringList labels;
      QList<QVariant> values;
      this->Options->getAxisLabels(axes[i], labels);
      QStringList::Iterator iter = labels.begin();
      for( ; iter != labels.end(); ++iter)
        {
        values.append(QVariant(iter->toDouble()));
        }

      pqSMAdaptor::setMultipleElementProperty(
          proxy->GetProperty(labelProperties[i]), values);
      }
    }

  if(this->ModifiedData & AxisTitleModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(this->Options->getAxisTitle(axes[i]));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisTitle"), values);
    }

  if(this->ModifiedData & AxisTitleFontModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      QFont titleFont = this->Options->getAxisTitleFont(axes[i]);
      values.append(QVariant(titleFont.family()));
      values.append(QVariant(titleFont.pointSize()));
      values.append(QVariant(titleFont.bold() ? 1 : 0));
      values.append(QVariant(titleFont.italic() ? 1 : 0));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisTitleFont"), values);
    }

  if(this->ModifiedData & AxisTitleColorModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      QColor color = this->Options->getAxisTitleColor(axes[i]);
      values.append(QVariant((double)color.redF()));
      values.append(QVariant((double)color.greenF()));
      values.append(QVariant((double)color.blueF()));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisTitleColor"), values);
    }

  if(this->ModifiedData & AxisTitleAlignmentModified)
    {
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      values.append(QVariant(this->Options->getAxisTitleAlignment(axes[i])));
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisTitleAlignment"), values);
    }

  this->ModifiedData = 0;
}

void pqChartOptionsHandler::resetChanges()
{
  if(this->ModifiedData == 0)
    {
    return;
    }

  this->initializeOptions();
  this->ModifiedData = 0;
}

void pqChartOptionsHandler::initializeOptions()
{
  if(!this->View || !this->Options)
    {
    return;
    }

  int i, j;
  vtkQtChartAxis::AxisLocation axes[] =
    {
    vtkQtChartAxis::Left,
    vtkQtChartAxis::Bottom,
    vtkQtChartAxis::Right,
    vtkQtChartAxis::Top
    };

  const char *labelProperties[] =
    {
    "LeftAxisLabels",
    "BottomAxisLabels",
    "RightAxisLabels",
    "TopAxisLabels"
    };

  // Initialize the chart options. Block the signals from the editor.
  QList<QVariant> values;
  vtkSMProxy *proxy = this->View->getProxy();
  this->Options->blockSignals(true);

  // Get the chart title parameters.
  this->Options->setTitle(pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ChartTitle")).toString());
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ChartTitleFont"));
  if(values.size() == 4)
    {
    this->Options->setTitleFont(QFont(values[0].toString(),
        values[1].toInt(), values[2].toInt() != 0 ? QFont::Bold : -1,
        values[3].toInt() != 0));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ChartTitleColor"));
  if(values.size() == 3)
    {
    this->Options->setTitleColor(QColor::fromRgbF(values[0].toDouble(),
        values[1].toDouble(), values[2].toDouble()));
    }

  this->Options->setTitleAlignment(pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ChartTitleAlignment")).toInt());

  // Get the legend parameters.
  this->Options->setLegendShowing(pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ShowLegend")).toInt() != 0);
  this->Options->setLegendLocation((vtkQtChartLegend::LegendLocation)
      pqSMAdaptor::getElementProperty(proxy->GetProperty(
      "LegendLocation")).toInt());
  this->Options->setLegendFlow((vtkQtChartLegend::ItemFlow)
      pqSMAdaptor::getElementProperty(proxy->GetProperty(
      "LegendFlow")).toInt());

  // Get the general axis parameters.
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxis"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisShowing(axes[i], values[i].toInt() != 0);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxisGrid"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisGridShowing(axes[i], values[i].toInt() != 0);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisGridType"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisGridType(axes[i],
        (vtkQtChartAxisOptions::AxisGridColor)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->Options->setAxisColor(axes[i], QColor::fromRgbF(
        values[j].toDouble(), values[j + 1].toDouble(),
        values[j + 2].toDouble()));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisGridColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->Options->setAxisGridColor(axes[i], QColor::fromRgbF(
        values[j].toDouble(), values[j + 1].toDouble(),
        values[j + 2].toDouble()));
    }

  // Get the axis label parameters.
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxisLabels"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisLabelsShowing(axes[i], values[i].toInt() != 0);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    this->Options->setAxisLabelFont(axes[i], QFont(values[j].toString(),
        values[j + 1].toInt(), values[j + 2].toInt() != 0 ? QFont::Bold : -1,
        values[j + 3].toInt() != 0));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->Options->setAxisLabelColor(axes[i], QColor::fromRgbF(
        values[j].toDouble(), values[j + 1].toDouble(),
        values[j + 2].toDouble()));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelNotation"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisLabelNotation(axes[i],
        (pqChartValue::NotationType)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelPrecision"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisLabelPrecision(axes[i], values[i].toInt());
    }

  // Get the axis layout parameters.
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisScale"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisScale(axes[i], values[i].toInt() != 0);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisBehavior"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisBehavior(axes[i],
        (vtkQtChartAxisLayer::AxisBehavior)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisMinimum"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisMinimum(axes[i], values[i].toDouble());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisMaximum"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisMaximum(axes[i], values[i].toDouble());
    }

  for(i = 0; i < 4; i++)
    {
    values = pqSMAdaptor::getMultipleElementProperty(
        proxy->GetProperty(labelProperties[i]));
    QStringList labels;
    int precision = this->Options->getAxisLabelPrecision(axes[i]);
    for(j = 0; j < values.size(); j++)
      {
      pqChartValue value = values[j].toDouble();
      labels.append(value.getString(precision));
      }

    this->Options->setAxisLabels(axes[i], labels);
    }

  // Get the axis title parameters.
  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitle"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisTitle(axes[i], values[i].toString());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    this->Options->setAxisTitleFont(axes[i], QFont(values[j].toString(),
        values[j + 1].toInt(), values[j + 2].toInt() != 0 ? QFont::Bold : -1,
        values[j + 3].toInt() != 0));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    this->Options->setAxisTitleColor(axes[i], QColor::fromRgbF(
        values[j].toDouble(), values[j + 1].toDouble(),
        values[j + 2].toDouble()));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleAlignment"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    this->Options->setAxisTitleAlignment(axes[i], values[i].toInt());
    }

  this->Options->blockSignals(false);
}


