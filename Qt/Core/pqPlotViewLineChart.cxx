/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewLineChart.cxx

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

/// \file pqPlotViewLineChart.cxx
/// \date 7/13/2007

#include "pqPlotViewLineChart.h"

#include "pqChartArea.h"
#include "pqChartLegendModel.h"
#include "pqChartSeriesColorManager.h"
#include "pqChartSeriesOptionsGenerator.h"
#include "pqLineChart.h"
#include "pqLineChartModel.h"
#include "pqLineChartOptions.h"
#include "pqLineChartRepresentation.h"
#include "pqLineChartSeriesOptions.h"
#include "pqVTKLineChartSeries.h"

#include <QColor>
#include <QList>
#include <QMap>
#include <QPen>
#include <QPointer>
#include <QString>
#include <QtDebug>

#include "vtkEventQtSlotConnect.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkTimeStamp.h"


class pqPlotViewLineChartSeries
{
public:
  pqPlotViewLineChartSeries(const QString &display,
      pqVTKLineChartSeries *model);
  pqPlotViewLineChartSeries(
      const pqPlotViewLineChartSeries &other);
  ~pqPlotViewLineChartSeries() {}

  pqPlotViewLineChartSeries &operator=(
      const pqPlotViewLineChartSeries &other);

public:
  pqVTKLineChartSeries *Model;
  QString RepresentationName;
  int Chart;
  unsigned int LegendId;
};


class pqPlotViewLineChartItem
{
public:
  pqPlotViewLineChartItem(pqLineChartRepresentation *display);
  ~pqPlotViewLineChartItem() {}

  bool isUpdateNeeded();
  bool setDataType(int dataType);

public:
  QPointer<pqLineChartRepresentation> Representation;
  QList<pqPlotViewLineChartSeries> Series;
  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp ModifiedTime;
  int DataType;
};


class pqPlotViewLineChartInternal
{
public:
  pqPlotViewLineChartInternal();
  ~pqPlotViewLineChartInternal() {}

  QPointer<pqLineChart> Layer[4];
  pqLineChartModel *Model[4];
  QMap<vtkSMProxy *, pqPlotViewLineChartItem *> Representations;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqChartLegendModel *Legend;
};


//----------------------------------------------------------------------------
pqPlotViewLineChartSeries::pqPlotViewLineChartSeries(
    const QString &display, pqVTKLineChartSeries *model)
  : RepresentationName(display)
{
  this->Model = model;
  this->Chart = -1;
  this->LegendId = 0;
}

pqPlotViewLineChartSeries::pqPlotViewLineChartSeries(
    const pqPlotViewLineChartSeries &other)
  : RepresentationName(other.RepresentationName)
{
  this->Model = other.Model;
  this->Chart = other.Chart;
  this->LegendId = other.LegendId;
}

pqPlotViewLineChartSeries &pqPlotViewLineChartSeries::operator=(
    const pqPlotViewLineChartSeries &other)
{
  this->Model = other.Model;
  this->RepresentationName = other.RepresentationName;
  this->Chart = other.Chart;
  this->LegendId = other.LegendId;
  return *this;
}


//----------------------------------------------------------------------------
pqPlotViewLineChartItem::pqPlotViewLineChartItem(
    pqLineChartRepresentation *display)
  : Representation(display), Series(), LastUpdateTime(), ModifiedTime()
{
  this->DataType = 0;
}

bool pqPlotViewLineChartItem::isUpdateNeeded()
{
  bool updateNeeded = this->LastUpdateTime <= this->ModifiedTime;
  vtkRectilinearGrid *data = this->Representation->getClientSideData();
  updateNeeded = updateNeeded || (data &&
      data->GetMTime() > this->LastUpdateTime);
  return updateNeeded;
}

bool pqPlotViewLineChartItem::setDataType(int dataType)
{
  if(this->DataType != dataType)
    {
    this->DataType = dataType;
    return true;
    }

  return false;
}


//----------------------------------------------------------------------------
pqPlotViewLineChartInternal::pqPlotViewLineChartInternal()
  : Representations()
{
  this->Layer[pqPlotViewLineChart::BottomLeft] = 0;
  this->Layer[pqPlotViewLineChart::BottomRight] = 0;
  this->Layer[pqPlotViewLineChart::TopLeft] = 0;
  this->Layer[pqPlotViewLineChart::TopRight] = 0;
  this->Model[pqPlotViewLineChart::BottomLeft] = 0;
  this->Model[pqPlotViewLineChart::BottomRight] = 0;
  this->Model[pqPlotViewLineChart::TopLeft] = 0;
  this->Model[pqPlotViewLineChart::TopRight] = 0;
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Legend = 0;
}


