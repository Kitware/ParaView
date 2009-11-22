/*=========================================================================

   Program: ParaView
   Module:    pqChartView.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqChartView.h"

#include "pqEventDispatcher.h"
#include "pqImageUtil.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkQtChartArea.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartView.h"
#include "vtkQtChartWidget.h"
#include "vtkSMChartViewProxy.h"
#include "vtkSMSourceProxy.h"

//-----------------------------------------------------------------------------
pqChartView::pqChartView(
  const QString& type, const QString& group, 
  const QString& name, 
  vtkSMViewProxy* viewProxy, 
  pqServer* server, 
  QObject* parentObject)
: Superclass(type, group, name, viewProxy, server, parentObject)
{
  viewProxy->GetID(); // this results in calling CreateVTKObjects().
}

//-----------------------------------------------------------------------------
pqChartView::~pqChartView()
{
}

//-----------------------------------------------------------------------------
void pqChartView::initialize()
{
  this->Superclass::initialize();

  // Set up the view undo/redo.
  vtkQtChartContentsSpace *contents =
    this->getVTKChartView()->GetChartArea()->getContentsSpace();
  this->connect(contents, SIGNAL(historyPreviousAvailabilityChanged(bool)),
    this, SIGNAL(canUndoChanged(bool)));
  this->connect(contents, SIGNAL(historyNextAvailabilityChanged(bool)),
    this, SIGNAL(canRedoChanged(bool)));
}

//-----------------------------------------------------------------------------
/// Return a widget associated with this view.
QWidget* pqChartView::getWidget()
{
  return vtkSMChartViewProxy::SafeDownCast(this->getProxy())->GetChartWidget();
}

//-----------------------------------------------------------------------------
/// Returns the internal vtkQtChartView which provide the implementation for
/// the chart rendering.
vtkQtChartView* pqChartView::getVTKChartView() const
{
  return vtkSMChartViewProxy::SafeDownCast(this->getProxy())->GetChartView();
}

//-----------------------------------------------------------------------------
/// This view does not support saving to image.
bool pqChartView::saveImage(int width, int height, const QString& filename)
{
  vtkQtChartWidget* plot_widget = 
    qobject_cast<vtkQtChartWidget*>(this->getWidget());

  QSize curSize;
  if (width != 0 && height != 0)
    {
    curSize = plot_widget->size();
    plot_widget->resize(width, height);
    }

  plot_widget->saveChart(filename);
  return true;
}

#if defined(VTK_USE_QVTK_QTOPENGL) && (QT_EDITION & QT_MODULE_OPENGL)
#include <QGLWidget>
#endif

//-----------------------------------------------------------------------------
/// Capture the view image into a new vtkImageData with the given magnification
/// and returns it. The caller is responsible for freeing the returned image.
vtkImageData* pqChartView::captureImage(int magnification)
{
  QWidget* plot_widget = this->getWidget();
  QSize curSize = plot_widget->size();
  QSize newSize = curSize*magnification;
  if (magnification > 1)
    {
    // Magnify.
    plot_widget->resize(newSize);
    }

  vtkQtChartWidget* plot_widget2 = 
    qobject_cast<vtkQtChartWidget*>(this->getWidget());
  if (plot_widget2)
    {
    // with OpenGL viewport, screenshots don't work correctly, so replace it
    // with non-openGL viewport.
    plot_widget2->getChartArea()->setUseOpenGLIfAvailable(false);
    }

  // vtkSMRenderViewProxy::CaptureWindow() ensures that render is called on the
  // view. Hence, we must explicitly call render here to be consistent.
  this->forceRender();

  // Since charts don't update the view immediately, we need to process all the
  // Qt::QueuedConnection slots which render the plots before capturing the
  // image.
  pqEventDispatcher::processEventsAndWait(0);

  QPixmap grabbedPixMap = QPixmap::grabWidget(plot_widget);

  if (plot_widget2)
    {
    plot_widget2->getChartArea()->setUseOpenGLIfAvailable(true);
    }

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
/// Capture the view image of the given size and returns it. The caller is
/// responsible for freeing the returned image.
vtkImageData* pqChartView::captureImage(const QSize& asize)
{
  QWidget* plot_widget = this->getWidget();
  QSize curSize = plot_widget->size();
  if (asize.isValid())
    {
    plot_widget->resize(asize);
    }
  vtkImageData* img = this->captureImage(1);
  if (asize.isValid())
    {
    plot_widget->resize(curSize);
    }
  return img;

}

//-----------------------------------------------------------------------------
/// Called to undo interaction.
void pqChartView::undo()
{
  vtkQtChartArea* area = this->getVTKChartView()->GetChartArea();
  area->getContentsSpace()->historyPrevious();
}

//-----------------------------------------------------------------------------
/// Called to redo interaction.
void pqChartView::redo()
{
  vtkQtChartArea* area = this->getVTKChartView()->GetChartArea();
  area->getContentsSpace()->historyNext();
}

//-----------------------------------------------------------------------------
/// Returns true if undo can be done.
bool pqChartView::canUndo() const
{
  vtkQtChartArea* area = this->getVTKChartView()->GetChartArea();
  return area->getContentsSpace()->isHistoryPreviousAvailable();
}

//-----------------------------------------------------------------------------
/// Returns true if redo can be done.
bool pqChartView::canRedo() const
{
  vtkQtChartArea* area = this->getVTKChartView()->GetChartArea();
  return area->getContentsSpace()->isHistoryNextAvailable();
}

//-----------------------------------------------------------------------------
/// Resets the zoom level to 100%.
void pqChartView::resetDisplay()
{
  vtkQtChartArea* area = this->getVTKChartView()->GetChartArea();
  area->getContentsSpace()->resetZoom();
}

//-----------------------------------------------------------------------------
/// Returns true if data on the given output port can be displayed by this view.
bool pqChartView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source ? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort || !source ||
     opPort->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID() || !sourceProxy ||
     sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  if (sourceProxy->GetHints() &&
    sourceProxy->GetHints()->FindNestedElementByName("Plotable"))
    {
    return true;
    }

  vtkPVDataInformation* dataInfo = opPort->getDataInformation();
  return (dataInfo && dataInfo->DataSetTypeIsA("vtkTable"));
}

