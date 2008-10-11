/*=========================================================================

   Program: ParaView
   Module:    ClientChartViewActiveOptions.cxx

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

/// \file ClientChartViewActiveOptions.cxx
/// \date 7/27/2007

#include "ClientChartViewActiveOptions.h"

#include "pqApplicationCore.h"
#include "ClientChartViewOptions.h"
#include "pqOptionsDialog.h"
#include "pqSMAdaptor.h"
#include "ClientChartView.h"
#include "pqUndoStack.h"

#include <QEvent>
#include <QHideEvent>
#include <QString>
#include <QVariant>
#include <QWidget>

#include "vtkSMProperty.h"
#include "vtkSMProxy.h"


class ClientChartViewActiveOptionsInternal : public pqOptionsPageApplyHandler
{
public:
  enum ModifiedFlag
    {
    TitleModified = 0x00000001,
    TitleFontModified = 0x00000002,
    TitleColorModified = 0x00000004,
    TitleAlignmentModified = 0x00000008,
    ShowLegendModified = 0x00000010,
    LegendLocationModified = 0x00000020,
    LegendFlowModified = 0x00000030,
    ShowAxisModified = 0x00000080,
    ShowAxisGridModified = 0x00000100,
    AxisGridTypeModified = 0x00000200,
    AxisColorModified = 0x00000400,
    AxisGridColorModified = 0x00000800,
    ShowAxisLabelsModified = 0x00001000,
    AxisLabelFontModified = 0x00002000,
    AxisLabelColorModified = 0x00004000,
    AxisLabelNotationModified = 0x00008000,
    AxisLabelPrecisionModified = 0x00010000,
    AxisScaleModified = 0x00020000,
    AxisBehaviorModified = 0x00040000,
    AxisMinimumModified = 0x00080000,
    AxisMaximumModified = 0x00100000,
    AxisLabelsModified = 0x00200000,
    AxisTitleModified = 0x00400000,
    AxisTitleFontModified = 0x00800000,
    AxisTitleColorModified = 0x01000000,
    AxisTitleAlignmentModified = 0x02000000
    };

public:
  ClientChartViewActiveOptionsInternal();
  virtual ~ClientChartViewActiveOptionsInternal() {}

  virtual void applyChanges();
  virtual void resetChanges();

  void setOptions(ClientChartViewOptions *options);
  void setChart(ClientChartView *chart);
  void initializeOptions();
  void setModified(ModifiedFlag flag);

public:
  unsigned int ModifiedData;
  ClientChartViewOptions *Options;
  ClientChartView *Chart;
};


//----------------------------------------------------------------------------
ClientChartViewActiveOptionsInternal::ClientChartViewActiveOptionsInternal()
{
  this->ModifiedData = 0;
  this->Options = 0;
  this->Chart = 0;
}

void ClientChartViewActiveOptionsInternal::applyChanges()
{
  if(this->ModifiedData == 0 || !this->Options || !this->Chart)
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

  vtkSMProxy *proxy = this->Chart->getProxy();
  pqUndoStack *stack = pqApplicationCore::instance()->getUndoStack();
  if(stack)
    {
    stack->beginUndoSet("Chart Options");
    }

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

  if(this->ModifiedData & AxisMinimumModified)
    {
    QVariant minimum;
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      this->Options->getAxisMinimum(axes[i], minimum);
      values.append(minimum);
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisMinimum"), values);
    }

  if(this->ModifiedData & AxisMaximumModified)
    {
    QVariant maximum;
    QList<QVariant> values;
    for(i = 0; i < 4; i++)
      {
      this->Options->getAxisMaximum(axes[i], maximum);
      values.append(maximum);
      }

    pqSMAdaptor::setMultipleElementProperty(
        proxy->GetProperty("AxisMaximum"), values);
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

  if(stack)
    {
    stack->endUndoSet();
    }

  this->ModifiedData = 0;
  proxy->UpdateVTKObjects();
  this->Chart->render();
}

void ClientChartViewActiveOptionsInternal::resetChanges()
{
  if(this->ModifiedData == 0)
    {
    return;
    }

  this->initializeOptions();
  this->ModifiedData = 0;
}

void ClientChartViewActiveOptionsInternal::setOptions(ClientChartViewOptions *options)
{
  this->Options = options;
  if(this->Options)
    {
    this->Options->setApplyHandler(this);
    }
}

void ClientChartViewActiveOptionsInternal::setChart(ClientChartView *chart)
{
  this->Chart = chart;
  this->initializeOptions();
}

void ClientChartViewActiveOptionsInternal::initializeOptions()
{
  if(!this->Chart || !this->Options)
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
  vtkSMProxy *proxy = this->Chart->getProxy();
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
        (vtkQtChartAxisOptions::NotationType)values[i].toInt());
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
      QVariant value = values[j].toDouble();
      labels.append(QString::number(values[j].toDouble(),'g',precision));
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

void ClientChartViewActiveOptionsInternal::setModified(
    ClientChartViewActiveOptionsInternal::ModifiedFlag flag)
{
  this->ModifiedData |= flag;
  this->Options->sendChangesAvailable();
}


//----------------------------------------------------------------------------
ClientChartViewActiveOptions::ClientChartViewActiveOptions(QObject *parentObject)
  : pqActiveViewOptions(parentObject)
{
  this->Internal = new ClientChartViewActiveOptionsInternal();
  this->Dialog = 0;
}

ClientChartViewActiveOptions::~ClientChartViewActiveOptions()
{
  delete this->Internal;
}

void ClientChartViewActiveOptions::showOptions(pqView *view, const QString &page,
    QWidget *widgetParent)
{
  // Create the chart options dialog if necessary.
  if(!this->Dialog)
    {
    this->Dialog = new pqOptionsDialog(widgetParent);
    this->Dialog->setObjectName("ActiveChartOptions");
    this->Dialog->setWindowTitle("Chart Options");
    ClientChartViewOptions *options = new ClientChartViewOptions();
    this->Internal->setOptions(options);
    this->Dialog->addOptions(options);

    this->connect(this->Dialog, SIGNAL(finished(int)),
        this, SLOT(finishDialog(int)));
    this->connect(this->Dialog, SIGNAL(destroyed()),
        this, SLOT(cleanupDialog()));

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
        SIGNAL(axisLabelNotationChanged(vtkQtChartAxis::AxisLocation, vtkQtChartAxisOptions::NotationType)),
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
        SIGNAL(axisMinimumChanged(vtkQtChartAxis::AxisLocation, const QVariant &)),
        this, SLOT(setAxisMinimumModified()));
    this->connect(options,
        SIGNAL(axisMaximumChanged(vtkQtChartAxis::AxisLocation, const QVariant &)),
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

  // See if the view is a plot view.
  ClientChartView *chart = qobject_cast<ClientChartView *>(view);
  this->Internal->setChart(chart);
  if(chart)
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

void ClientChartViewActiveOptions::changeView(pqView *view)
{
  if(this->Dialog)
    {
    this->Internal->setChart(qobject_cast<ClientChartView *>(view));
    }
}

void ClientChartViewActiveOptions::closeOptions()
{
  if(this->Dialog && this->Internal->Chart)
    {
    this->Dialog->accept();
    this->Internal->setChart(0);
    }
}

void ClientChartViewActiveOptions::finishDialog(int result)
{
  if(result != QDialog::Accepted)
    {
    this->Dialog->setApplyNeeded(false);
    }

  emit this->optionsClosed(this);
}

void ClientChartViewActiveOptions::cleanupDialog()
{
  // If the dialog was deleted, the chart options will be deleted as
  // well, which will clean up the chart connections.
  this->Dialog = 0;
  this->Internal->setOptions(0);
  this->Internal->setChart(0);
}

void ClientChartViewActiveOptions::setTitleModified()
{
  this->Internal->setModified(ClientChartViewActiveOptionsInternal::TitleModified);
}

void ClientChartViewActiveOptions::setTitleFontModified()
{
  this->Internal->setModified(ClientChartViewActiveOptionsInternal::TitleFontModified);
}

void ClientChartViewActiveOptions::setTitleColorModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::TitleColorModified);
}

void ClientChartViewActiveOptions::setTitleAlignmentModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::TitleAlignmentModified);
}

void ClientChartViewActiveOptions::setShowLegendModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::ShowLegendModified);
}

void ClientChartViewActiveOptions::setLegendLocationModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::LegendLocationModified);
}

void ClientChartViewActiveOptions::setLegendFlowModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::LegendFlowModified);
}

void ClientChartViewActiveOptions::setShowAxisModified()
{
  this->Internal->setModified(ClientChartViewActiveOptionsInternal::ShowAxisModified);
}

void ClientChartViewActiveOptions::setShowAxisGridModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::ShowAxisGridModified);
}

void ClientChartViewActiveOptions::setAxisGridTypeModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisGridTypeModified);
}

void ClientChartViewActiveOptions::setAxisColorModified()
{
  this->Internal->setModified(ClientChartViewActiveOptionsInternal::AxisColorModified);
}

void ClientChartViewActiveOptions::setAxisGridColorModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisGridColorModified);
}

void ClientChartViewActiveOptions::setShowAxisLabelsModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::ShowAxisLabelsModified);
}

void ClientChartViewActiveOptions::setAxisLabelFontModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisLabelFontModified);
}

void ClientChartViewActiveOptions::setAxisLabelColorModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisLabelColorModified);
}

void ClientChartViewActiveOptions::setAxisLabelNotationModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisLabelNotationModified);
}

void ClientChartViewActiveOptions::setAxisLabelPrecisionModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisLabelPrecisionModified);
}

void ClientChartViewActiveOptions::setAxisScaleModified()
{
  this->Internal->setModified(ClientChartViewActiveOptionsInternal::AxisScaleModified);
}

void ClientChartViewActiveOptions::setAxisBehaviorModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisBehaviorModified);
}

void ClientChartViewActiveOptions::setAxisMinimumModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisMinimumModified);
}

void ClientChartViewActiveOptions::setAxisMaximumModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisMaximumModified);
}

void ClientChartViewActiveOptions::setAxisLabelsModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisLabelsModified);
}

void ClientChartViewActiveOptions::setAxisTitleModified()
{
  this->Internal->setModified(ClientChartViewActiveOptionsInternal::AxisTitleModified);
}

void ClientChartViewActiveOptions::setAxisTitleFontModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisTitleFontModified);
}

void ClientChartViewActiveOptions::setAxisTitleColorModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisTitleColorModified);
}

void ClientChartViewActiveOptions::setAxisTitleAlignmentModified()
{
  this->Internal->setModified(
      ClientChartViewActiveOptionsInternal::AxisTitleAlignmentModified);
}


