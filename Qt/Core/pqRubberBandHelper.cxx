/*=========================================================================

   Program: ParaView
   Module:    pqRubberBandHelper.cxx

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

=========================================================================*/
#include "pqRubberBandHelper.h"

// ParaView Server Manager includes.
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqTimer.h"

// Qt Includes.
#include <QCursor>
#include <QPointer>
#include <QWidget>
#include <QMouseEvent>

// ParaView includes.
#include "pqPipelineSource.h"
#include "vtkCamera.h"
#include "vtkCell.h"
#include "vtkCollection.h"
#include "vtkInteractorStyleRubberBandZoom.h"
#include "vtkMath.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkNew.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include "zoom.xpm"

//---------------------------------------------------------------------------
class pqRubberBandHelper::pqInternal
{
public:
  // Current render view.
  QPointer<pqRenderView> RenderView;
  vtkSmartPointer<vtkCommand> Observer;
  int StartPosition[2];
  int PreviousInteractionMode;

  QCursor ZoomCursor;

  pqInternal(pqRubberBandHelper*) :
    ZoomCursor(QPixmap(zoom_xpm), 11, 11)
    {
    this->StartPosition[0] = this->StartPosition[1] = -1000;
    }

  ~pqInternal()
    {
    }
};

//-----------------------------------------------------------------------------
pqRubberBandHelper::pqRubberBandHelper(QObject* _parent/*=null*/)
: QObject(_parent)
{
  this->Internal = new pqInternal(this);
  this->Internal->Observer.TakeReference(
    vtkMakeMemberFunctionCommand(
      *this, &pqRubberBandHelper::onSelectionChanged));

  this->Mode = INTERACT;
  this->DisableCount = 0;
  QObject::connect(this, SIGNAL(enableSurfaceSelection(bool)),
    this, SIGNAL(enableBlockSelection(bool)));
}

