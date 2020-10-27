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
#include "pqOutputPort.h"
#include "pqPVApplicationCore.h"
#include "pqRenderView.h"
#include "pqSelectionManager.h"
#include "pqUndoStack.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMInteractiveSelectionPipeline.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTooltipSelectionPipeline.h"

#include <QSet>
#include <QToolTip>

#include <cassert>

#include "zoom.xpm"

static const int TOOLTIP_WAITING_TIME = 400;

QPointer<pqRenderViewSelectionReaction> pqRenderViewSelectionReaction::ActiveReaction;

//-----------------------------------------------------------------------------
pqRenderViewSelectionReaction::pqRenderViewSelectionReaction(
  QAction* parentObject, pqRenderView* view, SelectionMode mode, QActionGroup* modifierGroup)
  : Superclass(parentObject, modifierGroup)
  , View(view)
  , Mode(mode)
  , PreviousRenderViewMode(-1)
  , ZoomCursor(QCursor(QPixmap((const char**)zoom_xpm)))
  , MouseMovingTimer(this)
  , MouseMoving(false)
{
  this->MousePosition[0] = 0;
  this->MousePosition[1] = 0;
  for (size_t i = 0; i < sizeof(this->ObserverIds) / sizeof(this->ObserverIds[0]); ++i)
  {
    this->ObserverIds[i] = 0;
  }

  QObject::connect(parentObject, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));

  // if view == nullptr, we track the active view.
  if (view == nullptr)
  {
    QObject::connect(
      &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setView(pqView*)));
    // this ensure that the enabled-state is set correctly.
    this->setView(nullptr);
  }

  if (this->Mode == CLEAR_SELECTION || this->Mode == GROW_SELECTION ||
    this->Mode == SHRINK_SELECTION)
  {
    if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
    {
      this->connect(core->selectionManager(), SIGNAL(selectionChanged(pqOutputPort*)),
        SLOT(updateEnableState()));
    }
  }

  if (this->Mode == SELECT_FRUSTUM_CELLS || this->Mode == SELECT_FRUSTUM_POINTS)
  {
    this->DisableSelectionModifiers = true;
  }
  else
  {
    this->DisableSelectionModifiers = false;
  }

  this->setRepresentation(nullptr);
  if (this->Mode == SELECT_SURFACE_POINTDATA_INTERACTIVELY ||
    this->Mode == SELECT_SURFACE_CELLDATA_INTERACTIVELY)
  {
    QObject::connect(&pqActiveObjects::instance(),
      SIGNAL(representationChanged(pqDataRepresentation*)), this,
      SLOT(setRepresentation(pqDataRepresentation*)));
  }

  this->updateEnableState();

  this->MouseMovingTimer.setSingleShot(true);
  this->connect(&this->MouseMovingTimer, SIGNAL(timeout()), this, SLOT(onMouseStop()));
}

