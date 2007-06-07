/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewModule.cxx

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
#include "pqPlotViewModule.h"

#include "vtkPVDataInformation.h"
#include "vtkDataSet.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkSMAbstractViewModuleProxy.h"
#include "vtkSMGenericViewDisplayProxy.h"
#include "vtkSMProxy.h"
#include "vtkTimeStamp.h"

#include <QFileInfo>
#include <QImage>
#include <List>
#include <Map>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPrinter>
#include <QtDebug>
#include <QTimer>

#include "pqBarChartDisplay.h"
#include "pqChartArea.h"
#include "pqChartAxis.h"
#include "pqChartInteractor.h"
#include "pqChartSeriesOptionsGenerator.h"
#include "pqChartWidget.h"
#include "pqDisplay.h"
#include "pqHistogramChartOptions.h"
#include "pqHistogramChart.h"
#include "pqHistogramWidget.h"
#include "pqLineChart.h"
#include "pqLineChartDisplay.h"
#include "pqLineChartModel.h"
#include "pqLineChartOptions.h"
#include "pqLineChartWidget.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqVTKHistogramColor.h"
#include "pqVTKHistogramModel.h"
#include "pqVTKLineChartSeries.h"


class pqPlotViewModuleHistogram
{
public:
  pqPlotViewModuleHistogram();
  ~pqPlotViewModuleHistogram() {}

  pqBarChartDisplay *getCurrentDisplay() const;
  void setCurrentDisplay(pqBarChartDisplay *display);

  void update(bool force=false);
  bool isUpdateNeeded();

public:
  QPointer<pqHistogramChart> Layer;
  pqVTKHistogramModel *Model;
  pqVTKHistogramColor ColorScheme;

  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp MTime;
  QPointer<pqBarChartDisplay> LastUsedDisplay;
  QList<QPointer<pqBarChartDisplay> > Displays;
};


class pqPlotViewModuleLineChartSeries
{
public:
  pqPlotViewModuleLineChartSeries();
  pqPlotViewModuleLineChartSeries(int display, pqVTKLineChartSeries *model);
  pqPlotViewModuleLineChartSeries(
      const pqPlotViewModuleLineChartSeries &other);
  ~pqPlotViewModuleLineChartSeries() {}

  pqPlotViewModuleLineChartSeries &operator=(
      const pqPlotViewModuleLineChartSeries &other);

public:
  pqVTKLineChartSeries *Model;
  int ModelIndex;
  int DisplayIndex;
};


class pqPlotViewModuleLineChartItem
{
public:
  pqPlotViewModuleLineChartItem(pqLineChartDisplay *display);
  ~pqPlotViewModuleLineChartItem() {}

  bool isUpdateNeeded();

public:
  QPointer<pqLineChartDisplay> Display;
  QList<pqPlotViewModuleLineChartSeries> Series;
  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp ModifiedTime;
};


class pqPlotViewModuleLineChart
{
public:
  pqPlotViewModuleLineChart();
  ~pqPlotViewModuleLineChart();

  void update(bool force=false);

public:
  QPointer<pqLineChart> Layer;
  pqLineChartModel *Model;
  QMap<vtkSMProxy *, pqPlotViewModuleLineChartItem *> Displays;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};


class pqPlotViewModuleInternal
{
public:
  pqPlotViewModuleInternal();
  ~pqPlotViewModuleInternal();

  QPointer<pqChartWidget> Chart;
  pqPlotViewModuleHistogram *Histogram;
  pqPlotViewModuleLineChart *LineChart;
  int MaxNumberOfVisibleDisplays;
  bool RenderRequestPending;
};


//----------------------------------------------------------------------------
pqPlotViewModuleHistogram::pqPlotViewModuleHistogram()
  : Layer(0), ColorScheme(), LastUpdateTime(), MTime(), LastUsedDisplay(0),
    Displays()
{
  this->Model = 0;
}

