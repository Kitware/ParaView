/*=========================================================================

   Program: ParaView
   Module:    pqScatterPlotView.cxx

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

========================================================================*/
#include "pqScatterPlotView.h"

// Server Manager Includes.
#include "QVTKWidget.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVServerInformation.h"
#include "vtkSMAnimationSceneImageWriter.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMScatterPlotViewProxy.h"
#include "vtkSmartPointer.h"

// Qt Includes.
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QGridLayout>

// ParaView Includes.
#include "pqServer.h"
#include "pqSMAdaptor.h"

pqScatterPlotView::ManipulatorType pqScatterPlotView::TwoDManipulatorTypes[9] = 
{
    { 1, 0, 0, "Pan"},
    { 2, 0, 0, "Pan"},
    { 3, 0, 0, "Zoom"},
    { 1, 1, 0, "Zoom"},
    { 2, 1, 0, "Zoom"},
    { 3, 1, 0, "Zoom"},
    { 1, 0, 1, "Zoom"},
    { 2, 0, 1, "Zoom"},
    { 3, 0, 1, "Pan"},
};

pqScatterPlotView::ManipulatorType pqScatterPlotView::ThreeDManipulatorTypes[] = 
{
    { 1, 0, 0, "Rotate"},
    { 2, 0, 0, "Pan"},
    { 3, 0, 0, "Zoom"},
    { 1, 1, 0, "Roll"},
    { 2, 1, 0, "Rotate"},
    { 3, 1, 0, "Pan"},
    { 1, 0, 1, "Zoom"},
    { 2, 0, 1, "Rotate"},
    { 3, 0, 1, "Zoom"},
};

class pqScatterPlotView::pqInternal
{
public:
  QMap<vtkSMViewProxy*, QPointer<QVTKWidget> > RenderWidgets;
  // vtkSmartPointer<vtkPVAxesWidget> OrientationAxesWidget;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  bool ThreeDMode;
  bool InitializedWidgets;
  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->ThreeDMode = false;
    this->InitializedWidgets = false;
    }
};

//-----------------------------------------------------------------------------
pqScatterPlotView::pqScatterPlotView(
  const QString& group,
  const QString& name, 
  vtkSMViewProxy* viewProxy,
  pqServer* server, 
  QObject* _parent):
  Superclass(scatterPlotViewType(), group, name, viewProxy, server, _parent)
{
  this->Internal = new pqInternal();
}

//-----------------------------------------------------------------------------
pqScatterPlotView::~pqScatterPlotView()
{
  foreach (QVTKWidget* widget, this->Internal->RenderWidgets.values())
    {
    delete widget;
    }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMScatterPlotViewProxy* pqScatterPlotView::getScatterPlotViewProxy() const
{
  return vtkSMScatterPlotViewProxy::SafeDownCast(this->getViewProxy());
}

//-----------------------------------------------------------------------------
/// Resets the camera to include all visible data.
/// It is essential to call this resetCamera, to ensure that the reset camera
/// action gets pushed on the interaction undo stack.
void pqScatterPlotView::resetCamera()
{
  vtkSMScatterPlotViewProxy* view = vtkSMScatterPlotViewProxy::SafeDownCast(
    this->getProxy());

  vtkSMRenderViewProxy* renModule = view->GetRenderView();
  renModule->ResetCamera();
  this->render();
}

//-----------------------------------------------------------------------------
// This method is called for all pqRenderView objects irrespective
// of whether it is created from state/undo-redo/python or by the GUI. Hence
// don't change any render module properties here.
void pqScatterPlotView::initializeWidgets()
{
  if (this->Internal->InitializedWidgets)
    {
    return;
    }

  this->Internal->InitializedWidgets = true;

  vtkSMScatterPlotViewProxy* view = vtkSMScatterPlotViewProxy::SafeDownCast(
    this->getProxy());

  vtkSMRenderViewProxy* renModule = view->GetRenderView();
  QVTKWidget* vtkwidget = qobject_cast<QVTKWidget*>(this->getWidget());
  if (vtkwidget)
    {
    vtkwidget->SetRenderWindow(renModule->GetRenderWindow());
    }
}

//-----------------------------------------------------------------------------
vtkImageData* pqScatterPlotView::captureImage(int magnification)
{
  if (this->getWidget()->isVisible())
    {
    vtkSMScatterPlotViewProxy* view = vtkSMScatterPlotViewProxy::SafeDownCast(
      this->getProxy());

    vtkSMRenderViewProxy* renModule = view->GetRenderView();
    return renModule->CaptureWindow(magnification);
    }

  // Don't return any image when the view is not visible.
  return NULL;
}


void pqScatterPlotView::set3DMode(bool enable)
{
  if(enable == this->Internal->ThreeDMode)
    {
    return;
    }
  this->Internal->ThreeDMode = enable;
  this->initializeInteractors();
}

bool pqScatterPlotView::get3DMode()const
{
  return this->Internal->ThreeDMode;
}

const pqScatterPlotView::ManipulatorType* pqScatterPlotView
::getDefaultManipulatorTypesInternal()
{ 
  return this->Internal->ThreeDMode ? 
    pqScatterPlotView::ThreeDManipulatorTypes:
    pqScatterPlotView::TwoDManipulatorTypes; 
}
