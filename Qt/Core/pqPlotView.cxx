/*=========================================================================

   Program: ParaView
   Module:    pqPlotView.cxx

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
#include "pqPlotView.h"

#include "vtkDataSet.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkTimeStamp.h"

#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QMap>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPrinter>
#include <QtDebug>
#include <QTimer>

#include "pqBarChartRepresentation.h"
#include "pqChartArea.h"
#include "pqChartAxis.h"
#include "pqChartInteractor.h"
#include "pqChartSeriesOptionsGenerator.h"
#include "pqChartWidget.h"
#include "pqRepresentation.h"
#include "pqHistogramChartOptions.h"
#include "pqHistogramChart.h"
#include "pqHistogramWidget.h"
#include "pqLineChart.h"
#include "pqLineChartRepresentation.h"
#include "pqLineChartModel.h"
#include "pqLineChartOptions.h"
#include "pqLineChartWidget.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqVTKHistogramColor.h"
#include "pqVTKHistogramModel.h"
#include "pqVTKLineChartSeries.h"


class pqPlotViewHistogram
{
public:
  pqPlotViewHistogram();
  ~pqPlotViewHistogram() {}

  pqBarChartRepresentation *getCurrentRepresentation() const;
  void setCurrentRepresentation(pqBarChartRepresentation *display);

  void update(bool force=false);
  bool isUpdateNeeded();

public:
  QPointer<pqHistogramChart> Layer;
  pqVTKHistogramModel *Model;
  pqVTKHistogramColor ColorScheme;

  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp MTime;
  QPointer<pqBarChartRepresentation> LastUsedRepresentation;
  QList<QPointer<pqBarChartRepresentation> > Representations;
};


class pqPlotViewLineChartSeries
{
public:
  pqPlotViewLineChartSeries();
  pqPlotViewLineChartSeries(int display, pqVTKLineChartSeries *model);
  pqPlotViewLineChartSeries(
      const pqPlotViewLineChartSeries &other);
  ~pqPlotViewLineChartSeries() {}

  pqPlotViewLineChartSeries &operator=(
      const pqPlotViewLineChartSeries &other);

public:
  pqVTKLineChartSeries *Model;
  int ModelIndex;
  int RepresentationIndex;
};


class pqPlotViewLineChartItem
{
public:
  pqPlotViewLineChartItem(pqLineChartRepresentation *display);
  ~pqPlotViewLineChartItem() {}

  bool isUpdateNeeded();

public:
  QPointer<pqLineChartRepresentation> Representation;
  QList<pqPlotViewLineChartSeries> Series;
  vtkTimeStamp LastUpdateTime;
  vtkTimeStamp ModifiedTime;
};


class pqPlotViewLineChart
{
public:
  pqPlotViewLineChart();
  ~pqPlotViewLineChart();

  void update(bool force=false);

public:
  QPointer<pqLineChart> Layer;
  pqLineChartModel *Model;
  QMap<vtkSMProxy *, pqPlotViewLineChartItem *> Representations;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};


class pqPlotViewInternal
{
public:
  pqPlotViewInternal();
  ~pqPlotViewInternal();

  QPointer<pqChartWidget> Chart;
  pqPlotViewHistogram *Histogram;
  pqPlotViewLineChart *LineChart;
  int MaxNumberOfVisibleRepresentations;
  bool RenderRequestPending;
};


//----------------------------------------------------------------------------
pqPlotViewHistogram::pqPlotViewHistogram()
  : Layer(0), ColorScheme(), LastUpdateTime(), MTime(), LastUsedRepresentation(0),
    Representations()
{
  this->Model = 0;
}

pqBarChartRepresentation *pqPlotViewHistogram::getCurrentRepresentation() const
{
  QList<QPointer<pqBarChartRepresentation> >::ConstIterator display =
      this->Representations.begin();
  for( ; display != this->Representations.end(); ++display)
    {
    if(!display->isNull() && (*display)->isVisible())
      {
      return *display;
      }
    }

  return 0;
}

void pqPlotViewHistogram::setCurrentRepresentation(pqBarChartRepresentation *display)
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

  if(this->LastUsedRepresentation == display)
    {
    return;
    }

  this->LastUsedRepresentation = display;
  this->MTime.Modified();
}

void pqPlotViewHistogram::update(bool force)
{
  this->setCurrentRepresentation(this->getCurrentRepresentation());

  if(this->Model && (force || this->isUpdateNeeded()))
    {
    vtkDataArray *xarray = 0;
    vtkDataArray *yarray = 0;
    if(!this->LastUsedRepresentation.isNull())
      {
      xarray = this->LastUsedRepresentation->getXArray();
      yarray = this->LastUsedRepresentation->getYArray();
      if(!xarray || !yarray)
        {
        qCritical() << "Failed to locate the data to plot on either axes.";
        }
      }

    this->LastUpdateTime.Modified();
    this->Model->setDataArrays(xarray, yarray);
    }
}

bool pqPlotViewHistogram::isUpdateNeeded()
{
  bool force = true; //FIXME: until we fix thses conditions to include LUT.

  /*
  // We try to determine if we really need to update the GUI.

  // If the model has been modified since last update.
  force |= this->MTime > this->LastUpdateTime;

  // if the display has been modified since last update.
  force |= (this->LastUsedRepresentation) && 
    (this->LastUsedRepresentation->getProxy()->GetMTime() > 
     this->LastUpdateTime); 

  // if the data object obtained from the display has been modified 
  // since last update.
  vtkRectilinearGrid* data = this->LastUsedRepresentation ?
    this->LastUsedRepresentation->getClientSideData() : 0; 
  force |= (data) && (data->GetMTime() > this->LastUpdateTime);
  */

  return force;
}