pqBarChartDisplay *pqPlotViewModuleHistogram::getCurrentDisplay() const
{
  QList<QPointer<pqBarChartDisplay> >::ConstIterator display =
      this->Displays.begin();
  for( ; display != this->Displays.end(); ++display)
    {
    if(!display->isNull() && (*display)->isVisible())
      {
      return *display;
      }
    }

  return 0;
}

void pqPlotViewModuleHistogram::setCurrentDisplay(pqBarChartDisplay *display)
{
  // Update the lookup table.
  vtkSMProxy* lut = 0;
  if(display)
    {
    lut = pqSMAdaptor::getProxyProperty(
        display->getProxy()->GetProperty("LookupTable"));
    }

  this->ColorScheme.setMapIndexToColor(true);
  this->ColorScheme.setScalarsToColors(lut);

  if(this->LastUsedDisplay == display)
    {
    return;
    }

  this->LastUsedDisplay = display;
  this->MTime.Modified();
}

void pqPlotViewModuleHistogram::update(bool force)
{
  this->setCurrentDisplay(this->getCurrentDisplay());

  if(this->Model && (force || this->isUpdateNeeded()))
    {
    vtkDataArray *xarray = 0;
    vtkDataArray *yarray = 0;
    if(!this->LastUsedDisplay.isNull())
      {
      xarray = this->LastUsedDisplay->getXArray();
      yarray = this->LastUsedDisplay->getYArray();
      if(!xarray || !yarray)
        {
        qCritical() << "Failed to locate the data to plot on either axes.";
        }
      }

    this->LastUpdateTime.Modified();
    this->Model->setDataArrays(xarray, yarray);
    }
}

bool pqPlotViewModuleHistogram::isUpdateNeeded()
{
  bool force = true; //FIXME: until we fix thses conditions to include LUT.

  /*
  // We try to determine if we really need to update the GUI.

  // If the model has been modified since last update.
  force |= this->MTime > this->LastUpdateTime;

  // if the display has been modified since last update.
  force |= (this->LastUsedDisplay) && 
    (this->LastUsedDisplay->getProxy()->GetMTime() > 
     this->LastUpdateTime); 

  // if the data object obtained from the display has been modified 
  // since last update.
  vtkRectilinearGrid* data = this->LastUsedDisplay ?
    this->LastUsedDisplay->getClientSideData() : 0; 
  force |= (data) && (data->GetMTime() > this->LastUpdateTime);
  */

  return force;
}


//----------------------------------------------------------------------------
pqPlotViewModuleLineChartSeries::pqPlotViewModuleLineChartSeries()
{
  this->Model = 0;
  this->ModelIndex = -1;
  this->DisplayIndex = -1;
}

pqPlotViewModuleLineChartSeries::pqPlotViewModuleLineChartSeries(int display,
    pqVTKLineChartSeries *model)
{
  this->Model = model;
  this->ModelIndex = -1;
  this->DisplayIndex = display;
}

pqPlotViewModuleLineChartSeries::pqPlotViewModuleLineChartSeries(
    const pqPlotViewModuleLineChartSeries &other)
{
  this->Model = other.Model;
  this->ModelIndex = other.ModelIndex;
  this->DisplayIndex = other.DisplayIndex;
}

pqPlotViewModuleLineChartSeries &pqPlotViewModuleLineChartSeries::operator=(
    const pqPlotViewModuleLineChartSeries &other)
{
  this->Model = other.Model;
  this->ModelIndex = other.ModelIndex;
  this->DisplayIndex = other.DisplayIndex;
  return *this;
}


//----------------------------------------------------------------------------
pqPlotViewModuleLineChartItem::pqPlotViewModuleLineChartItem(
    pqLineChartDisplay *display)
  : Display(display), Series(), LastUpdateTime(), ModifiedTime()
{
}

bool pqPlotViewModuleLineChartItem::isUpdateNeeded()
{
  bool force = false;
  force = force || 
      (this->LastUpdateTime <= this->ModifiedTime);
  force = force || 
      (this->Display->getClientSideData() &&
      this->Display->getClientSideData()->GetMTime() > 
      this->LastUpdateTime);
  return force;
}