//----------------------------------------------------------------------------
pqPlotViewLineChart::pqPlotViewLineChart(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqPlotViewLineChartInternal();
}

pqPlotViewLineChart::~pqPlotViewLineChart()
{
  QMap<vtkSMProxy *, pqPlotViewLineChartItem *>::Iterator iter =
      this->Internal->Representations.begin();
  for( ; iter != this->Internal->Representations.end(); ++iter)
    {
    QList<pqPlotViewLineChartSeries>::Iterator series =
        iter.value()->Series.begin();
    for( ; series != iter.value()->Series.end(); ++series)
      {
      delete series->Model;
      }

    delete iter.value();
    }

  delete this->Internal;
}

void pqPlotViewLineChart::initialize(pqChartArea *chartArea,
    pqChartLegendModel *legend)
{
  if(this->Internal->Model[pqPlotViewLineChart::BottomLeft])
    {
    return;
    }

  // Save the legend model for later.
  this->Internal->Legend = legend;

  // Add a line chart layer for each of the possible axes
  // configurations. Add the layers in reverse order to keep the
  // bottom-left chart on top.
  int i = 3;
  for( ; i >= 0; i--)
    {
    this->Internal->Layer[i] = new pqLineChart(chartArea);
    if(i == pqPlotViewLineChart::BottomLeft)
      {
      this->Internal->Layer[i]->setAxes(
          chartArea->getAxis(pqChartAxis::Bottom),
          chartArea->getAxis(pqChartAxis::Left));
      }
    else if(i == pqPlotViewLineChart::BottomRight)
      {
      this->Internal->Layer[i]->setAxes(
          chartArea->getAxis(pqChartAxis::Bottom),
          chartArea->getAxis(pqChartAxis::Right));
      }
    else if(i == pqPlotViewLineChart::TopLeft)
      {
      this->Internal->Layer[i]->setAxes(chartArea->getAxis(pqChartAxis::Top),
          chartArea->getAxis(pqChartAxis::Left));
      }
    else if(i == pqPlotViewLineChart::TopRight)
      {
      this->Internal->Layer[i]->setAxes(chartArea->getAxis(pqChartAxis::Top),
          chartArea->getAxis(pqChartAxis::Right));
      }

    this->Internal->Model[i] = new pqLineChartModel(this);
    this->Internal->Layer[i]->setModel(
        this->Internal->Model[i]);
    chartArea->addLayer(this->Internal->Layer[i]);
    }

  // Set up the color options for the line charts. They should all
  // share the same series color manager.
  pqChartSeriesColorManager *manager =
      this->Internal->Layer[0]->getOptions()->getSeriesColorManager();
  manager->getGenerator()->setColorScheme(
      pqChartSeriesOptionsGenerator::Spectrum);
  for(i = 1; i < 4; i++)
    {
    this->Internal->Layer[i]->getOptions()->setSeriesColorManager(manager);
    }
}

