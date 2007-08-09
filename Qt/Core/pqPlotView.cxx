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

#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

#include <QEvent>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPrinter>
#include <QtDebug>
#include <QTimer>

#include "pqBarChartRepresentation.h"
#include "pqChartArea.h"
#include "pqChartAxis.h"
#include "pqChartContentsSpace.h"
#include "pqChartInteractor.h"
#include "pqChartInteractorSetup.h"
#include "pqChartLegend.h"
#include "pqChartLegendModel.h"
#include "pqChartMouseSelection.h"
#include "pqChartWidget.h"
#include "pqLineChartRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPlotViewHistogram.h"
#include "pqPlotViewLineChart.h"
#include "pqRepresentation.h"
#include "pqServer.h"


class pqPlotViewInternal
{
public:
  pqPlotViewInternal();
  ~pqPlotViewInternal();

  QPointer<pqChartWidget> Chart;
  QPointer<pqChartLegend> Legend;
  QPointer<pqChartMouseSelection> Selection;
  pqPlotViewHistogram *Histogram;
  pqPlotViewLineChart *LineChart;
  pqChartLegendModel *LegendModel;
  bool RenderRequestPending;
};


//----------------------------------------------------------------------------
pqPlotViewInternal::pqPlotViewInternal()
  : Chart(0), Legend(0), Selection(0)
{
  this->Histogram = 0;
  this->LineChart = 0;
  this->LegendModel = 0;
  this->RenderRequestPending = false;
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
}


//----------------------------------------------------------------------------
pqPlotView::pqPlotView(const QString& type,
  const QString& group, const QString& name, 
  vtkSMViewProxy* renModule, pqServer* server, QObject* _parent)
: Superclass(type, group, name, renModule, server, _parent)
{
  this->Internal = new pqPlotViewInternal();

  // Create the chart widget.
  this->Internal->Chart = new pqChartWidget();
  this->Internal->Chart->installEventFilter(this);

  // Get the chart area and set up the axes.
  pqChartArea *chartArea = this->Internal->Chart->getChartArea();
  chartArea->createAxis(pqChartAxis::Left);
  chartArea->createAxis(pqChartAxis::Bottom);
  chartArea->createAxis(pqChartAxis::Right);
  chartArea->createAxis(pqChartAxis::Top);

  // Set up the chart legend.
  this->Internal->LegendModel = new pqChartLegendModel(this);
  this->Internal->Legend = new pqChartLegend();
  this->Internal->Legend->setModel(this->Internal->LegendModel);

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

  if(this->Internal->LegendModel->getNumberOfEntries() == 0 &&
      this->Internal->Chart->getLegend() != 0)
    {
    // Remove the legend from the chart since it is not needed.
    this->Internal->Chart->setLegend(0);
    }
  else if(this->Internal->LegendModel->getNumberOfEntries() > 0 &&
      this->Internal->Chart->getLegend() == 0)
    {
    // Add the legend to the chart since it is needed.
    this->Internal->Chart->setLegend(this->Internal->Legend);
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
bool pqPlotView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort|| !source ||
     opPort->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID() || !sourceProxy ||
     sourceProxy->GetNumberOfParts()==0)
    {
    return false;
    }

  QString srcProxyName = source->getProxy()->GetXMLName();

  if(this->getViewType() == this->barChartType())
    {
    vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
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
    vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
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
bool pqPlotView::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::FocusIn)
    {
    emit this->focused(this);
    }

  return this->Superclass::eventFilter(caller, e);
}