//----------------------------------------------------------------------------
pqPlotViewModuleLineChart::pqPlotViewModuleLineChart()
  : Layer(0), Displays()
{
  this->Model = 0;
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
}

pqPlotViewModuleLineChart::~pqPlotViewModuleLineChart()
{
  QMap<vtkSMProxy *, pqPlotViewModuleLineChartItem *>::Iterator iter;
  for(iter = this->Displays.begin(); iter != this->Displays.end(); ++iter)
    {
    QList<pqPlotViewModuleLineChartSeries>::Iterator series =
        iter.value()->Series.begin();
    for(; series != iter.value()->Series.end(); ++series)
      {
      delete series->Model;
      }

    delete iter.value();
    }
}

void pqPlotViewModuleLineChart::update(bool force)
{
  if(this->Model)
    {
    QMap<vtkSMProxy *, pqPlotViewModuleLineChartItem *>::Iterator jter;
    for(jter = this->Displays.begin(); jter != this->Displays.end(); ++jter)
      {
      // Remove series from the model that are no longer enabled or
      // visible.
      bool isVisible = (*jter)->Display->isVisible();
      int total = (*jter)->Display->getNumberOfYArrays();
      QList<pqPlotViewModuleLineChartSeries>::Iterator series =
          (*jter)->Series.begin();
      while(series != (*jter)->Series.end())
        {
        if(!isVisible || series->DisplayIndex >= total ||
            !(*jter)->Display->getYArrayEnabled(series->DisplayIndex))
          {
          // Remove the series from the line chart model.
          this->Model->removeSeries(series->Model);
          delete series->Model;
          series = (*jter)->Series.erase(series);
          }
        else
          {
          ++series;
          }
        }

      if(isVisible && (force || (*jter)->isUpdateNeeded()))
        {
        (*jter)->LastUpdateTime.Modified();
        vtkDataArray *xArray = (*jter)->Display->getXArray();
        if(!xArray)
          {
          qDebug() << "Failed to locate X array.";
          }

        series = (*jter)->Series.begin();
        for(int i = 0; i < total; i++)
          {
          if((*jter)->Display->getYArrayEnabled(i))
            {
            // The series list should be kept in array order.
            while(series != (*jter)->Series.end() && series->DisplayIndex < i)
              {
              ++series;
              }

            vtkDataArray *yArray = (*jter)->Display->getYArray(i);
            if(!yArray)
              {
              qDebug() << "Failed to locate Y array.";
              }

            if(xArray && yArray)
              {
              if(series == (*jter)->Series.end())
                {
                // Add the new or newly enabled series to the end.
                (*jter)->Series.append(pqPlotViewModuleLineChartSeries(i,
                    new pqVTKLineChartSeries()));
                series = (*jter)->Series.end();
                pqPlotViewModuleLineChartSeries &plot = (*jter)->Series.last();

                // Set the model arrays.
                plot.Model->setDataArrays(xArray, yArray);

                // Add the line chart series to the line chart model.
                plot.ModelIndex = this->Model->getNumberOfSeries();
                this->Model->appendSeries(plot.Model);
                }
              else if(series->DisplayIndex == i)
                {
                // Update the arrays for the series.
                series->Model->setDataArrays(xArray, yArray);
                }
              else
                {
                // Insert the series in the list.
                series = (*jter)->Series.insert(series,
                    pqPlotViewModuleLineChartSeries(i,
                    new pqVTKLineChartSeries()));

                // Set the model arrays.
                series->Model->setDataArrays(xArray, yArray);

                // Add the line chart series to the line chart model.
                series->ModelIndex = this->Model->getNumberOfSeries();
                this->Model->appendSeries(series->Model);
                }
              }
            else if(series != (*jter)->Series.end() &&
                series->DisplayIndex == i)
              {
              // Remove the series from the line chart since the arrays
              // are not valid.
              this->Model->removeSeries(series->Model);
              delete series->Model;
              series = (*jter)->Series.erase(series);
              }
            }
          }
        }
      }
    }
}