void pqPlotViewLineChart::update(bool force)
{
  if(!this->Internal->Model[pqPlotViewLineChart::BottomLeft])
    {
    return;
    }

  QMap<vtkSMProxy *, pqPlotViewLineChartItem *>::Iterator jter =
      this->Internal->Representations.begin();
  for( ; jter != this->Internal->Representations.end(); ++jter)
    {
    if(!(*jter)->isUpdateNeeded() && !force)
      {
      continue;
      }

    // Update the display data.
    (*jter)->Representation->updateSeries();
    bool typeChanged = (*jter)->setDataType(
        (*jter)->Representation->getAttributeType());
    bool isVisible = (*jter)->Representation->isVisible();
    vtkDataArray *yArray = 0;
    vtkDataArray *xArray = (*jter)->Representation->getXArray();
    if(!xArray && isVisible)
      {
      qDebug() << "Failed to locate X array.";
      }

    // First, remove or update the current model series.
    QStringList displayNames;
    QList<pqPlotViewLineChartSeries>::Iterator series =
        (*jter)->Series.begin();
    while(series != (*jter)->Series.end())
      {
      // Remove the series if the data type has changed, the display
      // is not visible, or the series is not enabled.
      int index = (*jter)->Representation->getSeriesIndex(
          series->RepresentationName);
      if(typeChanged || !isVisible ||
          !(*jter)->Representation->isSeriesEnabled(index))
        {
        // Remove the series from the legend if needed.
        if(series->LegendId != 0)
          {
          this->Internal->Legend->removeEntry(
              this->Internal->Legend->getIndexForId(series->LegendId));
          series->LegendId = 0;
          }

        this->Internal->Model[series->Chart]->removeSeries(series->Model);
        delete series->Model;
        series = (*jter)->Series.erase(series);
        }
      else
        {
        yArray = (*jter)->Representation->getYArray(index);
        if(!yArray)
          {
          qDebug() << "Failed to locate Y array.";
          }

        if(xArray && yArray)
          {
          // Move the series if needed.
          int axesIndex = (*jter)->Representation->getSeriesAxesIndex(index);
          if(axesIndex != series->Chart)
            {
            this->Internal->Model[series->Chart]->removeSeries(series->Model);
            series->Chart = axesIndex;
            this->Internal->Model[series->Chart]->appendSeries(series->Model);
            }

          // Update the arrays and options for the series.
          series->Model->setMaskArrays(
            (*jter)->Representation->getXMaskArray(),
            (*jter)->Representation->getYMaskArray(index));
          series->Model->setDataArrays(xArray, yArray);
          pqLineChartSeriesOptions *options =
              this->Internal->Layer[series->Chart]->getOptions()->getSeriesOptions(
              this->Internal->Model[series->Chart]->getIndexOf(series->Model));
          QPen seriesPen;
          options->getPen(seriesPen);
          QColor color;
          (*jter)->Representation->getSeriesColor(index, color);
          seriesPen.setColor(color);
          seriesPen.setWidth(
              (*jter)->Representation->getSeriesThickness(index));
          seriesPen.setStyle((*jter)->Representation->getSeriesStyle(index));
          options->setPen(seriesPen);

          // Update the legend status for the series.
          if((*jter)->Representation->isSeriesInLegend(index))
            {
            QString label;
            (*jter)->Representation->getSeriesLabel(index, label);
            if(series->LegendId == 0)
              {
              // Add the series to the legend.
              series->LegendId = this->Internal->Legend->addEntry(
                  pqChartLegendModel::generateLineIcon(seriesPen), label);
              }
            else
              {
              // Update the legend label and icon in case they changed.
              int legendIndex = this->Internal->Legend->getIndexForId(
                  series->LegendId);
              this->Internal->Legend->setIcon(legendIndex,
                  pqChartLegendModel::generateLineIcon(seriesPen));
              this->Internal->Legend->setText(legendIndex, label);
              }
            }
          else if(series->LegendId != 0)
            {
            // Remove the series from the legend.
            this->Internal->Legend->removeEntry(
                this->Internal->Legend->getIndexForId(series->LegendId));
            series->LegendId = 0;
            }

          displayNames.append(series->RepresentationName);
          ++series;
          }
        else
          {
          // Remove the series from the legend if needed.
          if(series->LegendId != 0)
            {
            this->Internal->Legend->removeEntry(
                this->Internal->Legend->getIndexForId(series->LegendId));
            series->LegendId = 0;
            }

          // Remove the series if the x or y array are null.
          this->Internal->Model[series->Chart]->removeSeries(series->Model);
          delete series->Model;
          series = (*jter)->Series.erase(series);
          }
        }
      }

    // Next, add new series to the chart.
    if(isVisible)
      {
      (*jter)->Representation->beginSeriesChanges();
      series = (*jter)->Series.begin();
      int total = (*jter)->Representation->getNumberOfSeries();
      for(int i = 0; i < total; i++)
        {
        if((*jter)->Representation->isSeriesEnabled(i))
          {
          QString name;
          (*jter)->Representation->getSeriesName(i, name);
          if(displayNames.contains(name))
            {
            continue;
            }

          yArray = (*jter)->Representation->getYArray(i);
          if(!xArray || !yArray)
            {
            if(!yArray)
              {
              qDebug() << "Failed to locate Y array.";
              }

            continue;
            }

          // The series list should be kept in alphabetical order.
          while(series != (*jter)->Series.end() &&
              series->RepresentationName.compare(name) <= 0)
            {
            ++series;
            }

          pqPlotViewLineChartSeries *plot = 0;
          if(series == (*jter)->Series.end())
            {
            // Add the new or newly enabled series to the end.
            (*jter)->Series.append(pqPlotViewLineChartSeries(
                name, new pqVTKLineChartSeries()));
            series = (*jter)->Series.end();
            plot = &(*jter)->Series.last();
            }
          else
            {
            // Insert the series in the list.
            series = (*jter)->Series.insert(series,
                pqPlotViewLineChartSeries(name,
                new pqVTKLineChartSeries()));
            plot = &(*series);
            }

          // Set the model arrays.
          plot->Model->setMaskArrays(
            (*jter)->Representation->getXMaskArray(),
            (*jter)->Representation->getYMaskArray(i));
          plot->Model->setDataArrays(xArray, yArray);

          // Add the line chart series to the line chart model.
          plot->Chart = (*jter)->Representation->getSeriesAxesIndex(i);
          int index = this->Internal->Model[plot->Chart]->getNumberOfSeries();
          this->Internal->Model[plot->Chart]->appendSeries(plot->Model);

          // Update the series options.
          bool changedOptions = false;
          pqLineChartSeriesOptions *options =
              this->Internal->Layer[plot->Chart]->getOptions()->getSeriesOptions(index);
          QPen seriesPen;
          options->getPen(seriesPen);
          if((*jter)->Representation->isSeriesColorSet(i))
            {
            // Update the line color to match the set color.
            QColor color;
            (*jter)->Representation->getSeriesColor(i, color);
            seriesPen.setColor(color);
            changedOptions = true;
            }
          else
            {
            // Assign the chart selected color to the property.
            (*jter)->Representation->setSeriesColor(i, seriesPen.color());
            }

          if((*jter)->Representation->isSeriesStyleSet(i))
            {
            // Update the line style to match the set style.
            seriesPen.setStyle((*jter)->Representation->getSeriesStyle(i));
            changedOptions = true;
            }
          else
            {
            // Assign the chart selected style to the property.
            (*jter)->Representation->setSeriesStyle(i, seriesPen.style());
            }

          int thickness = (*jter)->Representation->getSeriesThickness(i);
          if(thickness != seriesPen.width())
            {
            seriesPen.setWidth(thickness);
            changedOptions = true;
            }

          if(changedOptions)
            {
            options->setPen(seriesPen);
            }

          // Add the series to the legend if needed.
          if((*jter)->Representation->isSeriesInLegend(i))
            {
            (*jter)->Representation->getSeriesLabel(i, name);
            plot->LegendId = this->Internal->Legend->addEntry(
                pqChartLegendModel::generateLineIcon(seriesPen), name);
            }
          }
        }

      (*jter)->Representation->endSeriesChanges();
      }

    (*jter)->LastUpdateTime.Modified();
    }
}