//----------------------------------------------------------------------------
pqPlotViewLineChartSeries::pqPlotViewLineChartSeries()
{
  this->Model = 0;
  this->ModelIndex = -1;
  this->RepresentationIndex = -1;
}

pqPlotViewLineChartSeries::pqPlotViewLineChartSeries(int display,
    pqVTKLineChartSeries *model)
{
  this->Model = model;
  this->ModelIndex = -1;
  this->RepresentationIndex = display;
}

pqPlotViewLineChartSeries::pqPlotViewLineChartSeries(
    const pqPlotViewLineChartSeries &other)
{
  this->Model = other.Model;
  this->ModelIndex = other.ModelIndex;
  this->RepresentationIndex = other.RepresentationIndex;
}

pqPlotViewLineChartSeries &pqPlotViewLineChartSeries::operator=(
    const pqPlotViewLineChartSeries &other)
{
  this->Model = other.Model;
  this->ModelIndex = other.ModelIndex;
  this->RepresentationIndex = other.RepresentationIndex;
  return *this;
}


//----------------------------------------------------------------------------
pqPlotViewLineChartItem::pqPlotViewLineChartItem(
    pqLineChartRepresentation *display)
  : Representation(display), Series(), LastUpdateTime(), ModifiedTime()
{
}

bool pqPlotViewLineChartItem::isUpdateNeeded()
{
  bool force = false;
  force = force || 
      (this->LastUpdateTime <= this->ModifiedTime);
  force = force || 
      (this->Representation->getClientSideData() &&
      this->Representation->getClientSideData()->GetMTime() > 
      this->LastUpdateTime);
  return force;
}


//----------------------------------------------------------------------------
pqPlotViewLineChart::pqPlotViewLineChart()
  : Layer(0), Representations()
{
  this->Model = 0;
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
}

pqPlotViewLineChart::~pqPlotViewLineChart()
{
  QMap<vtkSMProxy *, pqPlotViewLineChartItem *>::Iterator iter;
  for(iter = this->Representations.begin(); iter != this->Representations.end(); ++iter)
    {
    QList<pqPlotViewLineChartSeries>::Iterator series =
        iter.value()->Series.begin();
    for(; series != iter.value()->Series.end(); ++series)
      {
      delete series->Model;
      }

    delete iter.value();
    }
}