//-----------------------------------------------------------------------------
pqRenderViewSelectionReaction::~pqRenderViewSelectionReaction()
{
  this->cleanupObservers();
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::cleanupObservers()
{
  for (size_t i = 0; i < sizeof(this->ObserverIds) / sizeof(this->ObserverIds[0]); ++i)
  {
    if (this->ObservedObject != nullptr && this->ObserverIds[i] > 0)
    {
      this->ObservedObject->RemoveObserver(this->ObserverIds[i]);
    }
    this->ObserverIds[i] = 0;
  }
  this->ObservedObject = nullptr;
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::actionTriggered(bool val)
{
  QAction* actn = this->parentAction();
  if (actn->isCheckable())
  {
    if (val)
    {
      this->beginSelection();
    }
    else
    {
      this->endSelection();
    }
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
  this->endSelection();

  auto paction = this->parentAction();
  switch (this->Mode)
  {
    case CLEAR_SELECTION:
    case GROW_SELECTION:
      if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
      {
        paction->setEnabled(core->selectionManager()->hasActiveSelection());
      }
      else
      {
        paction->setEnabled(false);
      }
      break;
    case SHRINK_SELECTION:
      if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
      {
        bool can_shrink = false;
        for (auto port : core->selectionManager()->getSelectedPorts())
        {
          if (auto selsource = port->getSelectionInput())
          {
            vtkSMPropertyHelper helper(selsource, "NumberOfLayers");
            can_shrink |= (helper.GetAsInt() >= 1);
          }
        }
        paction->setEnabled(can_shrink);
      }
      else
      {
        paction->setEnabled(false);
      }
      break;
    case SELECT_SURFACE_POINTDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLDATA_INTERACTIVELY:
    {
      bool state = false;
      if (this->Representation)
      {
        vtkSMProxy* proxy = this->Representation->getProxy();
        vtkSMStringVectorProperty* prop =
          vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("ColorArrayName"));
        if (prop)
        {
          int association = std::atoi(prop->GetElement(3));
          const char* arrayName = prop->GetElement(4);

          vtkPVDataInformation* dataInfo = this->Representation->getInputDataInformation();

          vtkPVDataSetAttributesInformation* info = nullptr;
          if (association == vtkDataObject::CELL &&
            this->Mode == SELECT_SURFACE_CELLDATA_INTERACTIVELY)
          {
            info = dataInfo->GetCellDataInformation();
          }
          if (association == vtkDataObject::POINT &&
            this->Mode == SELECT_SURFACE_POINTDATA_INTERACTIVELY)
          {
            info = dataInfo->GetPointDataInformation();
          }

          if (info)
          {
            vtkPVArrayInformation* arrayInfo = info->GetArrayInformation(arrayName);
            state = arrayInfo && arrayInfo->GetDataType() == VTK_ID_TYPE;
          }
        }
      }
      paction->setEnabled(state);
    }
    break;
    default:
      paction->setEnabled(true);
      break;
  }
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
  this->parentAction()->setEnabled(this->View != nullptr);
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::setRepresentation(pqDataRepresentation* representation)
{
  if (this->Representation != representation)
  {
    // if we are currently in selection, finish that before changing the representation.
    this->endSelection();

    if (this->Representation != nullptr)
    {
      QObject::disconnect(this->RepresentationConnection);
    }

    this->Representation = representation;

    if (this->Representation != nullptr)
    {
      this->RepresentationConnection = this->connect(
        this->Representation, SIGNAL(colorArrayNameModified()), SLOT(updateEnableState()));
    }

    // update enable state.
    this->updateEnableState();
  }
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

  // Check if this selection support selection modifier
  if (pqRenderViewSelectionReaction::ActiveReaction)
  {
    // Check if selection are compatible, if not, deactivate selection modifiers
    if (!pqRenderViewSelectionReaction::ActiveReaction->isCompatible(this->Mode))
    {
      this->uncheckSelectionModifiers();
    }

    // Some other selection was active, end it before we start a new one.
    pqRenderViewSelectionReaction::ActiveReaction->endSelection();
  }

  // Enable/Disable selection if supported
  this->disableSelectionModifiers(this->DisableSelectionModifiers);

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
      vtkSMPropertyHelper(rmp, "InteractionMode").Set(vtkPVRenderView::INTERACTION_MODE_SELECTION);
      break;

    case SELECT_SURFACE_POINTDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLS_INTERACTIVELY:
    case SELECT_SURFACE_POINTS_INTERACTIVELY:
      pqCoreUtilities::promptUser("pqInteractiveSelection", QMessageBox::Information,
        "Interactive Selection Information",
        "You are entering interactive selection mode to highlight cells (or points). "
        "Simply move the mouse point over "
        "the dataset to interactively highlight elements. "
        "Use the 'Selection Display Inspector' to choose the array to label with.\n\n"
        "To add the currently "
        "highlighted element to the active selection, simply click on that element.\n\n"
        "You can click on selection modifier button or use modifier keys to subtract or "
        " even toggle the selection. Click outside of mesh to clear selection.\n\n"
        "Use the 'Esc' key or the same toolbar button to exit this mode.",
        QMessageBox::Ok | QMessageBox::Save);
      this->View->setCursor(Qt::CrossCursor);
      vtkSMPropertyHelper(rmp, "InteractionMode").Set(vtkPVRenderView::INTERACTION_MODE_SELECTION);
      break;

    case SELECT_SURFACE_POINTS_TOOLTIP:
      pqCoreUtilities::promptUser("pqTooltipSelection", QMessageBox::Information,
        "Tooltip Selection Information",
        "You are entering tooltip selection mode to display points information. "
        "Simply move the mouse point over the dataset to interactively highlight "
        "points and display a tooltip with points information.\n\n"
        "Use the 'Esc' key or the same toolbar button to exit this mode.",
        QMessageBox::Ok | QMessageBox::Save);
      // switch the interaction mode to selection mode even though we're no making
      // selections. This ensures that the render view realizes it's being used
      // for selection and will not release cached selection render buffers as the
      // selection interaction progresses (BUG #0015882).
      vtkSMPropertyHelper(rmp, "InteractionMode").Set(vtkPVRenderView::INTERACTION_MODE_SELECTION);
      break;

    case SELECT_SURFACE_CELLS_TOOLTIP:
      pqCoreUtilities::promptUser("pqTooltipSelection", QMessageBox::Information,
        "Tooltip Selection Information",
        "You are entering tooltip selection mode to display cell information. "
        "Simply move the mouse point over the dataset to interactively highlight "
        "points and display a tooltip with cell information.\n\n"
        "Use the 'Esc' key or the same toolbar button to exit this mode.",
        QMessageBox::Ok | QMessageBox::Save);
      // switch the interaction mode to selection mode even though we're no making
      // selections. This ensures that the render view realizes it's being used
      // for selection and will not release cached selection render buffers as the
      // selection interaction progresses (BUG #0015882).
      vtkSMPropertyHelper(rmp, "InteractionMode").Set(vtkPVRenderView::INTERACTION_MODE_SELECTION);
      break;

    case ZOOM_TO_BOX:
      this->View->setCursor(this->ZoomCursor);
      vtkSMPropertyHelper(rmp, "InteractionMode").Set(vtkPVRenderView::INTERACTION_MODE_ZOOM);
      break;

    case SELECT_SURFACE_CELLS_POLYGON:
    case SELECT_SURFACE_POINTS_POLYGON:
    case SELECT_CUSTOM_POLYGON:
      this->View->setCursor(Qt::PointingHandCursor);
      vtkSMPropertyHelper(rmp, "InteractionMode").Set(vtkPVRenderView::INTERACTION_MODE_POLYGON);
      break;

    case CLEAR_SELECTION:
      if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
      {
        core->selectionManager()->clearSelection();
      }
      break;

    case GROW_SELECTION:
    case SHRINK_SELECTION:
      if (pqPVApplicationCore* core = pqPVApplicationCore::instance())
      {
        core->selectionManager()->expandSelection(this->Mode == GROW_SELECTION ? 1 : -1);
      }
      break;

    default:
      this->View->setCursor(QCursor());
      break;
  }
  rmp->UpdateVTKObjects();

  // Setup observer.
  assert(this->ObserverIds[0] == 0 && this->ObservedObject == nullptr && this->ObserverIds[1] == 0);
  switch (this->Mode)
  {
    case ZOOM_TO_BOX:
      this->ObservedObject = rmp->GetInteractor();
      this->ObserverIds[0] = this->ObservedObject->AddObserver(
        vtkCommand::LeftButtonReleaseEvent, this, &pqRenderViewSelectionReaction::selectionChanged);
      break;

    case CLEAR_SELECTION:
    case GROW_SELECTION:
    case SHRINK_SELECTION:
      break;

    case SELECT_SURFACE_POINTDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLS_INTERACTIVELY:
    case SELECT_SURFACE_POINTS_INTERACTIVELY:
      this->ObservedObject = rmp->GetInteractor();
      this->ObserverIds[0] = this->ObservedObject->AddObserver(
        vtkCommand::MouseMoveEvent, this, &pqRenderViewSelectionReaction::onMouseMove);
      this->ObserverIds[1] = this->ObservedObject->AddObserver(vtkCommand::LeftButtonReleaseEvent,
        this, &pqRenderViewSelectionReaction::onLeftButtonRelease);

      this->ObserverIds[2] = this->ObservedObject->AddObserver(
        vtkCommand::MouseWheelForwardEvent, this, &pqRenderViewSelectionReaction::onWheelRotate);
      this->ObserverIds[3] = this->ObservedObject->AddObserver(
        vtkCommand::MouseWheelBackwardEvent, this, &pqRenderViewSelectionReaction::onWheelRotate);
      break;

    case SELECT_SURFACE_POINTS_TOOLTIP:
    case SELECT_SURFACE_CELLS_TOOLTIP:
      this->ObservedObject = rmp->GetInteractor();
      this->ObserverIds[0] = this->ObservedObject->AddObserver(
        vtkCommand::MouseMoveEvent, this, &pqRenderViewSelectionReaction::onMouseMove);
      break;

    default:
      this->ObservedObject = rmp;
      this->ObserverIds[0] = this->ObservedObject->AddObserver(
        vtkCommand::SelectionChangedEvent, this, &pqRenderViewSelectionReaction::selectionChanged);
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

  if (pqRenderViewSelectionReaction::ActiveReaction != this || this->PreviousRenderViewMode == -1)
  {
    return;
  }

  pqRenderViewSelectionReaction::ActiveReaction = nullptr;
  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  vtkSMPropertyHelper(rmp, "InteractionMode").Set(this->PreviousRenderViewMode);
  this->PreviousRenderViewMode = -1;
  rmp->UpdateVTKObjects();
  this->View->setCursor(QCursor());
  this->cleanupObservers();
  this->parentAction()->setChecked(false);
  this->MouseMovingTimer.stop();
  this->MouseMoving = false;
  this->UpdateTooltip();

  if (this->CurrentRepresentation != nullptr)
  {
    vtkSMSessionProxyManager* pxm = rmp->GetSessionProxyManager();
    vtkSMProxy* emptySel = pxm->NewProxy("sources", "IDSelectionSource");

    vtkSMPropertyHelper(this->CurrentRepresentation, "Selection").Set(emptySel);
    this->CurrentRepresentation->UpdateVTKObjects();
    this->CurrentRepresentation = nullptr;
    emptySel->Delete();

    rmp->StillRender();
  }
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::selectionChanged(vtkObject*, unsigned long, void* calldata)
{
  if (pqRenderViewSelectionReaction::ActiveReaction != this)
  {
    qWarning("Unexpected call to selectionChanged.");
    return;
  }

  BEGIN_UNDO_EXCLUDE();

  int selectionModifier = this->getSelectionModifier();
  int* region = reinterpret_cast<int*>(calldata);
  vtkObject* unsafe_object = reinterpret_cast<vtkObject*>(calldata);

  switch (this->Mode)
  {
    case SELECT_SURFACE_CELLS:
      this->View->selectOnSurface(region, selectionModifier);
      break;

    case SELECT_SURFACE_POINTS:
      this->View->selectPointsOnSurface(region, selectionModifier);
      break;

    case SELECT_FRUSTUM_CELLS:
      this->View->selectFrustum(region);
      break;

    case SELECT_FRUSTUM_POINTS:
      this->View->selectFrustumPoints(region);
      break;

    case SELECT_SURFACE_CELLS_POLYGON:
      this->View->selectPolygonCells(vtkIntArray::SafeDownCast(unsafe_object), selectionModifier);
      break;

    case SELECT_SURFACE_POINTS_POLYGON:
      this->View->selectPolygonPoints(vtkIntArray::SafeDownCast(unsafe_object), selectionModifier);
      break;

    case SELECT_BLOCKS:
      this->View->selectBlock(region, selectionModifier);
      break;

    case SELECT_CUSTOM_BOX:
      Q_EMIT this->selectedCustomBox(region);
      Q_EMIT this->selectedCustomBox(region[0], region[1], region[2], region[3]);
      break;

    case SELECT_CUSTOM_POLYGON:
      Q_EMIT this->selectedCustomPolygon(vtkIntArray::SafeDownCast(unsafe_object));
      break;

    case ZOOM_TO_BOX:
      break;

    default:
      break;
  }

  END_UNDO_EXCLUDE();

  this->endSelection();

  if (this->View)
  {
    bool frustumSelection = this->Mode == pqRenderViewSelectionReaction::SELECT_FRUSTUM_CELLS ||
      this->Mode == pqRenderViewSelectionReaction::SELECT_FRUSTUM_POINTS;
    this->View->emitSelectionSignals(frustumSelection);
  }
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::onMouseMove()
{
  switch (this->Mode)
  {
    case SELECT_SURFACE_POINTS_TOOLTIP:
    case SELECT_SURFACE_CELLS_TOOLTIP:
      this->MouseMovingTimer.start(TOOLTIP_WAITING_TIME);
      this->MouseMoving = true;
      // fast preselection is not working with the tooltip yet
      this->preSelection();
      break;

    case SELECT_SURFACE_POINTDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLS_INTERACTIVELY:
    case SELECT_SURFACE_POINTS_INTERACTIVELY:
      if (vtkPVRenderViewSettings::GetInstance()->GetEnableFastPreselection())
      {
        this->fastPreSelection();
      }
      else
      {
        this->preSelection();
      }
      break;

    default:
      qCritical("Invalid call to pqRenderViewSelectionReaction::onMouseMove");
      return;
  }
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::preSelection()
{
  if (pqRenderViewSelectionReaction::ActiveReaction != this)
  {
    qWarning("Unexpected call to preSelection.");
    return;
  }

  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  assert(rmp != nullptr);

  int x = rmp->GetInteractor()->GetEventPosition()[0];
  int y = rmp->GetInteractor()->GetEventPosition()[1];
  int* size = rmp->GetInteractor()->GetSize();
  this->MousePosition[0] = x;
  this->MousePosition[1] = y;

  vtkSMPreselectionPipeline* pipeline;
  switch (this->Mode)
  {
    case SELECT_SURFACE_POINTDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLS_INTERACTIVELY:
    case SELECT_SURFACE_POINTS_INTERACTIVELY:
      pipeline = vtkSMInteractiveSelectionPipeline::GetInstance();
      break;

    case SELECT_SURFACE_POINTS_TOOLTIP:
    case SELECT_SURFACE_CELLS_TOOLTIP:
      pipeline = vtkSMTooltipSelectionPipeline::GetInstance();
      break;

    default:
      qCritical("Invalid call to pqRenderViewSelectionReaction::preSelection");
      return;
  }

  if (x < 0 || y < 0 || x >= size[0] || y >= size[1])
  {
    // If the cursor goes out of the render window we hide the
    // interactive selection
    pipeline->Hide(rmp);
    this->UpdateTooltip();
    return;
  }

  int region[4] = { x, y, x, y };

  vtkNew<vtkCollection> selectedRepresentations;
  vtkNew<vtkCollection> selectionSources;
  bool status = false;
  switch (this->Mode)
  {
    case SELECT_SURFACE_POINTDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLDATA_INTERACTIVELY:
    {
      pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
      if (repr)
      {
        vtkSMStringVectorProperty* prop =
          vtkSMStringVectorProperty::SafeDownCast(repr->getProxy()->GetProperty("ColorArrayName"));
        if (prop)
        {
          int association = std::atoi(prop->GetElement(3));
          const char* arrayName = prop->GetElement(4);

          if (association == vtkDataObject::CELL &&
            this->Mode == SELECT_SURFACE_CELLDATA_INTERACTIVELY)
          {
            status = rmp->SelectSurfaceCells(
              region, selectedRepresentations, selectionSources, false, 0, false, arrayName);
          }
          if (association == vtkDataObject::POINT &&
            this->Mode == SELECT_SURFACE_POINTDATA_INTERACTIVELY)
          {
            status = rmp->SelectSurfacePoints(
              region, selectedRepresentations, selectionSources, false, 0, false, arrayName);
          }
        }
      }
    }
    break;

    case SELECT_SURFACE_CELLS_INTERACTIVELY:
      status = rmp->SelectSurfaceCells(region, selectedRepresentations, selectionSources);
      break;

    case SELECT_SURFACE_POINTS_INTERACTIVELY:
      status = rmp->SelectSurfacePoints(region, selectedRepresentations, selectionSources);
      break;

    case SELECT_SURFACE_POINTS_TOOLTIP:
      status = rmp->SelectSurfacePoints(region, selectedRepresentations, selectionSources);
      break;

    case SELECT_SURFACE_CELLS_TOOLTIP:
      status = rmp->SelectSurfaceCells(region, selectedRepresentations, selectionSources);
      break;

    default:
      qCritical("Invalid call to pqRenderViewSelectionReaction::preSelection");
      return;
  }

  if (status)
  {
    BEGIN_UNDO_EXCLUDE();
    pipeline->Show(vtkSMSourceProxy::SafeDownCast(selectedRepresentations->GetItemAsObject(0)),
      vtkSMSourceProxy::SafeDownCast(selectionSources->GetItemAsObject(0)), rmp);
    END_UNDO_EXCLUDE();
  }
  else
  {
    pipeline->Hide(rmp);
  }
  this->UpdateTooltip();
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::fastPreSelection()
{
  if (pqRenderViewSelectionReaction::ActiveReaction != this)
  {
    qWarning("Unexpected call to fastPreSelection.");
    return;
  }

  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  assert(rmp != nullptr);

  int x = rmp->GetInteractor()->GetEventPosition()[0];
  int y = rmp->GetInteractor()->GetEventPosition()[1];
  this->MousePosition[0] = x;
  this->MousePosition[1] = y;

  int region[4] = { x, y, x, y };

  vtkNew<vtkCollection> selectedRepresentations;
  vtkNew<vtkCollection> selectionSources;
  bool status = false;
  switch (this->Mode)
  {
    case SELECT_SURFACE_POINTDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLDATA_INTERACTIVELY:
    {
      pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
      if (repr)
      {
        vtkSMStringVectorProperty* prop =
          vtkSMStringVectorProperty::SafeDownCast(repr->getProxy()->GetProperty("ColorArrayName"));
        if (prop)
        {
          int association = std::atoi(prop->GetElement(3));
          const char* arrayName = prop->GetElement(4);

          if (association == vtkDataObject::CELL &&
            this->Mode == SELECT_SURFACE_CELLDATA_INTERACTIVELY)
          {
            status = rmp->SelectSurfaceCells(
              region, selectedRepresentations, selectionSources, false, 0, false, arrayName);
          }
          if (association == vtkDataObject::POINT &&
            this->Mode == SELECT_SURFACE_POINTDATA_INTERACTIVELY)
          {
            status = rmp->SelectSurfacePoints(
              region, selectedRepresentations, selectionSources, false, 0, false, arrayName);
          }
        }
      }
    }
    break;

    case SELECT_SURFACE_CELLS_INTERACTIVELY:
      status = rmp->SelectSurfaceCells(region, selectedRepresentations, selectionSources);
      break;

    case SELECT_SURFACE_POINTS_INTERACTIVELY:
      status = rmp->SelectSurfacePoints(region, selectedRepresentations, selectionSources);
      break;

    default:
      qCritical("Invalid call to pqRenderViewSelectionReaction::fastPreSelection");
      return;
  }

  if (status)
  {
    vtkSMPVRepresentationProxy* repr =
      vtkSMPVRepresentationProxy::SafeDownCast(selectedRepresentations->GetItemAsObject(0));

    if (this->CurrentRepresentation != nullptr && repr != this->CurrentRepresentation)
    {
      vtkSMSessionProxyManager* pxm = repr->GetSessionProxyManager();
      vtkSMProxy* emptySel = pxm->NewProxy("sources", "IDSelectionSource");

      vtkSMPropertyHelper(this->CurrentRepresentation, "Selection").Set(emptySel);
      this->CurrentRepresentation->UpdateVTKObjects();
      emptySel->Delete();
    }

    this->CurrentRepresentation = repr;

    vtkSMSourceProxy* sel = vtkSMSourceProxy::SafeDownCast(selectionSources->GetItemAsObject(0));

    vtkSMPropertyHelper(repr, "Selection").Set(sel);
    repr->UpdateVTKObjects();
  }
  else if (this->CurrentRepresentation != nullptr)
  {
    vtkSMSessionProxyManager* pxm = rmp->GetSessionProxyManager();
    vtkSMProxy* emptySel = pxm->NewProxy("sources", "IDSelectionSource");

    vtkSMPropertyHelper(this->CurrentRepresentation, "Selection").Set(emptySel);
    this->CurrentRepresentation->UpdateVTKObjects();
    this->CurrentRepresentation = nullptr;
    emptySel->Delete();
  }

  rmp->StillRender();
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::onMouseStop()
{
  this->MouseMoving = false;
  this->UpdateTooltip();
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::UpdateTooltip()
{
  if (this->Mode != SELECT_SURFACE_POINTS_TOOLTIP && this->Mode != SELECT_SURFACE_CELLS_TOOLTIP)
  {
    return;
  }

  vtkSMTooltipSelectionPipeline* pipeline = vtkSMTooltipSelectionPipeline::GetInstance();

  int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  if (this->Mode == SELECT_SURFACE_CELLS_TOOLTIP)
  {
    association = vtkDataObject::FIELD_ASSOCIATION_CELLS;
  }
  bool showTooltip;
  if (pipeline->CanDisplayTooltip(showTooltip))
  {
    std::string tooltipText;
    if (showTooltip && !this->MouseMoving && pipeline->GetTooltipInfo(association, tooltipText))
    {
      QWidget* widget = this->View->widget();

      // Take DPI scaling into account for the transformation
      qreal dpr = widget->devicePixelRatioF();

      // Convert renderer based position to a global position
      QPoint pos = widget->mapToGlobal(QPoint(
        this->MousePosition[0] / dpr, widget->size().height() - (this->MousePosition[1] / dpr)));

      QToolTip::showText(pos, tooltipText.c_str());
    }
    else
    {
      QToolTip::hideText();
    }
  }
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::onLeftButtonRelease()
{
  if (pqRenderViewSelectionReaction::ActiveReaction != this)
  {
    qWarning("Unexpected call to onLeftButtonRelease.");
    return;
  }

  vtkSMRenderViewProxy* viewProxy = this->View->getRenderViewProxy();
  assert(viewProxy != nullptr);

  int x = viewProxy->GetInteractor()->GetEventPosition()[0];
  int y = viewProxy->GetInteractor()->GetEventPosition()[1];
  if (x < 0 || y < 0)
  {
    // sometimes when the cursor goes quickly out of the window we receive -1
    // the rest of the code hangs in that case.
    return;
  }

  int selectionModifier = this->getSelectionModifier();
  if (selectionModifier == pqView::PV_SELECTION_DEFAULT)
  {
    selectionModifier = pqView::PV_SELECTION_ADDITION;
  }

  int region[4] = { x, y, x, y };

  switch (this->Mode)
  {
    case SELECT_SURFACE_POINTDATA_INTERACTIVELY:
    case SELECT_SURFACE_CELLDATA_INTERACTIVELY:
    {
      pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
      if (repr)
      {
        vtkSMStringVectorProperty* prop =
          vtkSMStringVectorProperty::SafeDownCast(repr->getProxy()->GetProperty("ColorArrayName"));
        if (prop)
        {
          int association = std::atoi(prop->GetElement(3));
          const char* arrayName = prop->GetElement(4);

          if (association == vtkDataObject::CELL)
          {
            this->View->selectOnSurface(region, selectionModifier, arrayName);
          }
          else
          {
            this->View->selectPointsOnSurface(region, selectionModifier, arrayName);
          }
        }
      }
    }
    break;

    case SELECT_SURFACE_CELLS_INTERACTIVELY:
      this->View->selectOnSurface(region, selectionModifier);
      break;

    case SELECT_SURFACE_POINTS_INTERACTIVELY:
      this->View->selectPointsOnSurface(region, selectionModifier);
      break;

    default:
      qCritical("Invalid call to pqRenderViewSelectionReaction::onLeftButtonRelease");
      break;
  }
}

//-----------------------------------------------------------------------------
int pqRenderViewSelectionReaction::getSelectionModifier()
{
  int selectionModifier = this->Superclass::getSelectionModifier();

  vtkSMRenderViewProxy* rmp = this->View->getRenderViewProxy();
  assert(rmp != nullptr);

  bool ctrl = rmp->GetInteractor()->GetControlKey() == 1;
  bool shift = rmp->GetInteractor()->GetShiftKey() == 1;
  if (ctrl && shift)
  {
    selectionModifier = pqView::PV_SELECTION_TOGGLE;
  }
  else if (ctrl)
  {
    selectionModifier = pqView::PV_SELECTION_ADDITION;
  }
  else if (shift)
  {
    selectionModifier = pqView::PV_SELECTION_SUBTRACTION;
  }
  return selectionModifier;
}

bool pqRenderViewSelectionReaction::isCompatible(SelectionMode mode)
{
  if (this->Mode == mode)
  {
    return true;
  }
  else if ((this->Mode == SELECT_SURFACE_CELLS || this->Mode == SELECT_SURFACE_CELLS_POLYGON ||
             this->Mode == SELECT_SURFACE_CELLS_INTERACTIVELY) &&
    (mode == SELECT_SURFACE_CELLS || mode == SELECT_SURFACE_CELLS_POLYGON ||
             mode == SELECT_SURFACE_CELLS_INTERACTIVELY))
  {
    return true;
  }
  else if ((this->Mode == SELECT_SURFACE_POINTS || this->Mode == SELECT_SURFACE_POINTS_POLYGON ||
             this->Mode == SELECT_SURFACE_POINTS_INTERACTIVELY) &&
    (mode == SELECT_SURFACE_POINTS || mode == SELECT_SURFACE_POINTS_POLYGON ||
             mode == SELECT_SURFACE_POINTS_INTERACTIVELY))
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqRenderViewSelectionReaction::onWheelRotate()
{
  if (pqRenderViewSelectionReaction::ActiveReaction == this)
  {
    this->onMouseMove();
  }
}
