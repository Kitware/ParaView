/*=========================================================================

   Program: ParaView
   Module:    pqPlotView.cxx

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
#include "pqPlotView.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPointer>
#include <QPrinter>
#include <QtDebug>
#include <QTimer>
#include <QVector>

#include "pqBarChartRepresentation.h"
#include "pqChartArea.h"
#include "pqChartAxis.h"
#include "pqChartAxisModel.h"
#include "pqChartAxisOptions.h"
#include "pqChartContentsSpace.h"
#include "pqChartInteractor.h"
#include "pqChartInteractorSetup.h"
#include "pqChartLegend.h"
#include "pqChartLegendModel.h"
#include "pqChartMouseSelection.h"
#include "pqChartTitle.h"
#include "pqChartWidget.h"
#include "pqEventDispatcher.h"
#include "pqImageUtil.h"
#include "pqLineChartRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPlotViewHistogram.h"
#include "pqPlotViewLineChart.h"
#include "pqRepresentation.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"

class pqPlotViewInternal
{
public:
  pqPlotViewInternal();
  ~pqPlotViewInternal();

  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqChartWidget> Chart;
  QPointer<pqChartLegend> Legend;
  QPointer<pqChartTitle> Title;
  QVector<QPointer<pqChartTitle> > AxisTitles;
  QPointer<pqChartMouseSelection> Selection;
  pqPlotViewHistogram *Histogram;
  pqPlotViewLineChart *LineChart;
  pqChartLegendModel *LegendModel;
  bool RenderRequestPending;
  bool ShowLegend;
  bool AxisLayoutModified;
};


//----------------------------------------------------------------------------
pqPlotViewInternal::pqPlotViewInternal()
  : Chart(0), Legend(0), Title(0), AxisTitles(), Selection(0)
{
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Histogram = 0;
  this->LineChart = 0;
  this->LegendModel = 0;
  this->RenderRequestPending = false;
  this->ShowLegend = true;
  this->AxisLayoutModified = true;

  this->AxisTitles.reserve(4);
  this->AxisTitles.append(0);
  this->AxisTitles.append(0);
  this->AxisTitles.append(0);
  this->AxisTitles.append(0);
}

pqPlotViewInternal::~pqPlotViewInternal()
{
  if(!this->Chart.isNull())
    {
    delete this->Chart;
    }

  if(!this->Legend.isNull())
    {
    delete this->Legend;
    }

  if(!this->Title.isNull())
    {
    delete this->Title;
    }

  QVector<QPointer<pqChartTitle> >::Iterator iter = this->AxisTitles.begin();
  for( ; iter != this->AxisTitles.end(); ++iter)
    {
    if(!iter->isNull())
      {
      delete *iter;
      }
    }
}


//----------------------------------------------------------------------------
pqPlotView::pqPlotView(const QString& type,
  const QString& group, const QString& name, 
  vtkSMViewProxy* renModule, pqServer* server, QObject* _parent)
: Superclass(type, group, name, renModule, server, _parent)
{
  this->Internal = new pqPlotViewInternal();

  // Create the chart widget and get the chart area.
  this->Internal->Chart = new pqChartWidget();
  pqChartArea *chartArea = this->Internal->Chart->getChartArea();

  // Set up the chart legend.
  this->Internal->LegendModel = new pqChartLegendModel(this);
  this->Internal->Legend = new pqChartLegend();
  this->Internal->Legend->setModel(this->Internal->LegendModel);

  // Set up the chart titles. The axis titles should be in the same
  // order as the properties: left, bottom, right, top.
  this->Internal->Title = new pqChartTitle();
  this->Internal->AxisTitles[0] = new pqChartTitle(Qt::Vertical);
  this->Internal->AxisTitles[1] = new pqChartTitle();
  this->Internal->AxisTitles[2] = new pqChartTitle(Qt::Vertical);
  this->Internal->AxisTitles[3] = new pqChartTitle();

  // Add the appropriate layers to the chart.
  if(type == this->barChartType())
    {
    this->Internal->Histogram = new pqPlotViewHistogram(this);
    this->Internal->Histogram->initialize(chartArea);

    // Listen to the visibility changed signal for the histogram layer.
    // It can only display one set of data at a time.
    this->connect(
      this, SIGNAL(representationVisibilityChanged(pqRepresentation *, bool)),
      this->Internal->Histogram, SLOT(updateVisibility(pqRepresentation *)));
    this->connect(this, SIGNAL(representationAdded(pqRepresentation *)),
      this->Internal->Histogram, SLOT(updateVisibility(pqRepresentation *)));
    }
  else if(type == this->XYPlotType())
    {
    this->Internal->LineChart = new pqPlotViewLineChart(this);
    this->Internal->LineChart->initialize(chartArea,
        this->Internal->LegendModel);
    }
  else
    {
    qDebug() << "PlotType: " << type << " not supported yet.";
    }

  // Set up the chart interactor.
  this->Internal->Chart->setObjectName("PlotWidget");
  this->Internal->Selection = pqChartInteractorSetup::createSplitZoom(
      this->Internal->Chart->getChartArea());
  if(this->Internal->Histogram)
    {
    this->Internal->Selection->setHistogram(
        this->Internal->Histogram->getChartLayer());
    this->Internal->Selection->setSelectionMode("Histogram-Bin");
    }

  // Set up the view undo/redo.
  pqChartContentsSpace *contents =
      this->Internal->Chart->getChartArea()->getContentsSpace();
  this->connect(
      contents, SIGNAL(historyPreviousAvailabilityChanged(bool)),
      this, SIGNAL(canUndoChanged(bool)));
  this->connect(
      contents, SIGNAL(historyNextAvailabilityChanged(bool)),
      this, SIGNAL(canRedoChanged(bool)));

  this->connect(this, SIGNAL(endRender()), this, SLOT(renderInternal()));
  this->connect(this, SIGNAL(representationAdded(pqRepresentation*)), 
      this, SLOT(addRepresentation(pqRepresentation*)));
  this->connect(this, SIGNAL(representationRemoved(pqRepresentation*)), 
      this, SLOT(removeRepresentation(pqRepresentation*)));

  // Listen for axis layout property changes.
  this->Internal->VTKConnect->Connect(
      renModule->GetProperty("AxisScale"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Internal->VTKConnect->Connect(
      renModule->GetProperty("AxisBehavior"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Internal->VTKConnect->Connect(
      renModule->GetProperty("AxisMinimum"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Internal->VTKConnect->Connect(
      renModule->GetProperty("AxisMaximum"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Internal->VTKConnect->Connect(
      renModule->GetProperty("LeftAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Internal->VTKConnect->Connect(
      renModule->GetProperty("BottomAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Internal->VTKConnect->Connect(
      renModule->GetProperty("RightAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));
  this->Internal->VTKConnect->Connect(
      renModule->GetProperty("TopAxisLabels"), vtkCommand::ModifiedEvent,
      this, SLOT(setAxisLayoutModified()));

  // Add the current Representations to the chart.
  QList<pqRepresentation*> currentRepresentations = this->getRepresentations();
  foreach(pqRepresentation* display, currentRepresentations)
    {
    this->addRepresentation(display);
    }
}

//-----------------------------------------------------------------------------
pqPlotView::~pqPlotView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqPlotView::getWidget()
{
  return this->Internal->Chart;
}

//-----------------------------------------------------------------------------
void pqPlotView::setDefaultPropertyValues()
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
  QFont chartFont = this->Internal->Chart->font();
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
bool pqPlotView::supportsUndo() const
{
  return !this->Internal->Chart.isNull() &&
      this->Internal->Chart->getChartArea()->getInteractor() != 0;
}

//-----------------------------------------------------------------------------
void pqPlotView::render()
{
  if (!this->Internal->RenderRequestPending)
    {
    this->Internal->RenderRequestPending = true;
    QTimer::singleShot(0, this, SLOT(delayedRender()));
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::resetCamera()
{
  if(!this->Internal->Chart.isNull())
    {
    this->Internal->Chart->getChartArea()->getContentsSpace()->resetZoom();
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::delayedRender()
{
  if (this->Internal->RenderRequestPending)
    {
    this->forceRender();
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::forceRender()
{
  this->Superclass::forceRender();
}

//-----------------------------------------------------------------------------
void pqPlotView::renderInternal()
{
  this->Internal->RenderRequestPending = false;

  vtkSMProxy *proxy = this->getProxy();
  if (this->Internal->Histogram)
    {
    this->Internal->Histogram->update();
    }

  if (this->Internal->LineChart)
    {
    pqLineChartSeries::SequenceType type = 
      static_cast<pqLineChartSeries::SequenceType>(
        pqSMAdaptor::getElementProperty(proxy->GetProperty("Type")).toInt());
    if (type != this->Internal->LineChart->getSequenceType())
      {
      this->Internal->LineChart->setSequenceType(type);
      this->Internal->LineChart->update(true);
      }
    else
      {
      this->Internal->LineChart->update(false);
      }
    }

  // Update the chart legend.
  QList<QVariant> values;
  this->Internal->ShowLegend = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ShowLegend")).toInt() != 0;
  if((this->Internal->LegendModel->getNumberOfEntries() == 0 ||
      !this->Internal->ShowLegend) && this->Internal->Chart->getLegend() != 0)
    {
    // Remove the legend from the chart since it is not needed.
    this->Internal->Chart->setLegend(0);
    }
  else if(this->Internal->LegendModel->getNumberOfEntries() > 0 &&
      this->Internal->ShowLegend && this->Internal->Chart->getLegend() == 0)
    {
    // Add the legend to the chart since it is needed.
    this->Internal->Chart->setLegend(this->Internal->Legend);
    }

  this->Internal->Legend->setLocation((pqChartLegend::LegendLocation)
      pqSMAdaptor::getElementProperty(proxy->GetProperty(
      "LegendLocation")).toInt());
  this->Internal->Legend->setFlow((pqChartLegend::ItemFlow)
      pqSMAdaptor::getElementProperty(proxy->GetProperty(
      "LegendFlow")).toInt());

  // Update the chart titles.
  this->updateTitles();

  // Update the axis layout.
  if(this->Internal->AxisLayoutModified)
    {
    this->updateAxisLayout();
    this->Internal->AxisLayoutModified = false;
    }

  // Update the axis options.
  this->updateAxisOptions();
}

//-----------------------------------------------------------------------------
vtkImageData* pqPlotView::captureImage(const QSize& newsize)
{
  QWidget* plot_widget = this->getWidget();
  QSize curSize = plot_widget->size();
  if (newsize.isValid())
    {
    plot_widget->resize(newsize);
    }
  vtkImageData* img = this->captureImage(1);
  if (newsize.isValid())
    {
    plot_widget->resize(curSize);
    }
  return img;
}

//-----------------------------------------------------------------------------
vtkImageData* pqPlotView::captureImage(int magnification)
{
  QWidget* plot_widget = this->getWidget();
  QSize curSize = plot_widget->size();
  QSize newSize = curSize*magnification;
  if (magnification > 1)
    {
    // Magnify.
    plot_widget->resize(newSize);
    }

  // vtkSMRenderViewProxy::CaptureWindow() ensures that render is called on the
  // view. Hence, we must explicitly call render here to be consistent.
  this->forceRender();

  // Since charts don't update the view immediatetly, we need to process all the
  // Qt::QueuedConnection slots which render the plots before capturing the
  // image.
  pqEventDispatcher::processEventsAndWait(0);

  QPixmap grabbedPixMap = QPixmap::grabWidget(plot_widget);

  if (magnification > 1)
    {
    // Restore size.
    plot_widget->resize(curSize);
    }

  // Now we need to convert this pixmap to vtkImageData.
  vtkImageData* vtkimage = vtkImageData::New();
  pqImageUtil::toImageData(grabbedPixMap.toImage(), vtkimage);

  // Update image extents based on window position.
  int *position = this->getViewProxy()->GetViewPosition();
  int extents[6];
  vtkimage->GetExtent(extents);
  for (int cc=0; cc < 4; cc++)
    {
    extents[cc] += position[cc/2]*magnification;
    }
  vtkimage->SetExtent(extents);
  return vtkimage;
}

//-----------------------------------------------------------------------------
bool pqPlotView::saveImage(int width, int height, const QString& filename)
{
  QWidget* plot_widget = this->getWidget();
  QSize curSize;
  if (width != 0 && height != 0)
    {
    curSize = plot_widget->size();
    plot_widget->resize(width, height);
    }

  if (QFileInfo(filename).suffix().toLower() == "pdf")
    {
    QStringList list;
    list.push_back(filename);
    if(this->getViewType() == this->barChartType())
      {
      QPrinter printer(QPrinter::ScreenResolution);
      printer.setOutputFormat(QPrinter::PdfFormat);
      printer.setOutputFileName(filename);
      this->Internal->Chart->printChart(printer);
      /*QPixmap grab = QPixmap::grabWidget(this->Internal->Chart);
      QSize viewportSize = grab.size();
      viewportSize.scale(printer.pageRect().size(), Qt::KeepAspectRatio);
      QPainter painter(&printer);
      painter.setWindow(QRect(QPoint(0, 0), grab.size()));
      painter.setViewport(QRect(QPoint(0, 0), viewportSize));
      painter.drawPixmap(0, 0, grab);*/
      }
    else if(this->getViewType() == this->XYPlotType())
      {
      QPrinter printer(QPrinter::ScreenResolution);
      printer.setOutputFormat(QPrinter::PdfFormat);
      printer.setOutputFileName(filename);
      this->Internal->Chart->printChart(printer);
      }
    else
      {
      if (curSize.isValid())
        {
        plot_widget->resize(curSize);
        }
      return false;
      }
    if (curSize.isValid())
      {
      plot_widget->resize(curSize);
      }
    return true;
    }

  QPixmap grabbedPixMap = QPixmap::grabWidget(plot_widget);
  if (curSize.isValid())
    {
    plot_widget->resize(curSize);
    }
  return grabbedPixMap.save(filename);
}