//-----------------------------------------------------------------------------
pqRubberBandHelper::~pqRubberBandHelper()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::DisabledPush()
{
  this->DisableCount++;
  this->emitEnabledSignals();
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::DisabledPop()
{
  if (this->DisableCount > 0)
    {
    this->DisableCount--;
    this->emitEnabledSignals();
    }
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::emitEnabledSignals()
{
  if (this->DisableCount == 1 || !this->Internal->RenderView)
    {
    emit this->enableSurfaceSelection(false);
    emit this->enableZoom(false);
    emit this->enablePick(false);
    emit this->enableSurfacePointsSelection(false);
    emit this->enableFrustumSelection(false);
    emit this->enableFrustumPointSelection(false);
    return;
    }

  if (this->DisableCount == 0 && this->Internal->RenderView)
    {
    vtkSMRenderViewProxy* proxy =
      this->Internal->RenderView->getRenderViewProxy();
    emit this->enableSurfaceSelection(proxy->IsSelectionAvailable());
    emit this->enableSurfacePointsSelection(proxy->IsSelectionAvailable());
    emit this->enablePick(proxy->IsSelectionAvailable());
    emit this->enableFrustumSelection(true);
    emit this->enableFrustumPointSelection(true);
    emit this->enableZoom(true);
    }
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::setView(pqView* view)
{
  pqRenderView* renView = qobject_cast<pqRenderView*>(view);
  if (renView == this->Internal->RenderView)
    {
    // nothing to do.
    return;
    }

  if (this->Internal->RenderView && this->Mode != INTERACT)
    {
    // Before switching view, disable selection mode on the old active view.
    this->setRubberBandOff();
    }

  this->Internal->RenderView = renView;
  this->Mode = INTERACT;
  pqTimer::singleShot(10, this, SLOT(emitEnabledSignals()));
}

//-----------------------------------------------------------------------------
int pqRubberBandHelper::setRubberBandOn(int selectionMode)
{
  pqRenderView* rm = this->Internal->RenderView;
  if (rm == 0 || this->Mode == selectionMode)
    {
    return 0;
    }
  // Ensure that it is not already in a selection mode
  if(this->Mode != INTERACT)
    {
    this->setRubberBandOff();
    }

  vtkSMRenderViewProxy* rmp = rm->getRenderViewProxy();
  if (!rmp)
    {
    qDebug("Selection is unavailable without visible data.");
    return 0;
    }

  // Store the previous interaction mode so we get back to that exact same
  // interaction mode once we are done.
  vtkSMPropertyHelper(rmp, "InteractionMode").Get(
        &this->Internal->PreviousInteractionMode);

  if (selectionMode == ZOOM)
    {
    vtkSMPropertyHelper(rmp, "InteractionMode").Set(
      vtkPVRenderView::INTERACTION_MODE_ZOOM);
    rmp->UpdateVTKObjects();
    this->Internal->RenderView->getWidget()->setCursor(
      this->Internal->ZoomCursor);
    this->Internal->RenderView->getWidget()->installEventFilter(this);
    }
  else if (selectionMode == PICK_ON_CLICK)
    {
    // we don't use render-window-interactor for picking-on-clicking since we
    // don't want to change the default interaction style. Instead we install an
    // event filter to listen to mouse click events.
    this->Internal->RenderView->getWidget()->installEventFilter(this);
    }
  else // FAST_INTERSECT, SELECT, SELECT_POINTS, FRUSTUM, FRUSTUM_POINTS, BLOCKS, PICK
    {
    vtkSMPropertyHelper(rmp, "InteractionMode").Set(
      vtkPVRenderView::INTERACTION_MODE_SELECTION);
    rmp->AddObserver(vtkCommand::SelectionChangedEvent, this->Internal->Observer);
    rmp->UpdateVTKObjects();
    this->Internal->RenderView->getWidget()->setCursor(Qt::CrossCursor);
    }

  this->Mode = selectionMode;
  emit this->selectionModeChanged(this->Mode);
  emit this->interactionModeChanged(false);
  emit this->selecting(true);
  emit this->startSelection();
  return 1;
}

//-----------------------------------------------------------------------------
int pqRubberBandHelper::setRubberBandOff()
{
  pqRenderView* rm = this->Internal->RenderView;
  if (rm == 0 || this->Mode == INTERACT)
    {
    return 0;
    }

  vtkSMRenderViewProxy* rmp = rm->getRenderViewProxy();
  if (!rmp)
    {
    //qDebug("No render module proxy specified. Cannot switch to interaction");
    return 0;
    }

  vtkSMPropertyHelper(rmp, "InteractionMode").Set(
    this->Internal->PreviousInteractionMode);
  rmp->UpdateVTKObjects();
  rmp->RemoveObserver(this->Internal->Observer);

  this->Internal->RenderView->getWidget()->removeEventFilter(this);

  // set the interaction cursor
  this->Internal->RenderView->getWidget()->setCursor(QCursor());
  this->Mode = INTERACT;
  emit this->selectionModeChanged(this->Mode);
  emit this->interactionModeChanged(true);
  emit this->selecting(false);
  emit this->stopSelection();
  return 1;
}

//-----------------------------------------------------------------------------
bool pqRubberBandHelper::eventFilter(QObject *watched, QEvent *_event)
{
  if (this->Mode == PICK_ON_CLICK &&
    watched == this->Internal->RenderView->getWidget())
    {
    if (_event->type() == QEvent::MouseButtonPress)
      {
      QMouseEvent& mouseEvent = (*static_cast<QMouseEvent*>(_event));
      if (mouseEvent.button() == Qt::LeftButton)
        {
        this->Internal->StartPosition[0] = mouseEvent.x();
        this->Internal->StartPosition[1] = mouseEvent.y();
        }
      }
    else if (_event->type() == QEvent::MouseButtonRelease)
      {
      QMouseEvent& mouseEvent = (*static_cast<QMouseEvent*>(_event));
      if (mouseEvent.button() == Qt::LeftButton)
        {
        if (this->Internal->StartPosition[0] == mouseEvent.x() &&
          this->Internal->StartPosition[1] == mouseEvent.y())
          {
          // we need to flip Y.
          int height = this->Internal->RenderView->getWidget()->size().height();
          int region[4] = {mouseEvent.x(), height - mouseEvent.y(),
            mouseEvent.x(), height - mouseEvent.y()};
          this->onSelectionChanged(NULL, 0, region);
          }
        }
      this->Internal->StartPosition[0] = -1000;
      this->Internal->StartPosition[1] = -1000;
      }
    }
  else if (this->Mode == ZOOM &&
    watched == this->Internal->RenderView->getWidget())
    {
    if (_event->type() == QEvent::MouseButtonRelease)
      {
      QMouseEvent& mouseEvent = (*static_cast<QMouseEvent*>(_event));
      if (mouseEvent.button() == Qt::LeftButton)
        {
        pqTimer::singleShot(0, this, SLOT(delayedSelectionChanged()));
        }
      }
    }

  return this->Superclass::eventFilter(watched, _event);
}

//-----------------------------------------------------------------------------
pqRenderView* pqRubberBandHelper::getRenderView() const
{
  return this->Internal->RenderView;
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginSurfaceSelection()
{
  this->setRubberBandOn(SELECT);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginSurfacePointsSelection()
{
  this->setRubberBandOn(SELECT_POINTS);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginFrustumSelection()
{
  this->setRubberBandOn(FRUSTUM);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginFrustumPointsSelection()
{
  this->setRubberBandOn(FRUSTUM_POINTS);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginBlockSelection()
{
  this->setRubberBandOn(BLOCKS);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginZoom()
{
  this->setRubberBandOn(ZOOM);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginPick()
{
  this->setRubberBandOn(PICK);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginPickOnClick()
{
  this->setRubberBandOn(PICK_ON_CLICK);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::beginFastIntersect()
{
  this->setRubberBandOn(FAST_INTERSECT);
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::endSelection()
{
  this->setRubberBandOff();
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::onSelectionChanged(vtkObject*, unsigned long,
  void* vregion)

{
  if (!this->Internal->RenderView)
    {
    //qDebug("Selection is unavailable without visible data.");
    return;
    }

  vtkSMRenderViewProxy* rmp =
    this->Internal->RenderView->getRenderViewProxy();
  if (!rmp)
    {
    qDebug("No render module proxy specified. Cannot switch to selection");
    return;
    }

  bool ctrl = (rmp->GetInteractor()->GetControlKey() == 1);
  int* region = reinterpret_cast<int*>(vregion);
  switch (this->Mode)
    {
  case SELECT:
    this->Internal->RenderView->selectOnSurface(region, ctrl);
    break;

  case SELECT_POINTS:
    this->Internal->RenderView->selectPointsOnSurface(region, ctrl);
    break;

  case FRUSTUM:
    this->Internal->RenderView->selectFrustum(region);
    break;

  case FRUSTUM_POINTS:
    this->Internal->RenderView->selectFrustumPoints(region);
    break;

  case BLOCKS:
    this->Internal->RenderView->selectBlock(region, ctrl);
    break;

  case ZOOM:
    // nothing to do.
    this->setRubberBandOff();
    this->Internal->RenderView->resetCenterOfRotationIfNeeded();
    break;

  case PICK:
      {
      pqDataRepresentation* picked = this->Internal->RenderView->pick(region);
      vtkSMProxySelectionModel* selModel = 
        this->Internal->RenderView->getServer()->activeSourcesSelectionModel();
      if (selModel)
        {
        selModel->SetCurrentProxy(
          picked? picked->getOutputPortFromInput()->getOutputPortProxy(): NULL,
          vtkSMProxySelectionModel::CLEAR_AND_SELECT);
        }
      }
    break;

  case PICK_ON_CLICK:
    if (region[0] == region[2] && region[1] == region[3])
      {
      pqDataRepresentation* picked = this->Internal->RenderView->pick(region);
      // in pick-on-click, we don't change the current item when user clicked on
      // a blank area. BUG #11428.
      if (picked)
        {
        vtkSMProxySelectionModel* selModel = 
          this->Internal->RenderView->getServer()->activeSourcesSelectionModel();
        if (selModel)
          {
          selModel->SetCurrentProxy(
            picked->getOutputPortFromInput()->getOutputPortProxy(),
            vtkSMProxySelectionModel::CLEAR_AND_SELECT);
          }
        }
      }
    break;
  case FAST_INTERSECT:
    if (region[0] == region[2] && region[1] == region[3])
      {
      vtkSMRenderViewProxy* renderViewProxy =
          this->Internal->RenderView->getRenderViewProxy();
      vtkSMSessionProxyManager* spxm = renderViewProxy->GetSessionProxyManager();

      vtkNew<vtkCollection> representations;
      vtkNew<vtkCollection> sources;
      renderViewProxy->SelectSurfaceCells(region, representations.GetPointer(), sources.GetPointer(), false);

      if(representations->GetNumberOfItems() > 0 && sources->GetNumberOfItems() > 0)
        {
        vtkSMPVRepresentationProxy* rep =
            vtkSMPVRepresentationProxy::SafeDownCast(representations->GetItemAsObject(0));
        vtkSMProxy* input = vtkSMPropertyHelper(rep, "Input").GetAsProxy(0);
        vtkSMSourceProxy* selection = vtkSMSourceProxy::SafeDownCast(sources->GetItemAsObject(0));

        // Picking info
        double display[3] = { region[0], region[1], 0.5 };
        double linePoint1[3], linePoint2[3];
        double* world;

        vtkRenderer* renderer = renderViewProxy->GetRenderer();
        renderer->SetDisplayPoint(display);
        renderer->DisplayToWorld();
        world = renderer->GetWorldPoint();
        for (int i=0; i < 3; i++)
          {
          linePoint1[i] = world[i] / world[3];
          }
        renderer->GetActiveCamera()->GetPosition(linePoint2);

        // Compute the  intersection...
        double intersection[3] = {0,0,0};
        vtkSMProxy* pickingHelper = spxm->NewProxy("misc","PickingHelper");
        vtkSMPropertyHelper(pickingHelper, "Input").Set( input );
        vtkSMPropertyHelper(pickingHelper, "Selection").Set( selection );
        vtkSMPropertyHelper(pickingHelper, "PointA").Set(linePoint1, 3);
        vtkSMPropertyHelper(pickingHelper, "PointB").Set(linePoint2, 3);
        pickingHelper->UpdateVTKObjects();
        pickingHelper->UpdateProperty("Update",1);
        vtkSMPropertyHelper(pickingHelper, "Intersection").UpdateValueFromServer();
        vtkSMPropertyHelper(pickingHelper, "Intersection").Get(intersection, 3);
        pickingHelper->Delete();

        emit intersectionFinished(intersection[0], intersection[1], intersection[2]);
        }
      else
        {
        // Need to warn user when used in RenderServer mode
        if(!renderViewProxy->IsSelectionAvailable())
          {
          qWarning("Snapping to the surface is not available therefore "
                   "the camera focal point will be used to determine "
                   "the depth of the picking.");
          }

        // Use camera focal point to get some Zbuffer
        double cameraFP[4];
        vtkRenderer* renderer = renderViewProxy->GetRenderer();
        vtkCamera* camera = renderer->GetActiveCamera();
        camera->GetFocalPoint(cameraFP); cameraFP[3] = 1.0;
        renderer->SetWorldPoint(cameraFP);
        renderer->WorldToDisplay();
        double *displayCoord = renderer->GetDisplayPoint();

        // Handle display to world conversion
        double display[3] = {region[0], region[1], displayCoord[2]};
        double center[3];
        renderer->SetDisplayPoint(display);
        renderer->DisplayToWorld();
        double* world = renderer->GetWorldPoint();
        for (int i=0; i < 3; i++)
          {
          center[i] = world[i] / world[3];
          }
        emit intersectionFinished(center[0], center[1], center[2]);
        }
      }
    break;
    }
  if (region)
    {
    emit this->selectionFinished(region[0], region[1], region[2], region[3]);
    }
}
//-----------------------------------------------------------------------------
void pqRubberBandHelper::triggerFastIntersect()
{
  if (!this->Internal->RenderView)
    {
    //qDebug("Pick is unavailable without visible data.");
    return;
    }

  vtkSMRenderViewProxy* rmp =
    this->Internal->RenderView->getRenderViewProxy();
  if (!rmp)
    {
    qDebug("No render module proxy specified. Cannot switch to selection");
    return;
    }

  vtkRenderWindowInteractor* rwi = rmp->GetInteractor();
  if (!rwi)
    {
    qDebug("No interactor specified. Cannot switch to selection");
    return;
    }

  // Get region
  int* eventpos = rwi->GetEventPosition();
  int region[4] = { eventpos[0], eventpos[1], eventpos[0], eventpos[1] };

  // Trigger fast intersection
  this->beginFastIntersect();
  this->onSelectionChanged(NULL, 0, region);
  this->endSelection();
}
