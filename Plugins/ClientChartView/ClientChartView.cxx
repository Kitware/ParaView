/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientChartView.h"

#include "ui_ClientChartView.h"

#include "vtkEventQtSlotConnect.h"
#include <vtkDataObjectTypes.h>
#include <vtkDataSetAttributes.h>
#include <vtkLookupTable.h>
#include <vtkPVDataInformation.h>
#include <vtkQtListView.h>
#include <vtkQtChartRepresentation.h>
#include <vtkQtChartView.h>
#include <vtkSmartPointer.h>
#include <vtkSMClientDeliveryRepresentationProxy.h>
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqServer.h>
#include "pqSMAdaptor.h"

#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QtDebug>
#include <QVector>
#include <QVariant>
#include <QStandardItemModel>

#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisModel.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartColorStyleGenerator.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartInteractorSetup.h"
#include "vtkQtChartInteractor.h"
#include "vtkQtChartLegend.h"
#include "vtkQtChartLegendModel.h"
#include "vtkQtChartMouseSelection.h"
#include "vtkQtChartSeriesLayer.h"
#include "vtkQtChartSeriesModel.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartSeriesSelectionHandler.h"
#include "vtkQtChartStyleManager.h"
#include "vtkQtChartTableSeriesModel.h"
#include "vtkQtChartTitle.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartMouseFunction.h"
#include "vtkQtChartMousePan.h"
#include "vtkQtChartMouseZoom.h"

#include "ChartSetupDialog.h"

////////////////////////////////////////////////////////////////////////////////////
// ClientChartView::implementation

class ClientChartView::implementation
{
public:
  implementation() :
    ChartLayer(0),
    ChartView(vtkSmartPointer<vtkQtChartView>::New()),
    ChartRepresentation(vtkQtChartRepresentation::New()),
    AxisLayoutModified(true),
    ShowLegend(true),
    VTKConnect(vtkSmartPointer<vtkEventQtSlotConnect>::New())
  {
    this->Widgets.setupUi(&this->Widget);

    // Set up the chart legend.
    this->Legend = new vtkQtChartLegend();
    this->LegendModel = this->Legend->getModel();

    // Set up the chart titles. The axis titles should be in the same
    // order as the properties: left, bottom, right, top.
    this->Title = new vtkQtChartTitle();
    this->AxisTitles.reserve(4);
    this->AxisTitles.append(new vtkQtChartTitle(Qt::Vertical));
    this->AxisTitles.append(new vtkQtChartTitle());
    this->AxisTitles.append(new vtkQtChartTitle(Qt::Vertical));
    this->AxisTitles.append(new vtkQtChartTitle());

    vtkQtChartArea *view = this->Widgets.chartWidget->getChartArea();

    // Set the chart color scheme to custom so we can use our own lookup table
    vtkQtChartStyleManager *style = view->getStyleManager();
    vtkQtChartColorStyleGenerator *gen = new vtkQtChartColorStyleGenerator(style, vtkQtChartColors::Custom);
    style->setGenerator(gen);

    // Set up the default interactor.
    // Create a new interactor and add it to the chart area.
    vtkQtChartInteractor *interactor = new vtkQtChartInteractor(view);
    view->setInteractor(interactor);

    // Set up the mouse buttons. Start with pan on the right button.
    interactor->addFunction(Qt::RightButton, new vtkQtChartMouseZoom(interactor));
    interactor->addFunction(Qt::RightButton, new vtkQtChartMouseZoomX(interactor),
        Qt::ControlModifier);
    interactor->addFunction(Qt::RightButton, new vtkQtChartMouseZoomY(interactor),
        Qt::AltModifier);

    // Add the zoom functionality to the middle button since the middle
    // button usually has the wheel, which is used for zooming.
    interactor->addFunction(Qt::MidButton, new vtkQtChartMousePan(interactor));

    // Add zoom functionality to the wheel.
    interactor->addWheelFunction(new vtkQtChartMouseZoom(interactor));
    interactor->addWheelFunction(new vtkQtChartMouseZoomX(interactor),
        Qt::ControlModifier);
    interactor->addWheelFunction(new vtkQtChartMouseZoomY(interactor),
        Qt::AltModifier);

    // Add selection to the left button.
    this->ChartMouseSelection =
        new vtkQtChartMouseSelection(interactor);
    interactor->addFunction(Qt::LeftButton, this->ChartMouseSelection);
    this->ChartSeriesSelectionHandler =
        new vtkQtChartSeriesSelectionHandler(this->ChartMouseSelection);
    this->ChartSeriesSelectionHandler->setModeNames("Chart - Series", "Chart - Points");
    this->ChartSeriesSelectionHandler->setMousePressModifiers(Qt::ShiftModifier, Qt::ControlModifier);

    // Give the chart view the chart area to draw in
    this->ChartView->SetChartView(view);
  }

