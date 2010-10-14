/*=========================================================================

   Program: ParaView
   Module:    pqTwoDRenderView.cxx

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
#include "pqTwoDRenderView.h"

// Server Manager Includes.
#include "QVTKWidget.h"
#include "vtkPVDataInformation.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTwoDRenderViewProxy.h"

// Qt Includes.

// ParaView Includes.
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqDataRepresentation.h"

pqTwoDRenderView::ManipulatorType pqTwoDRenderView::DefaultManipulatorTypes[9] = 
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

//-----------------------------------------------------------------------------
pqTwoDRenderView::pqTwoDRenderView(
  const QString& group,
  const QString& name, 
  vtkSMViewProxy* viewProxy,
  pqServer* server, 
  QObject* _parent):
  Superclass(twoDRenderViewType(), group, name, viewProxy, server, _parent)
{
  this->InitializedWidgets = false;
  QObject::connect(this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(updateVisibility(pqRepresentation*, bool)));
}

//-----------------------------------------------------------------------------
pqTwoDRenderView::~pqTwoDRenderView()
{
}

//-----------------------------------------------------------------------------
/// Resets the camera to include all visible data.
/// It is essential to call this resetCamera, to ensure that the reset camera
/// action gets pushed on the interaction undo stack.
void pqTwoDRenderView::resetCamera()
{
  this->getProxy()->InvokeCommand("ResetCamera");
  this->render();
}

//-----------------------------------------------------------------------------
// This method is called for all pqTwoDRenderView objects irrespective
// of whether it is created from state/undo-redo/python or by the GUI. Hence
// don't change any render module properties here.
void pqTwoDRenderView::initializeWidgets()
{
  if (this->InitializedWidgets)
    {
    return;
    }

  this->InitializedWidgets = true;

  vtkSMTwoDRenderViewProxy* view = vtkSMTwoDRenderViewProxy::SafeDownCast(
    this->getProxy());

  QVTKWidget* vtkwidget = qobject_cast<QVTKWidget*>(this->getWidget());
  if (vtkwidget)
    {
    vtkwidget->SetRenderWindow(view->GetRenderWindow());
    }
}

//-----------------------------------------------------------------------------
vtkImageData* pqTwoDRenderView::captureImage(int magnification)
{
  if (this->getWidget()->isVisible())
    {
    vtkSMTwoDRenderViewProxy* view = vtkSMTwoDRenderViewProxy::SafeDownCast(
      this->getProxy());

    return view->CaptureWindow(magnification);
    }

  // Don't return any image when the view is not visible.
  return NULL;
}

//-----------------------------------------------------------------------------
bool pqTwoDRenderView::canDisplay(pqOutputPort* opPort) const
{
  if (opPort == NULL || !this->Superclass::canDisplay(opPort))
    {
    return false;
    }

  pqPipelineSource* source = opPort->getSource();
  vtkSMSourceProxy* sourceProxy = 
    vtkSMSourceProxy::SafeDownCast(source->getProxy());
  if (!sourceProxy ||
     sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  const char* dataclassname = opPort->getDataClassName();
  return (strcmp(dataclassname, "vtkImageData") == 0 ||
    strcmp(dataclassname, "vtkUniformGrid") == 0);
}

//-----------------------------------------------------------------------------
void pqTwoDRenderView::updateVisibility(pqRepresentation* curRepr, bool visible)
{
  if (!qobject_cast<pqDataRepresentation*>(curRepr))
    {
    return;
    }

  if (visible)
    {
    QList<pqRepresentation*> reprs = this->getRepresentations();
    foreach (pqRepresentation* repr, reprs)
      {
      if (!qobject_cast<pqDataRepresentation*>(repr))
        {
        continue;
        }
      if (repr != curRepr && repr->isVisible())
        {
        repr->setVisible(false);
        }
      }
    }
}
