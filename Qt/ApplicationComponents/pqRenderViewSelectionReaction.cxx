/*=========================================================================

   Program: ParaView
   Module:  pqRenderViewSelectionReaction.cxx

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
#include "pqRenderViewSelectionReaction.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"
#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkIntArray.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

#include "zoom.xpm"

QPointer<pqRenderViewSelectionReaction> pqRenderViewSelectionReaction::ActiveReaction;

//-----------------------------------------------------------------------------
pqRenderViewSelectionReaction::pqRenderViewSelectionReaction(
  QAction* parentObject, pqRenderView* view, SelectionMode mode)
  : Superclass(parentObject),
  View(view),
  Mode(mode),
  PreviousRenderViewMode(-1),
  ObserverId(0),
  ZoomCursor(QCursor(QPixmap((const char **)zoom_xpm)))
{
  QObject::connect(parentObject, SIGNAL(triggered(bool)),
    this, SLOT(actionTriggered(bool)));

  // if view == NULL, we track the active view.
  if (view == NULL)
    {
    QObject::connect(
      &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
      this, SLOT(setView(pqView*)));
    // this ensure that the enabled-state is set correctly.
    this->setView(NULL);
    }
}

//-----------------------------------------------------------------------------
pqRenderViewSelectionReaction::~pqRenderViewSelectionReaction()
{
  if (this->View && this->ObserverId > 0)
    {
    this->View->getProxy()->RemoveObserver(this->ObserverId);
    }
  this->ObserverId = 0;
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::setView(pqView* view)
{
  if (this->View != view)
    {
    // if we were currently in selection, finish that before changing the view.
    this->endSelection();
    }

  this->View = qobject_cast<pqRenderView*>(view);

  // update enable state.
  this->parentAction()->setEnabled(this->View != NULL);
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::beginSelection()
{
  if (!this->View)
    {
    return;
    }

  if (pqRenderViewSelectionReaction::ActiveReaction == this)
    {
    // We are already doing a selection. Simply return.
    return;
    }

  if (pqRenderViewSelectionReaction::ActiveReaction)
    {
    // Some other selection was active, end it before we start a new one.
    pqRenderViewSelectionReaction::ActiveReaction->endSelection();
    }

  pqRenderViewSelectionReaction::ActiveReaction = this;

  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  vtkSMPropertyHelper(rmp, "InteractionMode").Get(&this->PreviousRenderViewMode);

  // Change the interactor mode. This is what render the rubber band.
  // Change the cursor rendered on the screen.
  switch (this->Mode)
    {
  case SELECT_SURFACE_CELLS:
  case SELECT_SURFACE_POINTS:
  case SELECT_FRUSTUM_CELLS:
  case SELECT_FRUSTUM_POINTS:
  case SELECT_BLOCKS:
  case SELECT_CUSTOM_BOX:
    this->View->setCursor(Qt::CrossCursor);
    vtkSMPropertyHelper(rmp, "InteractionMode").Set(
      vtkPVRenderView::INTERACTION_MODE_SELECTION);
    break;

  case ZOOM_TO_BOX:
    this->View->setCursor(this->ZoomCursor);
    vtkSMPropertyHelper(rmp, "InteractionMode").Set(
      vtkPVRenderView::INTERACTION_MODE_ZOOM);
    break;

  case SELECT_SURFACE_CELLS_POLYGON:
  case SELECT_SURFACE_POINTS_POLYGON:
  case SELECT_CUSTOM_POLYGON:
    this->View->setCursor(Qt::PointingHandCursor);
    vtkSMPropertyHelper(rmp, "InteractionMode").Set(
      vtkPVRenderView::INTERACTION_MODE_POLYGON);
    break;

  default:
    this->View->setCursor(QCursor());
    break;
    }
  rmp->UpdateVTKObjects();

  // Setup observer.
  if (this->Mode == ZOOM_TO_BOX)
    {
    this->ObserverId = rmp->GetInteractor()->AddObserver(
      vtkCommand::LeftButtonReleaseEvent,
      this, &pqRenderViewSelectionReaction::selectionChanged);
    }
  else
    {
    this->ObserverId = rmp->AddObserver(
      vtkCommand::SelectionChangedEvent,
      this, &pqRenderViewSelectionReaction::selectionChanged);
    }

  this->parentAction()->setChecked(true);
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::endSelection()
{
  if (!this->View)
    {
    return;
    }

  if (pqRenderViewSelectionReaction::ActiveReaction != this ||
    this->PreviousRenderViewMode == -1)
    {
    return;
    }

  pqRenderViewSelectionReaction::ActiveReaction = NULL;
  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  vtkSMPropertyHelper(rmp, "InteractionMode").Set(this->PreviousRenderViewMode);
  this->PreviousRenderViewMode = -1;
  rmp->UpdateVTKObjects();
  this->View->setCursor(QCursor());
  if (this->Mode == ZOOM_TO_BOX)
    {
    rmp->GetInteractor()->RemoveObserver(this->ObserverId);
    }
  else
    {
    rmp->RemoveObserver(this->ObserverId);
    }
  this->ObserverId = 0;
  this->parentAction()->setChecked(false);

}
  
//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::selectionChanged(
  vtkObject*, unsigned long, void* calldata)
{
  if (pqRenderViewSelectionReaction::ActiveReaction != this)
    {
    qWarning("Unexpected call to selectionChanged.");
    return;
    }

  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  Q_ASSERT(rmp != NULL);

  BEGIN_UNDO_EXCLUDE();

  bool ctrl = (rmp->GetInteractor()->GetControlKey() == 1);
  int* region = reinterpret_cast<int*>(calldata);
  vtkObject* unsafe_object = reinterpret_cast<vtkObject*>(calldata);

  switch (this->Mode)
    {
  case SELECT_SURFACE_CELLS:
    this->View->selectOnSurface(region, ctrl);
    break;

  case SELECT_SURFACE_POINTS:
    this->View->selectPointsOnSurface(region, ctrl);
    break;

  case SELECT_FRUSTUM_CELLS:
    this->View->selectFrustum(region);
    break;

  case SELECT_FRUSTUM_POINTS:
    this->View->selectFrustumPoints(region);
    break;

  case SELECT_SURFACE_CELLS_POLYGON:
    this->View->selectPolygonCells(vtkIntArray::SafeDownCast(unsafe_object),
      ctrl);
    break;

  case SELECT_SURFACE_POINTS_POLYGON:
    this->View->selectPolygonPoints(vtkIntArray::SafeDownCast(unsafe_object),
      ctrl);
    break;

  case SELECT_BLOCKS:
    this->View->selectBlock(region, ctrl);
    break;

  case SELECT_CUSTOM_BOX:
    emit this->selectedCustomBox(region);
    emit this->selectedCustomBox(region[0], region[1], region[2], region[3]);
    break;

  case SELECT_CUSTOM_POLYGON:
    emit this->selectedCustomPolygon(vtkIntArray::SafeDownCast(unsafe_object));
    break;

  case ZOOM_TO_BOX:
    this->View->resetCenterOfRotationIfNeeded();
    break;
    }

  END_UNDO_EXCLUDE();

  this->endSelection();
}