  ~implementation()
  {
    if(this->ChartRepresentation)
      {
      this->ChartRepresentation->Delete();
      this->ChartRepresentation = 0;
      }

    delete this->Legend;
    delete this->Title;

    QVector<QPointer<vtkQtChartTitle> >::Iterator iter = this->AxisTitles.begin();
    for( ; iter != this->AxisTitles.end(); ++iter)
      {
      if(!iter->isNull())
        {
        delete *iter;
        }
      }
  }

  vtkQtChartRepresentation *ChartRepresentation;
  vtkQtChartSeriesLayer* ChartLayer;
  QWidget Widget;
  Ui::ClientChartView Widgets;
  vtkSmartPointer<vtkQtChartView> ChartView;

  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<vtkQtChartLegend> Legend;
  QPointer<vtkQtChartTitle> Title;
  QPointer<vtkQtChartMouseSelection> ChartMouseSelection;
  QPointer<vtkQtChartSeriesSelectionHandler> ChartSeriesSelectionHandler;
  QVector<QPointer<vtkQtChartTitle> > AxisTitles;
  vtkQtChartLegendModel *LegendModel;
  bool ShowLegend;
  bool AxisLayoutModified;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientChartView

ClientChartView::ClientChartView(
    const QString& viewmoduletype, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p) :
  pqSingleInputView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation())
{
  // Listen for axis layout property changes.
  this->Implementation->VTKConnect->Connect(
      viewmodule->GetProperty("AxisScale"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Implementation->VTKConnect->Connect(
      viewmodule->GetProperty("AxisBehavior"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Implementation->VTKConnect->Connect(
      viewmodule->GetProperty("AxisMinimum"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Implementation->VTKConnect->Connect(
      viewmodule->GetProperty("AxisMaximum"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Implementation->VTKConnect->Connect(
      viewmodule->GetProperty("LeftAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Implementation->VTKConnect->Connect(
      viewmodule->GetProperty("BottomAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Implementation->VTKConnect->Connect(
      viewmodule->GetProperty("RightAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Implementation->VTKConnect->Connect(
      viewmodule->GetProperty("TopAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));

  QObject::connect(this->Implementation->Widgets.chartWidget->getChartArea()->getContentsSpace(),
                   SIGNAL(historyPreviousAvailabilityChanged(bool)),
                   this,
                   SIGNAL(canUndoChanged(bool)));
  QObject::connect(this->Implementation->Widgets.chartWidget->getChartArea()->getContentsSpace(),
                   SIGNAL(historyNextAvailabilityChanged(bool)),
                   this,
                   SIGNAL(canRedoChanged(bool)));
}

ClientChartView::~ClientChartView()
{
  delete this->Implementation;
}

QWidget* ClientChartView::getWidget()
{
  return &this->Implementation->Widget;
}

//-----------------------------------------------------------------------------
void ClientChartView::setChart(vtkQtChartSeriesLayer *chart)
{
  this->Implementation->ChartLayer = chart;
  this->Implementation->ChartRepresentation->SetChartLayer(chart);
  this->Implementation->ChartSeriesSelectionHandler->setLayer(this->Implementation->ChartLayer);
}

//-----------------------------------------------------------------------------
vtkQtChartSeriesLayer* ClientChartView::getChart()
{
  return this->Implementation->ChartLayer;
}

bool ClientChartView::canDisplay(pqOutputPort* output_port) const
{
  if(!output_port)
    return false;

  pqPipelineSource* const source = output_port->getSource();
  if(!source)
    return false;

  if(this->getServer()->GetConnectionID() != source->getServer()->GetConnectionID())
    return false;

  vtkSMSourceProxy* source_proxy =
    vtkSMSourceProxy::SafeDownCast(source->getProxy());
  if (!source_proxy ||
     source_proxy->GetOutputPortsCreated() == 0)
    {
    return false;
    }

  const char* name = output_port->getDataClassName();
  int type = vtkDataObjectTypes::GetTypeIdFromClassName(name);
  switch(type)
    {
    case VTK_TABLE:
      return true;
    }

  return false;
}

void ClientChartView::updateRepresentation(pqRepresentation* representation)
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = 
      vtkSMClientDeliveryRepresentationProxy::SafeDownCast(representation->getProxy());
  //proxy->Update();
  vtkDataObject* output = proxy->GetOutput();

  QString keyCol, firstDataCol, lastDataCol;

  if(vtkTable *table = vtkTable::SafeDownCast(output))
    {
    // sanity check
    if(table->GetNumberOfColumns() == 0)
      {
      return;
      }

    this->Implementation->ChartRepresentation->SetFirstDataColumn(table->GetColumnName(0));
    this->Implementation->ChartRepresentation->SetLastDataColumn(table->GetColumnName(table->GetNumberOfColumns()-1));
    this->Implementation->ChartRepresentation->SetInput(output);
    this->Implementation->ChartView->AddRepresentation(this->Implementation->ChartRepresentation);
    this->Implementation->ChartView->Update();
    }
}

void ClientChartView::showRepresentation(pqRepresentation* representation)
{
  this->Implementation->ChartMouseSelection->addHandler(this->Implementation->ChartSeriesSelectionHandler);
  this->Implementation->ChartMouseSelection->setSelectionMode("Chart - Series");

  this->updateRepresentation(representation);
}

void ClientChartView::hideRepresentation(pqRepresentation* representation)
{
  // Prevent mouse events from being sent to the series selection handler 
  // (since there will be none)
  this->Implementation->ChartMouseSelection->removeHandler(this->Implementation->ChartSeriesSelectionHandler);

  this->Implementation->ChartView->RemoveRepresentation(this->Implementation->ChartRepresentation);
  this->Implementation->ChartView->Update();
}

void ClientChartView::renderInternal()
{
  vtkSMProxy *proxy = this->getProxy();
  
  if(this->visibleRepresentation())
    {
    vtkSMClientDeliveryRepresentationProxy* const repProxy = 
      vtkSMClientDeliveryRepresentationProxy::SafeDownCast(this->visibleRepresentation()->getProxy());
    repProxy->Update();

    int columnsAsSeries = vtkSMPropertyHelper(repProxy, "ColumnsAsSeries").GetAsInt();
    this->Implementation->ChartRepresentation->SetColumnsAsSeries(columnsAsSeries);
      
    if(columnsAsSeries)
      {
      if(vtkSMPropertyHelper(repProxy, "UseYArrayIndex").GetAsInt())
        {
        this->Implementation->ChartRepresentation->SetKeyColumn(0);
        }
      else
        {
        this->Implementation->ChartRepresentation->SetKeyColumn(
          vtkSMPropertyHelper(repProxy, "XAxisArrayName").GetAsString());
        }

      QString seriesText = vtkSMPropertyHelper(repProxy, "SeriesFilterText").GetAsString();
      vtkSMPropertyHelper seriesHelper(repProxy, "SeriesStatus");
      for(unsigned int i=0; i<seriesHelper.GetNumberOfElements(); i+=2)
        {
        QString series = seriesHelper.GetAsString(i);
        bool status = QVariant(seriesHelper.GetAsString(i+1)).toBool() && series.startsWith(seriesText);

        for(int j=0; j<this->Implementation->ChartLayer->getModel()->getNumberOfSeries(); ++j)
          {
          if(series == this->Implementation->ChartLayer->getModel()->getSeriesName(j))
            {
            this->Implementation->ChartLayer->getSeriesOptions(j)->setVisible(status);
            break;
            }
          }
        }
      }
    }

  // Update the chart legend.
  QList<QVariant> values;
  this->Implementation->ShowLegend = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ShowLegend")).toInt() != 0;
  if((this->Implementation->LegendModel->getNumberOfEntries() == 0 ||
      !this->Implementation->ShowLegend) && this->Implementation->Widgets.chartWidget->getLegend() != 0)
    {
    // Remove the legend from the chart since it is not needed.
    this->Implementation->Widgets.chartWidget->setLegend(0);
    }
  else if(this->Implementation->LegendModel->getNumberOfEntries() > 0 &&
      this->Implementation->ShowLegend && this->Implementation->Widgets.chartWidget->getLegend() == 0)
    {
    // Add the legend to the chart since it is needed.
    this->Implementation->Widgets.chartWidget->setLegend(this->Implementation->Legend);
    }

  this->Implementation->Legend->setLocation((vtkQtChartLegend::LegendLocation)
      pqSMAdaptor::getElementProperty(proxy->GetProperty(
      "LegendLocation")).toInt());
  this->Implementation->Legend->setFlow((vtkQtChartLegend::ItemFlow)
      pqSMAdaptor::getElementProperty(proxy->GetProperty(
      "LegendFlow")).toInt());

  // Update the chart titles.
  this->updateTitles();

  // Update the axis layout.
  if(this->Implementation->AxisLayoutModified)
    {
    this->updateAxisLayout();
    this->Implementation->AxisLayoutModified = false;
    }

  // Update the axis options.
  this->updateAxisOptions();

  // Update the zooming options.
  this->updateZoomingBehavior();

  if(pqSMAdaptor::getElementProperty(proxy->GetProperty("ResetAxes")).toBool())
    {
    this->resetAxes();
    }
}

bool ClientChartView::saveImage(int vtkNotUsed(width), int vtkNotUsed(height), const QString &filename )
{
  this->Implementation->Widgets.chartWidget->saveChart(filename);

  return true;
}

//-----------------------------------------------------------------------------
void ClientChartView::setDefaultPropertyValues()
{
  pqView::setDefaultPropertyValues();

  // Load defaults for the properties that need them.
  int i = 0;
  QList<QVariant> values;
  for(i = 0; i < 4; i++)
    {
    values.append(QVariant((double)0.0));
    values.append(QVariant((double)0.0));
    values.append(QVariant((double)0.0));
    }

  vtkSMProxy *proxy = this->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelColor"), values);
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitleColor"), values);
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
      proxy->GetProperty("AxisColor"), values);
  values.clear();
  for(i = 0; i < 4; i++)
    {
    QColor grid = Qt::lightGray;
    values.append(QVariant((double)grid.redF()));
    values.append(QVariant((double)grid.greenF()));
    values.append(QVariant((double)grid.blueF()));
    }

  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisGridColor"), values);
  QFont chartFont = this->Implementation->Widgets.chartWidget->font();
  values.clear();
  values.append(chartFont.family());
  values.append(QVariant(chartFont.pointSize()));
  values.append(QVariant(chartFont.bold() ? 1 : 0));
  values.append(QVariant(chartFont.italic() ? 1 : 0));
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("ChartTitleFont"), values);
  for(i = 0; i < 3; i++)
    {
    values.append(chartFont.family());
    values.append(QVariant(chartFont.pointSize()));
    values.append(QVariant(chartFont.bold() ? 1 : 0));
    values.append(QVariant(chartFont.italic() ? 1 : 0));
    }

  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelFont"), values);
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitleFont"), values);
}

//-----------------------------------------------------------------------------
void ClientChartView::setAxisLayoutModified()
{
  this->Implementation->AxisLayoutModified = true;
}

//-----------------------------------------------------------------------------
void ClientChartView::updateTitles()
{
  // Update the chart title.
  vtkSMProxy *proxy = this->getProxy();
  QString titleText = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ChartTitle")).toString();
  if(titleText.isEmpty() && this->Implementation->Widgets.chartWidget->getTitle() != 0)
    {
    // Remove the chart title.
    this->Implementation->Widgets.chartWidget->setTitle(0);
    }
  else if(!titleText.isEmpty() && this->Implementation->Widgets.chartWidget->getTitle() == 0)
    {
    // Add the title to the chart.
    this->Implementation->Widgets.chartWidget->setTitle(this->Implementation->Title);
    }

  this->Implementation->Title->setText(titleText);
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ChartTitleFont"));
  if(values.size() == 4)
    {
    this->Implementation->Title->setFont(QFont(values[0].toString(),
        values[1].toInt(), values[2].toInt() != 0 ? QFont::Bold : -1,
        values[3].toInt() != 0));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ChartTitleColor"));
  if(values.size() == 3)
    {
    QPalette palette = this->Implementation->Title->palette();
    palette.setColor(QPalette::Text, QColor::fromRgbF(values[0].toDouble(),
        values[1].toDouble(), values[2].toDouble()));
    this->Implementation->Title->setPalette(palette);
    }

  int alignment = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ChartTitleAlignment")).toInt();
  if(alignment == 0)
    {
    alignment = Qt::AlignLeft;
    }
  else if(alignment == 2)
    {
    alignment = Qt::AlignRight;
    }
  else
    {
    alignment = Qt::AlignCenter;
    }

  this->Implementation->Title->setTextAlignment(alignment);
  this->Implementation->Title->update();

  // Update the axis titles.
  int i, j;
  vtkQtChartAxis::AxisLocation axes[] =
    {
    vtkQtChartAxis::Left,
    vtkQtChartAxis::Bottom,
    vtkQtChartAxis::Right,
    vtkQtChartAxis::Top
    };

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitle"));
  for(i = 0; i < 4 && i < values.size(); ++i)
    {
    titleText = values[i].toString();
    if(titleText.isEmpty() &&
        this->Implementation->Widgets.chartWidget->getAxisTitle(axes[i]) != 0)
      {
      // Remove the axis title.
      this->Implementation->Widgets.chartWidget->setAxisTitle(axes[i], 0);
      }
    else if(!titleText.isEmpty() &&
        this->Implementation->Widgets.chartWidget->getAxisTitle(axes[i]) == 0)
      {
      // Add the axis title to the chart.
      this->Implementation->Widgets.chartWidget->setAxisTitle(axes[i], this->Implementation->AxisTitles[i]);
      }

    this->Implementation->AxisTitles[i]->setText(titleText);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    this->Implementation->AxisTitles[i]->setFont(QFont(values[j].toString(),
        values[j + 1].toInt(), values[j + 2].toInt() != 0 ? QFont::Bold : -1,
        values[j + 3].toInt() != 0));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    QPalette palette = this->Implementation->AxisTitles[i]->palette();
    palette.setColor(QPalette::Text, QColor::fromRgbF(values[j].toDouble(),
        values[j + 1].toDouble(), values[j + 2].toDouble()));
    this->Implementation->AxisTitles[i]->setPalette(palette);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleAlignment"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    alignment = values[i].toInt();
    if(alignment == 0)
      {
      alignment = Qt::AlignLeft;
      }
    else if(alignment == 2)
      {
      alignment = Qt::AlignRight;
      }
    else
      {
      alignment = Qt::AlignCenter;
      }

    this->Implementation->AxisTitles[i]->setTextAlignment(alignment);
    this->Implementation->AxisTitles[i]->update();
    }
}

//-----------------------------------------------------------------------------
void ClientChartView::updateAxisLayout()
{
  vtkQtChartAxisLayer *area = this->Implementation->Widgets.chartWidget->getChartArea()->getAxisLayer();
  vtkQtChartAxis *axes[] = {0, 0, 0, 0};
  vtkQtChartAxis::AxisLocation location[] =
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

  axes[0] = area->getAxis(location[0]);
  axes[1] = area->getAxis(location[1]);
  axes[2] = area->getAxis(location[2]);
  axes[3] = area->getAxis(location[3]);

  int i = 0;
  vtkSMProxy *proxy = this->getProxy();
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisScale"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    axes[i]->getOptions()->setAxisScale(values[i].toInt() != 0 ?
        vtkQtChartAxisOptions::Logarithmic : vtkQtChartAxisOptions::Linear);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisBehavior"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    area->setAxisBehavior(location[i],
        (vtkQtChartAxisLayer::AxisBehavior)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisMinimum"));
  QList<QVariant> maxValues = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisMaximum"));
  for(i = 0; i < 4 && i < values.size() && i < maxValues.size(); i++)
    {
    if(area->getAxisBehavior(location[i]) == vtkQtChartAxisLayer::BestFit)
      {
      axes[i]->setBestFitRange(values[i].toDouble(),
          maxValues[i].toDouble());
      }
    }

  for(i = 0; i < 4; i++)
    {
    if(area->getAxisBehavior(location[i]) == vtkQtChartAxisLayer::FixedInterval)
      {
      values = pqSMAdaptor::getMultipleElementProperty(
          proxy->GetProperty(labelProperties[i]));
      vtkQtChartAxisModel *model = axes[i]->getModel();
      model->startModifyingData();
      model->removeAllLabels();
      for(int j = 0; j < values.size(); j++)
        {
        model->addLabel(values[j].toDouble());
        }

      model->finishModifyingData();
      }
    }

  area->getChartArea()->updateLayout();
}

//-----------------------------------------------------------------------------
void ClientChartView::updateAxisOptions()
{
  vtkQtChartAxisLayer *area = this->Implementation->Widgets.chartWidget->getChartArea()->getAxisLayer();
  vtkQtChartAxisOptions *options[] = {0, 0, 0, 0};
  options[0] = area->getAxis(vtkQtChartAxis::Left)->getOptions();
  options[1] = area->getAxis(vtkQtChartAxis::Bottom)->getOptions();
  options[2] = area->getAxis(vtkQtChartAxis::Right)->getOptions();
  options[3] = area->getAxis(vtkQtChartAxis::Top)->getOptions();

  int i, j;
  vtkSMProxy *proxy = this->getProxy();
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxis"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    options[i]->setVisible(values[i].toInt() != 0);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxisLabels"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    options[i]->setLabelsVisible(values[i].toInt() != 0);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ShowAxisGrid"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    options[i]->setGridVisible(values[i].toInt() != 0);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    options[i]->setAxisColor(QColor::fromRgbF(values[j].toDouble(),
        values[j + 1].toDouble(), values[j + 2].toDouble()));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    options[i]->setLabelColor(QColor::fromRgbF(values[j].toDouble(),
        values[j + 1].toDouble(), values[j + 2].toDouble()));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    options[i]->setLabelFont(QFont(values[j].toString(), values[j + 1].toInt(),
        values[j + 2].toInt() != 0 ? QFont::Bold : -1,
        values[j + 3].toInt() != 0));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelPrecision"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    options[i]->setPrecision(values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisLabelNotation"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    options[i]->setNotation((vtkQtChartAxisOptions::NotationType)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisGridType"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    options[i]->setGridColorType(
        (vtkQtChartAxisOptions::AxisGridColor)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisGridColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    options[i]->setGridColor(QColor::fromRgbF(values[j].toDouble(),
        values[j + 1].toDouble(), values[j + 2].toDouble()));
    }
}

//-----------------------------------------------------------------------------
void ClientChartView::updateZoomingBehavior()
{
  vtkQtChartInteractor *interactor = this->Implementation->Widgets.chartWidget->getChartArea()->getInteractor();

  vtkSMProxy *proxy = this->getProxy();
  QString value = pqSMAdaptor::getEnumerationProperty(
      proxy->GetProperty("ZoomingBehavior")).toString();

  if(value == "Both")
    {
    interactor->setFunction(Qt::RightButton, new vtkQtChartMouseZoom(interactor));
    interactor->setWheelFunction(new vtkQtChartMouseZoom(interactor));
    }
  else if(value == "Horizontal")
    {
    interactor->setFunction(Qt::RightButton, new vtkQtChartMouseZoomX(interactor));
    interactor->setWheelFunction(new vtkQtChartMouseZoomX(interactor));
    }
  else if(value == "Vertical")
    {
    interactor->setFunction(Qt::RightButton, new vtkQtChartMouseZoomY(interactor));
    interactor->setWheelFunction(new vtkQtChartMouseZoomY(interactor));
    }
  else if(value == "Box")
    {
    interactor->setFunction(Qt::RightButton, new vtkQtChartMouseZoomBox(interactor));
    }
}


//-----------------------------------------------------------------------------
void ClientChartView::resetAxes()
{
  vtkQtChartAxisLayer *area = this->Implementation->Widgets.chartWidget->getChartArea()->getAxisLayer();
  vtkQtChartAxis *axes[] = {0, 0, 0, 0};
  vtkQtChartAxis::AxisLocation location[] =
    {
    vtkQtChartAxis::Left,
    vtkQtChartAxis::Bottom,
    vtkQtChartAxis::Right,
    vtkQtChartAxis::Top
    };

  area->getAxis(location[0])->reset();
  area->getAxis(location[1])->reset();
  area->getAxis(location[2])->reset();
  area->getAxis(location[3])->reset();
}

//-----------------------------------------------------------------------------
void ClientChartView::undo()
{
  vtkQtChartContentsSpace *space = this->Implementation->Widgets.chartWidget->getChartArea()->getContentsSpace();
  space->historyPrevious();
}

//-----------------------------------------------------------------------------
void ClientChartView::redo()
{
  vtkQtChartContentsSpace *space = this->Implementation->Widgets.chartWidget->getChartArea()->getContentsSpace();
  space->historyNext();
}

//-----------------------------------------------------------------------------
bool ClientChartView::canUndo() const
{
  vtkQtChartContentsSpace *space = this->Implementation->Widgets.chartWidget->getChartArea()->getContentsSpace();
  return space->isHistoryPreviousAvailable();
}

//-----------------------------------------------------------------------------
bool ClientChartView::canRedo() const
{
  vtkQtChartContentsSpace *space = this->Implementation->Widgets.chartWidget->getChartArea()->getContentsSpace();
  return space->isHistoryNextAvailable();
}
