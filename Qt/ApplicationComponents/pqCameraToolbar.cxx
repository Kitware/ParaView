/*=========================================================================

   Program: ParaView
   Module:    pqCameraToolbar.cxx

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
#include "pqCameraToolbar.h"
#include "ui_pqCameraToolbar.h"

#include "pqCameraReaction.h"
#include "pqRubberBandHelper.h"
#include "pqActiveObjects.h"

#include "vtkSMRenderViewProxy.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkCommand.h"

//-----------------------------------------------------------------------------
void pqCameraToolbar::constructor()
{
  this->ZoomToBoxStarted = false;
  Ui::pqCameraToolbar ui;
  ui.setupUi(this);
  new pqCameraReaction(ui.actionResetCamera, pqCameraReaction::RESET_CAMERA);
  new pqCameraReaction(ui.actionPositiveX, pqCameraReaction::RESET_POSITIVE_X);
  new pqCameraReaction(ui.actionNegativeX, pqCameraReaction::RESET_NEGATIVE_X);
  new pqCameraReaction(ui.actionPositiveY, pqCameraReaction::RESET_POSITIVE_Y);
  new pqCameraReaction(ui.actionNegativeY, pqCameraReaction::RESET_NEGATIVE_Y);
  new pqCameraReaction(ui.actionPositiveZ, pqCameraReaction::RESET_POSITIVE_Z);
  new pqCameraReaction(ui.actionNegativeZ, pqCameraReaction::RESET_NEGATIVE_Z);

  /// HACK: Please FIX me at some point.
  this->SelectionHelper = new pqRubberBandHelper(this);
  // Set up connection with selection helpers for all views.
  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this->SelectionHelper, SLOT(setView(pqView*)));
  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this, SLOT(activeViewChanged(pqView*)));
  QObject::connect(this->SelectionHelper,
    SIGNAL(enableZoom(bool)),
    ui.actionZoomToBox, SLOT(setEnabled(bool)));
  QObject::connect(ui.actionZoomToBox, SIGNAL(triggered()),
    this->SelectionHelper, SLOT(beginZoom()));
  QObject::connect(this->SelectionHelper, SIGNAL(startSelection()),
    this, SLOT(startZoomToBox()));

  // When a selection is marked, we revert to interaction mode.
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(selectionFinished(int, int, int, int)),
    this->SelectionHelper, SLOT(endSelection()));
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(stopSelection()),
    this, SLOT(endZoomToBox()));
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(selectionModeChanged(int)),
    this, SLOT(onSelectionModeChanged(int)));
  this->ZoomAction = ui.actionZoomToBox;
}

//-----------------------------------------------------------------------------
void pqCameraToolbar::onSelectionModeChanged(int mode)
{
  this->ZoomAction->setChecked(mode == pqRubberBandHelper::ZOOM);
}
//-----------------------------------------------------------------------------
void pqCameraToolbar::startZoomToBox()
{
  if(this->ZoomAction->isChecked() && this->Interactor)
    {
    this->Interactor->InvokeEvent(vtkCommand::StartInteractionEvent);
    this->ZoomToBoxStarted = true;
    }
}

//-----------------------------------------------------------------------------
void pqCameraToolbar::endZoomToBox()
{
  if(this->ZoomToBoxStarted && this->Interactor)
    {
    this->Interactor->InvokeEvent(vtkCommand::EndInteractionEvent);
    }
  this->ZoomToBoxStarted = false;
}
//-----------------------------------------------------------------------------
void pqCameraToolbar::activeViewChanged(pqView* view)
{
  // Reset always
  this->Interactor = NULL;

  // If we are lucky just set it.
  if(view)
    {
    vtkSMRenderViewProxy* viewProxy =
        vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    this->Interactor = viewProxy ? viewProxy->GetInteractor() : NULL;
    }
}
