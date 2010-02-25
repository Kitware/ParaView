/*=========================================================================

   Program: ParaView
   Module:    pqContextView.cxx

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
#include "pqContextView.h"

#include "pqEventDispatcher.h"
#include "pqImageUtil.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqImageUtil.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"

#include "vtkContextView.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMXYChartViewProxy.h"
#include "vtkChartXY.h"
#include "vtkSMSourceProxy.h"

#include "QVTKWidget.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqContextView::pqContextView(
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
pqContextView::~pqContextView()
{
}

//-----------------------------------------------------------------------------
void pqContextView::initialize()
{
  this->Superclass::initialize();
}

//-----------------------------------------------------------------------------
/// Return a widget associated with this view.
QWidget* pqContextView::getWidget()
{
  vtkSMContextViewProxy *proxy =
      vtkSMContextViewProxy::SafeDownCast(this->getProxy());
  return proxy ? proxy->GetChartWidget() : NULL;
}

//-----------------------------------------------------------------------------
/// Returns the internal vtkChartView that provides the implementation for
/// the chart rendering.
vtkContextView* pqContextView::getVTKContextView() const
{
  return vtkSMContextViewProxy::SafeDownCast(this->getProxy())->GetChartView();
}

//-----------------------------------------------------------------------------
vtkSMContextViewProxy* pqContextView::getContextViewProxy() const
{
  return vtkSMContextViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
/// This view does not support saving to image.
bool pqContextView::saveImage(int width, int height, const QString& filename)
{
  QWidget* vtkwidget = this->getWidget();
  QSize cursize = vtkwidget->size();
  QSize fullsize = QSize(width, height);
  QSize newsize = cursize;
  int magnification = 1;
  if (width>0 && height>0)
    {
    magnification = pqView::computeMagnification(fullsize, newsize);
    vtkwidget->resize(newsize);
    }
  this->render();

  int error_code = vtkErrorCode::UnknownError;
  vtkImageData* vtkimage = this->captureImage(magnification);
  if (vtkimage)
    {
    error_code = pqImageUtil::saveImage(vtkimage, filename);
    vtkimage->Delete();
    }

  switch (error_code)
    {
    case vtkErrorCode::UnrecognizedFileTypeError:
      qCritical() << "Failed to determine file type for file:"
        << filename.toAscii().data();
      break;
    case vtkErrorCode::NoError:
      // success.
      break;
    default:
      qCritical() << "Failed to save image.";
    }

  if (width>0 && height>0)
    {
    vtkwidget->resize(newsize);
    vtkwidget->resize(cursize);
    this->render();
    }
  return (error_code == vtkErrorCode::NoError);
}

//-----------------------------------------------------------------------------
/// Capture the view image into a new vtkImageData with the given magnification
/// and returns it. The caller is responsible for freeing the returned image.
vtkImageData* pqContextView::captureImage(int magnification)
{
  if (this->getWidget()->isVisible())
    {
    return this->getContextViewProxy()->CaptureWindow(magnification);
    }

  // Don't return any image when the view is not visible.
  return NULL;
}

//-----------------------------------------------------------------------------
/// Called to undo interaction.
void pqContextView::undo()
{
}

//-----------------------------------------------------------------------------
/// Called to redo interaction.
void pqContextView::redo()
{
}

//-----------------------------------------------------------------------------
/// Returns true if undo can be done.
bool pqContextView::canUndo() const
{
  return false;
}

//-----------------------------------------------------------------------------
/// Returns true if redo can be done.
bool pqContextView::canRedo() const
{
  return false;
}

//-----------------------------------------------------------------------------
/// Resets the zoom level to 100%.
void pqContextView::resetDisplay()
{
  vtkSMXYChartViewProxy *proxy =
      vtkSMXYChartViewProxy::SafeDownCast(this->getContextViewProxy());
  if (proxy)
    {
    proxy->GetChartXY()->RecalculateBounds();
    this->getWidget()->update();
    }
}

//-----------------------------------------------------------------------------
/// Returns true if data on the given output port can be displayed by this view.
bool pqContextView::canDisplay(pqOutputPort* opPort) const
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