//----------------------------------------------------------------------------
pqPlotViewModuleInternal::pqPlotViewModuleInternal()
  : Chart(0)
{
  this->Histogram = 0;
  this->LineChart = 0;
  this->MaxNumberOfVisibleDisplays = -1;
  this->RenderRequestPending = false;
}

pqPlotViewModuleInternal::~pqPlotViewModuleInternal()
{
  if(!this->Chart.isNull())
    {
    delete this->Chart;
    }

  if(this->Histogram)
    {
    delete this->Histogram;
    }

  if(this->LineChart)
    {
    delete this->LineChart;
    }
}


//----------------------------------------------------------------------------
pqPlotViewModule::pqPlotViewModule(const QString& type,
  const QString& group, const QString& name, 
  vtkSMAbstractViewModuleProxy* renModule, pqServer* server, QObject* _parent)
: pqGenericViewModule(type, group, name, renModule, server, _parent)
{

  this->Internal = new pqPlotViewModuleInternal();
  if(type == this->barChartType())
    {
    pqHistogramChart *histogram = 0;
    this->Internal->Chart = pqHistogramWidget::createHistogram(0, &histogram);
    this->Internal->Histogram = new pqPlotViewModuleHistogram();
    this->Internal->Histogram->Layer = histogram;
    this->Internal->Histogram->Model = new pqVTKHistogramModel(this);
    this->Internal->Histogram->ColorScheme.setModel(
        this->Internal->Histogram->Model);
    histogram->getOptions()->setColorScheme(
        &this->Internal->Histogram->ColorScheme);
    histogram->setModel(this->Internal->Histogram->Model);
    this->Internal->MaxNumberOfVisibleDisplays = 1;
    }
  else if(type == this->XYPlotType())
    {
    pqLineChart *lineChart = 0;
    this->Internal->Chart = pqLineChartWidget::createLineChart(0, &lineChart);
    this->Internal->LineChart = new pqPlotViewModuleLineChart();
    this->Internal->LineChart->Layer = lineChart;
    lineChart->getOptions()->getGenerator()->setColorScheme(
        pqChartSeriesOptionsGenerator::Cool);
    this->Internal->LineChart->Model = new pqLineChartModel(this);
    lineChart->setModel(this->Internal->LineChart->Model);
    this->Internal->MaxNumberOfVisibleDisplays = -1;
    }
  else
    {
    qDebug() << "PlotType: " << type << " not supported yet.";
    }

  if(this->Internal->Chart)
    {
    this->Internal->Chart->setObjectName("PlotWidget");
    }
  
  QObject::connect(this, SIGNAL(displayVisibilityChanged(pqDisplay*, bool)),
    this, SLOT(visibilityChanged(pqDisplay*)));
  QObject::connect(this, SIGNAL(displayAdded(pqDisplay*)),
    this, SLOT(visibilityChanged(pqDisplay*)));

  QObject::connect(this, SIGNAL(endRender()), this, SLOT(renderInternal()));
  QObject::connect(this, SIGNAL(displayAdded(pqDisplay*)), 
    this, SLOT(addDisplay(pqDisplay*)));
  QObject::connect(this, SIGNAL(displayRemoved(pqDisplay*)), 
    this, SLOT(removeDisplay(pqDisplay*)));

  // Add the current displays to the chart.
  QList<pqDisplay*> currentDisplays = this->getDisplays();
  foreach(pqDisplay* display, currentDisplays)
    {
    this->addDisplay(display);
    }
}