//-----------------------------------------------------------------------------
void pqPlotView::undo()
{
  if(this->supportsUndo())
    {
    pqChartArea *area = this->Internal->Chart->getChartArea();
    area->getInteractor()->getContentsSpace()->historyPrevious();
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::redo()
{
  if(this->supportsUndo())
    {
    pqChartArea *area = this->Internal->Chart->getChartArea();
    area->getInteractor()->getContentsSpace()->historyNext();
    }
}

//-----------------------------------------------------------------------------
bool pqPlotView::canUndo() const
{
  if(this->supportsUndo())
    {
    pqChartArea *area = this->Internal->Chart->getChartArea();
    pqChartContentsSpace *space = area->getInteractor()->getContentsSpace();
    return space->isHistoryPreviousAvailable();
    }

  return false;
}

//-----------------------------------------------------------------------------
bool pqPlotView::canRedo() const
{
  if(this->supportsUndo())
    {
    pqChartArea *area = this->Internal->Chart->getChartArea();
    pqChartContentsSpace *space = area->getInteractor()->getContentsSpace();
    return space->isHistoryNextAvailable();
    }

  return false;
}

//-----------------------------------------------------------------------------
bool pqPlotView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort|| !source ||
     opPort->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID() || !sourceProxy ||
     sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  QString srcProxyName = source->getProxy()->GetXMLName();

  if(this->getViewType() == this->barChartType())
    {
    vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
    return (dataInfo && dataInfo->DataSetTypeIsA("vtkDataObject"));
    }
  else if(this->getViewType() == this->XYPlotType())
    {
    vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
    return (dataInfo && dataInfo->DataSetTypeIsA("vtkDataObject"));
    //vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
    //if (dataInfo)
    //  {
    //  if (dataInfo->GetNumberOfPoints() <= 1)
    //    {
    //    // can be XY-plotted  only when number of points > 1.
    //    return false;
    //    }

    //  if (srcProxyName == "ProbeLine" )
    //    {
    //    return true;
    //    }

    //  int extent[6];
    //  dataInfo->GetExtent(extent);
    //  int non_zero_dims = 0;
    //  for (int cc=0; cc < 3; cc++)
    //    {
    //    non_zero_dims += (extent[2*cc+1]-extent[2*cc]>0)? 1: 0;
    //    }
    //  return (dataInfo->GetDataClassName() == QString("vtkRectilinearGrid")) &&
    //    (non_zero_dims == 1);
    //  }
    }
  return false;
}

//-----------------------------------------------------------------------------
void pqPlotView::addRepresentation(pqRepresentation* display)
{
  pqBarChartRepresentation *histogram =
      qobject_cast<pqBarChartRepresentation *>(display);
  pqLineChartRepresentation *lineChart =
      qobject_cast<pqLineChartRepresentation *>(display);
  if(histogram && this->Internal->Histogram)
    {
    this->Internal->Histogram->addRepresentation(histogram);
    }
  else if(lineChart && this->Internal->LineChart &&
      lineChart->getProxy()->GetXMLName() == QString("XYPlotRepresentation"))
    {
    this->Internal->LineChart->addRepresentation(lineChart);
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::removeRepresentation(pqRepresentation* display)
{
  pqBarChartRepresentation *histogram =
      qobject_cast<pqBarChartRepresentation *>(display);
  pqLineChartRepresentation *lineChart =
      qobject_cast<pqLineChartRepresentation *>(display);
  if(histogram && this->Internal->Histogram)
    {
    this->Internal->Histogram->removeRepresentation(histogram);
    }
  else if(lineChart && this->Internal->LineChart)
    {
    this->Internal->LineChart->removeRepresentation(lineChart);
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::removeAllRepresentations()
{
  if(this->Internal->Histogram)
    {
    this->Internal->Histogram->removeAllRepresentations();
    }

  if(this->Internal->LineChart)
    {
    this->Internal->LineChart->removeAllRepresentations();
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::setAxisLayoutModified()
{
  this->Internal->AxisLayoutModified = true;
}

//-----------------------------------------------------------------------------
void pqPlotView::updateTitles()
{
  // Update the chart title.
  vtkSMProxy *proxy = this->getProxy();
  QString titleText = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ChartTitle")).toString();
  if(titleText.isEmpty() && this->Internal->Chart->getTitle() != 0)
    {
    // Remove the chart title.
    this->Internal->Chart->setTitle(0);
    }
  else if(!titleText.isEmpty() && this->Internal->Chart->getTitle() == 0)
    {
    // Add the title to the chart.
    this->Internal->Chart->setTitle(this->Internal->Title);
    }

  emit this->beginSetTitleText(this, titleText);
  this->Internal->Title->setText(titleText);
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ChartTitleFont"));
  if(values.size() == 4)
    {
    this->Internal->Title->setFont(QFont(values[0].toString(),
        values[1].toInt(), values[2].toInt() != 0 ? QFont::Bold : -1,
        values[3].toInt() != 0));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("ChartTitleColor"));
  if(values.size() == 3)
    {
    QPalette palette = this->Internal->Title->palette();
    palette.setColor(QPalette::Text, QColor::fromRgbF(values[0].toDouble(),
        values[1].toDouble(), values[2].toDouble()));
    this->Internal->Title->setPalette(palette);
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

  this->Internal->Title->setTextAlignment(alignment);
  this->Internal->Title->update();

  // Update the axis titles.
  int i, j;
  pqChartAxis::AxisLocation axes[] =
    {
    pqChartAxis::Left,
    pqChartAxis::Bottom,
    pqChartAxis::Right,
    pqChartAxis::Top
    };

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitle"));
  for(i = 0; i < 4 && i < values.size(); ++i)
    {
    titleText = values[i].toString();
    if(titleText.isEmpty() &&
        this->Internal->Chart->getAxisTitle(axes[i]) != 0)
      {
      // Remove the axis title.
      this->Internal->Chart->setAxisTitle(axes[i], 0);
      }
    else if(!titleText.isEmpty() &&
        this->Internal->Chart->getAxisTitle(axes[i]) == 0)
      {
      // Add the axis title to the chart.
      this->Internal->Chart->setAxisTitle(axes[i], this->Internal->AxisTitles[i]);
      }

    emit this->beginSetTitleText(this, titleText);
    this->Internal->AxisTitles[i]->setText(titleText);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleFont"));
  for(i = 0, j = 0; i < 4 && j + 3 < values.size(); i++, j += 4)
    {
    this->Internal->AxisTitles[i]->setFont(QFont(values[j].toString(),
        values[j + 1].toInt(), values[j + 2].toInt() != 0 ? QFont::Bold : -1,
        values[j + 3].toInt() != 0));
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisTitleColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    QPalette palette = this->Internal->AxisTitles[i]->palette();
    palette.setColor(QPalette::Text, QColor::fromRgbF(values[j].toDouble(),
        values[j + 1].toDouble(), values[j + 2].toDouble()));
    this->Internal->AxisTitles[i]->setPalette(palette);
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

    this->Internal->AxisTitles[i]->setTextAlignment(alignment);
    this->Internal->AxisTitles[i]->update();
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::updateAxisLayout()
{
  pqChartArea *area = this->Internal->Chart->getChartArea();
  pqChartAxis *axes[] = {0, 0, 0, 0};
  pqChartAxis::AxisLocation location[] =
    {
    pqChartAxis::Left,
    pqChartAxis::Bottom,
    pqChartAxis::Right,
    pqChartAxis::Top
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
    axes[i]->setScaleType(values[i].toInt() != 0 ?
        pqChartPixelScale::Logarithmic : pqChartPixelScale::Linear);
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisBehavior"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    area->setAxisBehavior(location[i],
        (pqChartArea::AxisBehavior)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisMinimum"));
  QList<QVariant> maxValues = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisMaximum"));
  for(i = 0; i < 4 && i < values.size() && i < maxValues.size(); i++)
    {
    if(area->getAxisBehavior(location[i]) == pqChartArea::BestFit)
      {
      axes[i]->setBestFitRange(pqChartValue(values[i].toDouble()),
          pqChartValue(maxValues[i].toDouble()));
      }
    }

  for(i = 0; i < 4; i++)
    {
    if(area->getAxisBehavior(location[i]) == pqChartArea::FixedInterval)
      {
      values = pqSMAdaptor::getMultipleElementProperty(
          proxy->GetProperty(labelProperties[i]));
      pqChartAxisModel *model = axes[i]->getModel();
      model->startModifyingData();
      model->removeAllLabels();
      for(int j = 0; j < values.size(); j++)
        {
        model->addLabel(pqChartValue(values[j].toDouble()));
        }

      model->finishModifyingData();
      }
    }

  area->updateLayout();
}

//-----------------------------------------------------------------------------
void pqPlotView::updateAxisOptions()
{
  pqChartArea *area = this->Internal->Chart->getChartArea();
  pqChartAxisOptions *options[] = {0, 0, 0, 0};
  options[0] = area->getAxis(pqChartAxis::Left)->getOptions();
  options[1] = area->getAxis(pqChartAxis::Bottom)->getOptions();
  options[2] = area->getAxis(pqChartAxis::Right)->getOptions();
  options[3] = area->getAxis(pqChartAxis::Top)->getOptions();

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
    options[i]->setNotation((pqChartValue::NotationType)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisGridType"));
  for(i = 0; i < 4 && i < values.size(); i++)
    {
    options[i]->setGridColorType(
        (pqChartAxisOptions::AxisGridColor)values[i].toInt());
    }

  values = pqSMAdaptor::getMultipleElementProperty(
      proxy->GetProperty("AxisGridColor"));
  for(i = 0, j = 0; i < 4 && j + 2 < values.size(); i++, j += 3)
    {
    options[i]->setGridColor(QColor::fromRgbF(values[j].toDouble(),
        values[j + 1].toDouble(), values[j + 2].toDouble()));
    }
}