void pqPlotViewLineChart::update(bool force)
{
  if(this->Model)
    {
    QMap<vtkSMProxy *, pqPlotViewLineChartItem *>::Iterator jter;
    for(jter = this->Representations.begin(); jter != this->Representations.end(); ++jter)
      {
      // Remove series from the model that are no longer enabled or
      // visible.
      bool isVisible = (*jter)->Representation->isVisible();
      int total = (*jter)->Representation->getNumberOfYArrays();
      QList<pqPlotViewLineChartSeries>::Iterator series =
          (*jter)->Series.begin();
      while(series != (*jter)->Series.end())
        {
        if(!isVisible || series->RepresentationIndex >= total ||
            !(*jter)->Representation->getYArrayEnabled(series->RepresentationIndex))
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
        vtkDataArray *xArray = (*jter)->Representation->getXArray();
        if(!xArray)
          {
          qDebug() << "Failed to locate X array.";
          }

        series = (*jter)->Series.begin();
        for(int i = 0; i < total; i++)
          {
          if((*jter)->Representation->getYArrayEnabled(i))
            {
            // The series list should be kept in array order.
            while(series != (*jter)->Series.end() && series->RepresentationIndex < i)
              {
              ++series;
              }

            vtkDataArray *yArray = (*jter)->Representation->getYArray(i);
            if(!yArray)
              {
              qDebug() << "Failed to locate Y array.";
              }

            if(xArray && yArray)
              {
              if(series == (*jter)->Series.end())
                {
                // Add the new or newly enabled series to the end.
                (*jter)->Series.append(pqPlotViewLineChartSeries(i,
                    new pqVTKLineChartSeries()));
                series = (*jter)->Series.end();
                pqPlotViewLineChartSeries &plot = (*jter)->Series.last();

                // Set the model arrays.
                plot.Model->setDataArrays(xArray, yArray);

                // Add the line chart series to the line chart model.
                plot.ModelIndex = this->Model->getNumberOfSeries();
                this->Model->appendSeries(plot.Model);
                }
              else if(series->RepresentationIndex == i)
                {
                // Update the arrays for the series.
                series->Model->setDataArrays(xArray, yArray);
                }
              else
                {
                // Insert the series in the list.
                series = (*jter)->Series.insert(series,
                    pqPlotViewLineChartSeries(i,
                    new pqVTKLineChartSeries()));

                // Set the model arrays.
                series->Model->setDataArrays(xArray, yArray);

                // Add the line chart series to the line chart model.
                series->ModelIndex = this->Model->getNumberOfSeries();
                this->Model->appendSeries(series->Model);
                }
              }
            else if(series != (*jter)->Series.end() &&
                series->RepresentationIndex == i)
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
pqPlotViewInternal::pqPlotViewInternal()
  : Chart(0)
{
  this->Histogram = 0;
  this->LineChart = 0;
  this->MaxNumberOfVisibleRepresentations = -1;
  this->RenderRequestPending = false;
}

pqPlotViewInternal::~pqPlotViewInternal()
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
pqPlotView::pqPlotView(const QString& type,
  const QString& group, const QString& name, 
  vtkSMViewProxy* renModule, pqServer* server, QObject* _parent)
: Superclass(type, group, name, renModule, server, _parent)
{

  this->Internal = new pqPlotViewInternal();
  if(type == this->barChartType())
    {
    pqHistogramChart *histogram = 0;
    this->Internal->Chart = pqHistogramWidget::createHistogram(0, &histogram);
    this->Internal->Histogram = new pqPlotViewHistogram();
    this->Internal->Histogram->Layer = histogram;
    this->Internal->Histogram->Model = new pqVTKHistogramModel(this);
    this->Internal->Histogram->ColorScheme.setModel(
        this->Internal->Histogram->Model);
    histogram->getOptions()->setColorScheme(
        &this->Internal->Histogram->ColorScheme);
    histogram->setModel(this->Internal->Histogram->Model);
    this->Internal->MaxNumberOfVisibleRepresentations = 1;
    }
  else if(type == this->XYPlotType())
    {
    pqLineChart *lineChart = 0;
    this->Internal->Chart = pqLineChartWidget::createLineChart(0, &lineChart);
    this->Internal->LineChart = new pqPlotViewLineChart();
    this->Internal->LineChart->Layer = lineChart;
    lineChart->getOptions()->getGenerator()->setColorScheme(
        pqChartSeriesOptionsGenerator::Cool);
    this->Internal->LineChart->Model = new pqLineChartModel(this);
    lineChart->setModel(this->Internal->LineChart->Model);
    this->Internal->MaxNumberOfVisibleRepresentations = -1;
    }
  else
    {
    qDebug() << "PlotType: " << type << " not supported yet.";
    }

  if(this->Internal->Chart)
    {
    this->Internal->Chart->setObjectName("PlotWidget");
    }
  
  QObject::connect(this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(visibilityChanged(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(visibilityChanged(pqRepresentation*)));

  QObject::connect(this, SIGNAL(endRender()), this, SLOT(renderInternal()));
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)), 
    this, SLOT(addRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)), 
    this, SLOT(removeRepresentation(pqRepresentation*)));

  // Add the current displays to the chart.
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
void pqPlotView::visibilityChanged(pqRepresentation* disp)
{
  if (disp->isVisible())
    {
    int max_visible = this->Internal->MaxNumberOfVisibleRepresentations-1;
    int cc=0;
    QList<pqRepresentation*> dislays = this->getRepresentations();
    foreach(pqRepresentation* d, dislays)
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
void pqPlotView::render()
{
  if (!this->Internal->RenderRequestPending)
    {
    this->Internal->RenderRequestPending = true;
    QTimer::singleShot(0, this, SLOT(delayedRender()));
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
vtkImageData* pqPlotView::captureImage(int magnification)
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
bool pqPlotView::saveImage(int width, int height, 
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
bool pqPlotView::canDisplaySource(pqPipelineSource* source) const
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
void pqPlotView::addRepresentation(pqRepresentation* display)
{
  pqBarChartRepresentation *histogram = qobject_cast<pqBarChartRepresentation *>(display);
  pqLineChartRepresentation *lineChart = qobject_cast<pqLineChartRepresentation *>(display);
  if(histogram)
    {
    if(this->Internal->Histogram &&
        !this->Internal->Histogram->Representations.contains(histogram))
      {
      this->Internal->Histogram->Representations.push_back(histogram);
      }
    }
  else if(lineChart &&
      lineChart->getProxy()->GetXMLName() == QString("XYPlotRepresentation"))
    {
    if(this->Internal->LineChart &&
        !this->Internal->LineChart->Representations.contains(lineChart->getProxy()))
      {
      pqPlotViewLineChartItem *item =
          new pqPlotViewLineChartItem(lineChart);
      this->Internal->LineChart->Representations[lineChart->getProxy()] = item;
      this->Internal->LineChart->VTKConnect->Connect(
          lineChart->getProxy(), vtkCommand::PropertyModifiedEvent,
          this, SLOT(markLineItemModified(vtkObject *)));
      item->ModifiedTime.Modified();
      }
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::removeRepresentation(pqRepresentation* display)
{
  pqBarChartRepresentation *histogram = qobject_cast<pqBarChartRepresentation *>(display);
  pqLineChartRepresentation *lineChart = qobject_cast<pqLineChartRepresentation *>(display);
  if(histogram && this->Internal->Histogram)
    {
    this->Internal->Histogram->Representations.removeAll(histogram);
    if(histogram == this->Internal->Histogram->LastUsedRepresentation)
      {
      this->Internal->Histogram->setCurrentRepresentation(0);
      }
    }
  else if(lineChart && this->Internal->LineChart)
    {
    if(this->Internal->LineChart->Representations.contains(lineChart->getProxy()))
      {
      this->Internal->LineChart->VTKConnect->Disconnect(lineChart->getProxy());
      pqPlotViewLineChartItem *item =
          this->Internal->LineChart->Representations.take(lineChart->getProxy());
      QList<pqPlotViewLineChartSeries>::Iterator series;
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
void pqPlotView::removeAllRepresentations()
{
  if(this->Internal->Histogram)
    {
    this->Internal->Histogram->Representations.clear();
    this->Internal->Histogram->MTime.Modified();
    }

  if(this->Internal->LineChart)
    {
    this->Internal->LineChart->Model->removeAll();
    QMap<vtkSMProxy *, pqPlotViewLineChartItem *>::Iterator iter =
        this->Internal->LineChart->Representations.begin();
    for( ; iter != this->Internal->LineChart->Representations.end(); ++iter)
      {
      this->Internal->LineChart->VTKConnect->Disconnect(iter.key());
      QList<pqPlotViewLineChartSeries>::Iterator series =
          iter.value()->Series.begin();
      for(; series != iter.value()->Series.end(); ++series)
        {
        delete series->Model;
        }

      delete iter.value();
      }

    this->Internal->LineChart->Representations.clear();
    }
}

//-----------------------------------------------------------------------------
void pqPlotView::markLineItemModified(vtkObject *object)
{
  if(this->Internal->LineChart)
    {
    // Look up the line chart item using the proxy.
    vtkSMProxy *proxy = vtkSMProxy::SafeDownCast(object);
    QMap<vtkSMProxy *, pqPlotViewLineChartItem *>::Iterator iter =
        this->Internal->LineChart->Representations.find(proxy);
    if(iter != this->Internal->LineChart->Representations.end())
      {
      iter.value()->ModifiedTime.Modified();
      }
    }
}

