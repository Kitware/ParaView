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
#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "pqRenderView.h"
#include "pqSelectionManager.h"
#include "pqUndoStack.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPVRenderView.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMInteractiveSelectionPipeline.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"

#include <cassert>

#include "zoom.xpm"

QPointer<pqRenderViewSelectionReaction> pqRenderViewSelectionReaction::ActiveReaction;

//-----------------------------------------------------------------------------
pqRenderViewSelectionReaction::pqRenderViewSelectionReaction(
  QAction* parentObject, pqRenderView* view, SelectionMode mode)
  : Superclass(parentObject),
  View(view),
  Mode(mode),
  PreviousRenderViewMode(-1),
  ZoomCursor(QCursor(QPixmap((const char **)zoom_xpm)))
{
for (size_t i = 0;
     i < sizeof(this->ObserverIds) / sizeof(this->ObserverIds[0]); ++i)
    {
    this->ObserverIds[i] = 0;
    }

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

  if (this->Mode == CLEAR_SELECTION)
    {
    if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
      {
      this->connect(core->selectionManager(), SIGNAL(selectionChanged(pqOutputPort*)),
        SLOT(updateEnableState()));
      }
    }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqRenderViewSelectionReaction::~pqRenderViewSelectionReaction()
{
  this->cleanupObservers();
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::cleanupObservers()
{
  for (size_t i = 0;
       i < sizeof(this->ObserverIds) / sizeof(this->ObserverIds[0]); ++i)
    {
    if (this->ObservedObject != NULL && this->ObserverIds[i] > 0)
    {
      this->ObservedObject->RemoveObserver(this->ObserverIds[i]);
    }
    this->ObserverIds[i] = 0;
    }
  this->ObservedObject = NULL;
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::actionTriggered(bool val)
{
  QAction* actn = this->parentAction();
  if (actn->isCheckable())
    {
    if (val) { this->beginSelection(); }
    else { this->endSelection(); }
    }
  else
    {
    this->beginSelection();
    this->endSelection();
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::updateEnableState()
{
  bool enabled = true;
  if (this->Mode == CLEAR_SELECTION)
    {
    if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
      {
      enabled = core->selectionManager()->getSelectedPort() != NULL;
      }
    }
  this->parentAction()->setEnabled(enabled);
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

  case SELECT_SURFACE_CELLS_INTERACTIVELY:
  case SELECT_SURFACE_POINTS_INTERACTIVELY:
    pqCoreUtilities::promptUser(
      "pqInteractiveSelection",
      QMessageBox::Information,
      "Interactive Selection Information",
      "You are entering interactive selection mode to highlight cells (or points). "
      "Simply move the mouse point over "
      "the dataset to interactively highlight elements. "
      "Use the 'Selection Display Inspector' to choose the array to label with.\n\n"
      "To add the currently "
      "highlighted element to the active selection, simply click on that element.\n\n"
      "Use the 'Esc' key or the same toolbar button to exit this mode.",
      QMessageBox::Ok | QMessageBox::Save);
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

  case CLEAR_SELECTION:
    if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
      {
      core->selectionManager()->clearSelection();
      }
    break;

  default:
    this->View->setCursor(QCursor());
    break;
    }
  rmp->UpdateVTKObjects();

  // Setup observer.
  assert(this->ObserverIds[0] == 0 && this->ObservedObject == NULL &&
    this->ObserverIds[1] == 0);
  switch (this->Mode)
    {
  case ZOOM_TO_BOX:
    this->ObservedObject = rmp->GetInteractor();
    this->ObserverIds[0] = this->ObservedObject->AddObserver(
      vtkCommand::LeftButtonReleaseEvent,
      this, &pqRenderViewSelectionReaction::selectionChanged);
    break;

  case CLEAR_SELECTION:
    break;

  case SELECT_SURFACE_CELLS_INTERACTIVELY:
  case SELECT_SURFACE_POINTS_INTERACTIVELY:
    this->ObservedObject = rmp->GetInteractor();
    this->ObserverIds[0] = this->ObservedObject->AddObserver(
      vtkCommand::MouseMoveEvent,
      this, &pqRenderViewSelectionReaction::onMouseMove);
    this->ObserverIds[1] = this->ObservedObject->AddObserver(
      vtkCommand::LeftButtonReleaseEvent,
      this, &pqRenderViewSelectionReaction::onLeftButtonRelease);

    this->ObserverIds[2] = this->ObservedObject->AddObserver(
      vtkCommand::MouseWheelForwardEvent,
      this, &pqRenderViewSelectionReaction::onWheelRotate);
    this->ObserverIds[3] = this->ObservedObject->AddObserver(
      vtkCommand::MouseWheelBackwardEvent,
      this, &pqRenderViewSelectionReaction::onWheelRotate);
    break;

  default:
    this->ObservedObject = rmp;
    this->ObserverIds[0] = this->ObservedObject->AddObserver(
      vtkCommand::SelectionChangedEvent,
      this, &pqRenderViewSelectionReaction::selectionChanged);
    break;
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
  this->cleanupObservers();
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

  pqRenderView::pqSelectionOperator selOp = pqRenderView::PV_SELECTION_NEW;
  if (rmp->GetInteractor()->GetControlKey() == 1)
    {
    selOp = pqRenderView::PV_SELECTION_MERGE;
    }
  else if (rmp->GetInteractor()->GetShiftKey() == 1)
    {
    selOp = pqRenderView::PV_SELECTION_SUBTRACT;
    }

  int* region = reinterpret_cast<int*>(calldata);
  vtkObject* unsafe_object = reinterpret_cast<vtkObject*>(calldata);

  switch (this->Mode)
    {
  case SELECT_SURFACE_CELLS:
    this->View->selectOnSurface(region, selOp);
    break;

  case SELECT_SURFACE_POINTS:
    this->View->selectPointsOnSurface(region, selOp);
    break;

  case SELECT_FRUSTUM_CELLS:
    this->View->selectFrustum(region);
    break;

  case SELECT_FRUSTUM_POINTS:
    this->View->selectFrustumPoints(region);
    break;

  case SELECT_SURFACE_CELLS_POLYGON:
    this->View->selectPolygonCells(vtkIntArray::SafeDownCast(unsafe_object),
      selOp);
    break;

  case SELECT_SURFACE_POINTS_POLYGON:
    this->View->selectPolygonPoints(vtkIntArray::SafeDownCast(unsafe_object),
      selOp);
    break;

  case SELECT_BLOCKS:
    this->View->selectBlock(region, selOp);
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

  default:
    break;
    }

  END_UNDO_EXCLUDE();

  this->endSelection();

  if (this->View)
    {
    bool frustumSelection =
      this->Mode == pqRenderViewSelectionReaction::SELECT_FRUSTUM_CELLS ||
      this->Mode == pqRenderViewSelectionReaction::SELECT_FRUSTUM_POINTS;
    this->View->emitSelectionSignals(frustumSelection);
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::onMouseMove()
{
  if (pqRenderViewSelectionReaction::ActiveReaction != this)
    {
    qWarning("Unexpected call to onMouseMove.");
    return;
    }

  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  Q_ASSERT(rmp != NULL);

  int x = rmp->GetInteractor()->GetEventPosition()[0];
  int y = rmp->GetInteractor()->GetEventPosition()[1];
  int* size = rmp->GetInteractor()->GetSize();
  vtkSMInteractiveSelectionPipeline* iSelectionPipeline =
    vtkSMInteractiveSelectionPipeline::GetInstance();
  if (x < 0 || y < 0 || x >= size[0] || y >= size[1])
    {
    // If the cursor goes out of the render window we hide the
    // interactive selection
    iSelectionPipeline->Hide(rmp);
    return;
    }

  int region[4] = {x, y, x, y};

  vtkNew<vtkCollection> selectedRepresentations;
  vtkNew<vtkCollection> selectionSources;
  bool status = false;
  switch (this->Mode)
    {
  case SELECT_SURFACE_CELLS_INTERACTIVELY:
    status = rmp->SelectSurfaceCells(
      region, selectedRepresentations.GetPointer(), selectionSources.GetPointer());
    break;

  case SELECT_SURFACE_POINTS_INTERACTIVELY:
    status = rmp->SelectSurfacePoints(
      region, selectedRepresentations.GetPointer(), selectionSources.GetPointer());
    break;

  default:
    qCritical("Invalid call to pqRenderViewSelectionReaction::onMouseMove");
    return;
    }

  if (status)
    {
    BEGIN_UNDO_EXCLUDE();
    iSelectionPipeline->Show(
      vtkSMSourceProxy::SafeDownCast(selectedRepresentations->GetItemAsObject(0)),
      vtkSMSourceProxy::SafeDownCast(selectionSources->GetItemAsObject(0)),
      rmp);
    END_UNDO_EXCLUDE();
    }
  else
    {
    iSelectionPipeline->Hide(rmp);
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::onLeftButtonRelease()
{
  if (pqRenderViewSelectionReaction::ActiveReaction != this)
    {
    qWarning("Unexpected call to selectionChanged.");
    return;
    }

  vtkSMRenderViewProxy* viewProxy = this->View->getRenderViewProxy();
  Q_ASSERT(viewProxy != NULL);

  int x = viewProxy->GetInteractor()->GetEventPosition()[0];
  int y = viewProxy->GetInteractor()->GetEventPosition()[1];
  if (x < 0 || y < 0)
    {
    // sometimes when the cursor goes quickly out of the window we receive -1
    // the rest of the code hangs in that case.
    return;
    }

  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  pqRenderView::pqSelectionOperator selOp = pqRenderView::PV_SELECTION_MERGE;
  if (rmp->GetInteractor()->GetShiftKey() == 1)
    {
    selOp = pqRenderView::PV_SELECTION_SUBTRACT;
    }

  int region[4] = {x, y, x, y};

  switch (this->Mode)
    {
  case SELECT_SURFACE_CELLS_INTERACTIVELY:
    this->View->selectOnSurface(region, selOp);
    break;

  case SELECT_SURFACE_POINTS_INTERACTIVELY:
    this->View->selectPointsOnSurface(region, selOp);
    break;

  default:
    qCritical("Invalid call to pqRenderViewSelectionReaction::onLeftButtonRelease");
    break;
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::onWheelRotate()
{
  if (pqRenderViewSelectionReaction::ActiveReaction == this)
    {
    this->onMouseMove();
    }
}