void pqPlotViewLineChart::addRepresentation(
    pqLineChartRepresentation *lineChart)
{
  if(lineChart &&
      !this->Internal->Representations.contains(lineChart->getProxy()))
    {
    pqPlotViewLineChartItem *item = new pqPlotViewLineChartItem(lineChart);
    this->Internal->Representations[lineChart->getProxy()] = item;
    this->Internal->VTKConnect->Connect(
        lineChart->getProxy(), vtkCommand::PropertyModifiedEvent,
        this, SLOT(markLineItemModified(vtkObject *)));
    item->ModifiedTime.Modified();
    }
}

void pqPlotViewLineChart::removeRepresentation(
    pqLineChartRepresentation *lineChart)
{
  if(lineChart &&
      this->Internal->Representations.contains(lineChart->getProxy()))
    {
    this->Internal->VTKConnect->Disconnect(lineChart->getProxy());
    pqPlotViewLineChartItem *item =
        this->Internal->Representations.take(lineChart->getProxy());
    QList<pqPlotViewLineChartSeries>::Iterator series =
        item->Series.begin();
    for( ; series != item->Series.end(); ++series)
      {
      // Remove the series from the legend if needed.
      if(series->LegendId != 0)
        {
        this->Internal->Legend->removeEntry(
            this->Internal->Legend->getIndexForId(series->LegendId));
        series->LegendId = 0;
        }

      this->Internal->Model[series->Chart]->removeSeries(series->Model);
      delete series->Model;
      }

    delete item;
    }
}

void pqPlotViewLineChart::removeAllRepresentations()
{
  for(int i = 0; i < 4; i++)
    {
    this->Internal->Model[i]->removeAll();
    }

  QMap<vtkSMProxy *, pqPlotViewLineChartItem *>::Iterator iter =
      this->Internal->Representations.begin();
  for( ; iter != this->Internal->Representations.end(); ++iter)
    {
    this->Internal->VTKConnect->Disconnect(iter.key());
    QList<pqPlotViewLineChartSeries>::Iterator series =
        iter.value()->Series.begin();
    for( ; series != iter.value()->Series.end(); ++series)
      {
      // Remove the series from the legend if needed.
      if(series->LegendId != 0)
        {
        this->Internal->Legend->removeEntry(
            this->Internal->Legend->getIndexForId(series->LegendId));
        series->LegendId = 0;
        }

      delete series->Model;
      }

    delete iter.value();
    }

  this->Internal->Representations.clear();
}

void pqPlotViewLineChart::markLineItemModified(vtkObject *object)
{
  // Look up the line chart item using the proxy.
  vtkSMProxy *proxy = vtkSMProxy::SafeDownCast(object);
  QMap<vtkSMProxy *, pqPlotViewLineChartItem *>::Iterator iter =
      this->Internal->Representations.find(proxy);
  if(iter != this->Internal->Representations.end())
    {
    iter.value()->ModifiedTime.Modified();
    }
}