//-----------------------------------------------------------------------------
pqPlotViewModule::~pqPlotViewModule()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqPlotViewModule::getWidget()
{
  return this->Internal->Chart;
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::visibilityChanged(pqDisplay* disp)
{
  if (disp->isVisible())
    {
    int max_visible = this->Internal->MaxNumberOfVisibleDisplays-1;
    int cc=0;
    QList<pqDisplay*> dislays = this->getDisplays();
    foreach(pqDisplay* d, dislays)
      {
      if (d != disp && d->isVisible())
        {
        cc++;
        if (max_visible >= 0 && cc > max_visible)
          {
          d->setVisible(false);
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::render()
{
  if (!this->Internal->RenderRequestPending)
    {
    this->Internal->RenderRequestPending = true;
    QTimer::singleShot(0, this, SLOT(delayedRender()));
    }
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::delayedRender()
{
  if (this->Internal->RenderRequestPending)
    {
    this->forceRender();
    }
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::forceRender()
{
  this->Superclass::forceRender();
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::renderInternal()
{
  this->Internal->RenderRequestPending = false;
  if(this->Internal->Histogram)
    {
    this->Internal->Histogram->update();
    }

  if(this->Internal->LineChart)
    {
    this->Internal->LineChart->update();
    }
}

//-----------------------------------------------------------------------------
vtkImageData* pqPlotViewModule::captureImage(int magnification)
{
  QPixmap grabbedPixMap = QPixmap::grabWidget(this->getWidget());
  grabbedPixMap = grabbedPixMap.scaled(grabbedPixMap.size().width()*magnification,
    grabbedPixMap.size().height()*magnification);

  // Now we need to convert this pixmap to vtkImageData.
  QImage image = grabbedPixMap.toImage();

  vtkImageData* vtkimage = vtkImageData::New();
  vtkimage->SetScalarTypeToUnsignedChar();
  vtkimage->SetNumberOfScalarComponents(3);
  vtkimage->SetDimensions(image.size().width(), image.size().height(), 1);
  vtkimage->AllocateScalars();

  QSize imgSize = image.size();

  unsigned char* data = static_cast<unsigned char*>(vtkimage->GetScalarPointer());
  for (int y=0; y < imgSize.height(); y++)
    {
    int index=(imgSize.height()-y-1) * imgSize.width()*3;
    for (int x=0; x< imgSize.width(); x++)
      {
      QRgb color = image.pixel(x, y);
      data[index++] = qRed(color);
      data[index++] = qGreen(color);
      data[index++] = qBlue(color);
      }
    }

  // Update image extents based on window position.
  int *position = this->getViewModuleProxy()->GetWindowPosition();
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
bool pqPlotViewModule::saveImage(int width, int height, 
    const QString& filename)
{
  if (width != 0 && height != 0)
    {
    this->getWidget()->resize(width, height);
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
      return false;
      }
    return true;
    }

  QPixmap grabbedPixMap = QPixmap::grabWidget(this->getWidget());
  return grabbedPixMap.save(filename);
}

//-----------------------------------------------------------------------------
bool pqPlotViewModule::canDisplaySource(pqPipelineSource* source) const
{
  if(!source || 
     source->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID())
    {
    return false;
    }

  QString srcProxyName = source->getProxy()->GetXMLName();

  if(this->getViewType() == this->barChartType())
    {
    vtkPVDataInformation* dataInfo = source->getDataInformation();
    if (dataInfo)
      {
      int extent[6];
      dataInfo->GetExtent(extent);
      int non_zero_dims = 0;
      for (int cc=0; cc < 3; cc++)
        {
        non_zero_dims += (extent[2*cc+1]-extent[2*cc]>0)? 1: 0;
        }

      return (dataInfo->GetDataClassName() == QString("vtkRectilinearGrid")) &&
        (non_zero_dims == 1);
      }
    }
  else if(this->getViewType() == this->XYPlotType())
    {
    vtkPVDataInformation* dataInfo = source->getDataInformation();
    if (dataInfo)
      {
      if (dataInfo->GetNumberOfPoints() <= 1)
        {
        // can be XY-plotted  only when number of points > 1.
        return false;
        }

      if (srcProxyName == "ProbeLine" )
        {
        return true;
        }

      int extent[6];
      dataInfo->GetExtent(extent);
      int non_zero_dims = 0;
      for (int cc=0; cc < 3; cc++)
        {
        non_zero_dims += (extent[2*cc+1]-extent[2*cc]>0)? 1: 0;
        }
      return (dataInfo->GetDataClassName() == QString("vtkRectilinearGrid")) &&
        (non_zero_dims == 1);
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::addDisplay(pqDisplay* display)
{
  pqBarChartDisplay *histogram = qobject_cast<pqBarChartDisplay *>(display);
  pqLineChartDisplay *lineChart = qobject_cast<pqLineChartDisplay *>(display);
  if(histogram)
    {
    if(this->Internal->Histogram &&
        !this->Internal->Histogram->Displays.contains(histogram))
      {
      this->Internal->Histogram->Displays.push_back(histogram);
      }
    }
  else if(lineChart &&
      lineChart->getProxy()->GetXMLName() == QString("XYPlotDisplay2"))
    {
    if(this->Internal->LineChart &&
        !this->Internal->LineChart->Displays.contains(lineChart->getProxy()))
      {
      pqPlotViewModuleLineChartItem *item =
          new pqPlotViewModuleLineChartItem(lineChart);
      this->Internal->LineChart->Displays[lineChart->getProxy()] = item;
      this->Internal->LineChart->VTKConnect->Connect(
          lineChart->getProxy(), vtkCommand::PropertyModifiedEvent,
          this, SLOT(markLineItemModified(vtkObject *)));
      item->ModifiedTime.Modified();
      }
    }
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::removeDisplay(pqDisplay* display)
{
  pqBarChartDisplay *histogram = qobject_cast<pqBarChartDisplay *>(display);
  pqLineChartDisplay *lineChart = qobject_cast<pqLineChartDisplay *>(display);
  if(histogram && this->Internal->Histogram)
    {
    this->Internal->Histogram->Displays.removeAll(histogram);
    if(histogram == this->Internal->Histogram->LastUsedDisplay)
      {
      this->Internal->Histogram->setCurrentDisplay(0);
      }
    }
  else if(lineChart && this->Internal->LineChart)
    {
    if(this->Internal->LineChart->Displays.contains(lineChart->getProxy()))
      {
      this->Internal->LineChart->VTKConnect->Disconnect(lineChart->getProxy());
      pqPlotViewModuleLineChartItem *item =
          this->Internal->LineChart->Displays.take(lineChart->getProxy());
      QList<pqPlotViewModuleLineChartSeries>::Iterator series;
      for(series = item->Series.begin(); series != item->Series.end(); ++series)
        {
        this->Internal->LineChart->Model->removeSeries(series->Model);
        delete series->Model;
        }

      delete item;
      }
    }
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::removeAllDisplays()
{
  if(this->Internal->Histogram)
    {
    this->Internal->Histogram->Displays.clear();
    this->Internal->Histogram->MTime.Modified();
    }

  if(this->Internal->LineChart)
    {
    this->Internal->LineChart->Model->removeAll();
    QMap<vtkSMProxy *, pqPlotViewModuleLineChartItem *>::Iterator iter =
        this->Internal->LineChart->Displays.begin();
    for( ; iter != this->Internal->LineChart->Displays.end(); ++iter)
      {
      this->Internal->LineChart->VTKConnect->Disconnect(iter.key());
      QList<pqPlotViewModuleLineChartSeries>::Iterator series =
          iter.value()->Series.begin();
      for(; series != iter.value()->Series.end(); ++series)
        {
        delete series->Model;
        }

      delete iter.value();
      }

    this->Internal->LineChart->Displays.clear();
    }
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::markLineItemModified(vtkObject *object)
{
  if(this->Internal->LineChart)
    {
    // Look up the line chart item using the proxy.
    vtkSMProxy *proxy = vtkSMProxy::SafeDownCast(object);
    QMap<vtkSMProxy *, pqPlotViewModuleLineChartItem *>::Iterator iter =
        this->Internal->LineChart->Displays.find(proxy);
    if(iter != this->Internal->LineChart->Displays.end())
      {
      iter.value()->ModifiedTime.Modified();
      }
    }
}

