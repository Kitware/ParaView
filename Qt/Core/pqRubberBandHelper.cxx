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
#include "vtkIntArray.h"
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
  vtkWeakPointer<vtkObject> ObservedProxy;
  QPointer<pqRubberBandHelper> Owner;
  QPointer<pqRenderView> RenderView;
  int PreviousInteractionMode;
  unsigned long ObserverId;
  unsigned long PressEventObserverId;
  int LastPressedPosition[2];

  QCursor ZoomCursor;

  pqInternal(pqRubberBandHelper* owner) :
    ZoomCursor(QPixmap(zoom_xpm), 11, 11)
    {
    this->Owner = owner;
    this->ObserverId = this->PressEventObserverId = 0;
    this->LastPressedPosition[0] = this->LastPressedPosition[1] = -10000;
    }

  ~pqInternal()
    {
    this->RemoveObserver();
    }

  void AddSelectionObserver(vtkSMRenderViewProxy* proxyToObserver)
  {
    this->RemoveObserver();
    this->ObservedProxy = proxyToObserver;
    if(this->ObservedProxy)
      {
      this->ObserverId = this->ObservedProxy->AddObserver(
            vtkCommand::SelectionChangedEvent,
            this->Owner.data(),
            &pqRubberBandHelper::onSelectionChanged);
      }
  }

  void AddZoomObserver(vtkPVGenericRenderWindowInteractor* proxyToObserver)
  {
    this->RemoveObserver();
    this->ObservedProxy = proxyToObserver;
    if(this->ObservedProxy)
      {
      this->ObserverId = this->ObservedProxy->AddObserver(
            vtkCommand::LeftButtonReleaseEvent,
            this->Owner.data(),
            &pqRubberBandHelper::onZoom);
      }
  }


  void AddPolygonObserver(vtkSMRenderViewProxy* proxyToObserver)
  {
    this->RemoveObserver();
    this->ObservedProxy = proxyToObserver;
    if(this->ObservedProxy)
      {
      this->ObserverId = this->ObservedProxy->AddObserver(
            vtkCommand::SelectionChangedEvent,
            this->Owner.data(),
            &pqRubberBandHelper::onPolygonSelection);
      }
  }

  void AddPickObserver(vtkPVGenericRenderWindowInteractor* proxyToObserver)
  {
    this->RemoveObserver();
    this->ObservedProxy = proxyToObserver;
    if(this->ObservedProxy)
      {
      this->ObserverId = this->ObservedProxy->AddObserver(
            vtkCommand::LeftButtonReleaseEvent,
            this->Owner.data(),
            &pqRubberBandHelper::onPickOnClick);
      this->PressEventObserverId = this->ObservedProxy->AddObserver(
            vtkCommand::LeftButtonPressEvent,
            this,
            &pqRubberBandHelper::pqInternal::UpdatePressedPosition);
      }
  }

  void RemoveObserver()
  {
    if(this->ObservedProxy && this->ObserverId)
      {
      this->ObservedProxy->RemoveObserver(this->ObserverId);
      if(this->PressEventObserverId)
        {
        this->ObservedProxy->RemoveObserver(this->PressEventObserverId);
        }
      }
    this->ObserverId = 0;
    this->PressEventObserverId = 0;
  }

  void UpdatePressedPosition(vtkObject* obj, unsigned long, void*)
  {
    vtkPVGenericRenderWindowInteractor* interactor =
        vtkPVGenericRenderWindowInteractor::SafeDownCast(obj);
    if(interactor)
      {
      interactor->GetEventPosition(this->LastPressedPosition);
      }
  }

  bool IsSamePosition(int pos[2])
  {
    return (pos[0] == this->LastPressedPosition[0] && pos[1] == this->LastPressedPosition[1]);
  }

  void ClearPressPosition()
  {
    this->LastPressedPosition[0] = this->LastPressedPosition[1] = -1000;
  }
};

//-----------------------------------------------------------------------------
pqRubberBandHelper::pqRubberBandHelper(QObject* _parent/*=null*/)
: QObject(_parent)
{
  this->Internal = new pqInternal(this);

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
    emit this->enablePolygonPointsSelection(false);
    emit this->enablePolygonCellsSelection(false);
    return;
    }

  if (this->DisableCount == 0 && this->Internal->RenderView)
    {
    vtkSMRenderViewProxy* proxy =
      this->Internal->RenderView->getRenderViewProxy();
    emit this->enableSurfaceSelection(proxy ?
          NULL == proxy->IsSelectVisibleCellsAvailable() : false);
    emit this->enableSurfacePointsSelection(proxy ?
          NULL == proxy->IsSelectVisiblePointsAvailable() : false);
    emit this->enablePolygonCellsSelection(proxy ?
      NULL == proxy->IsSelectVisibleCellsAvailable() : false);
    emit this->enablePolygonPointsSelection(proxy ?
      NULL == proxy->IsSelectVisiblePointsAvailable() : false);
    emit this->enablePick(proxy ?
          proxy->IsSelectionAvailable() : false);
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
    this->Internal->AddZoomObserver(rmp->GetInteractor());
    rmp->UpdateVTKObjects();
    this->Internal->RenderView->setCursor(
      this->Internal->ZoomCursor);
    }
  else if (selectionMode == POLYGON_POINTS || selectionMode == POLYGON_CELLS)
    {
    vtkSMPropertyHelper(rmp, "InteractionMode").Set(
      vtkPVRenderView::INTERACTION_MODE_POLYGON);
    this->Internal->AddPolygonObserver(rmp);
    rmp->UpdateVTKObjects();
    this->Internal->RenderView->setCursor(Qt::PointingHandCursor);
    }
  else if (selectionMode == PICK_ON_CLICK)
    {
    this->Internal->AddPickObserver(rmp->GetInteractor());
    }
  else // FAST_INTERSECT, SELECT, SELECT_POINTS, FRUSTUM, FRUSTUM_POINTS, BLOCKS, PICK
    {
    vtkSMPropertyHelper(rmp, "InteractionMode").Set(
      vtkPVRenderView::INTERACTION_MODE_SELECTION);
    this->Internal->AddSelectionObserver(rmp);
    rmp->UpdateVTKObjects();
    this->Internal->RenderView->setCursor(Qt::CrossCursor);
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
  this->Internal->RemoveObserver();

  // set the interaction cursor
  this->Internal->RenderView->setCursor(QCursor());
  this->Mode = INTERACT;
  emit this->selectionModeChanged(this->Mode);
  emit this->interactionModeChanged(true);
  emit this->selecting(false);
  emit this->stopSelection();
  return 1;
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

  case POLYGON_POINTS:
    // nothing to do.
    this->setRubberBandOff();
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
      double world[3];
      renderViewProxy->ConvertDisplayToPointOnSurface(region, world);
      emit intersectionFinished(world[0], world[1], world[2]);
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
//-----------------------------------------------------------------------------
void pqRubberBandHelper::onZoom(vtkObject*, unsigned long, void*)
{
  pqTimer::singleShot(0, this, SLOT(delayedSelectionChanged()));
}
//-----------------------------------------------------------------------------
void pqRubberBandHelper::onPolygonSelection(vtkObject*, unsigned long,
 void* vpolygonpoints)
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

  vtkObject* pointArray = static_cast<vtkObject*>(vpolygonpoints);
  if(vpolygonpoints && vtkIntArray::SafeDownCast(pointArray))
    {
    vtkIntArray* polygonPoints = vtkIntArray::SafeDownCast(pointArray);
    bool ctrl = (rmp->GetInteractor()->GetControlKey() == 1);
    switch (this->Mode)
      {
      case POLYGON_POINTS:
        this->Internal->RenderView->selectPolygonPoints(polygonPoints, ctrl);
        break;
      case POLYGON_CELLS:
        this->Internal->RenderView->selectPolygonCells(polygonPoints, ctrl);
        break;
      }
    this->endSelection();
    }
}
//-----------------------------------------------------------------------------
void pqRubberBandHelper::onPickOnClick(vtkObject*, unsigned long, void*)
{
  if(!this->Internal->RenderView->getRenderViewProxy() ||
     !this->Internal->RenderView->getRenderViewProxy()->GetInteractor())
    {
    return; // We can not do anything
    }

  int region[4] = {0,0,0,0};

  // Get point coordinate
  pqRenderView* rm = this->Internal->RenderView;
  rm->getRenderViewProxy()->GetInteractor()->GetEventPosition(region);

  if(this->Internal->IsSamePosition(region))
    {
    // we need to flip Y.
    int height = this->Internal->RenderView->getWidget()->size().height();
    region[1] = height - region[1];

    // Need to duplicate [x,y,0,0] to be [x,y,x,y]
    region[2] = region[0];
    region[3] = region[1];

    // Trigger event
    this->onSelectionChanged(NULL, 0, region);

    // Reset presion press position
    this->Internal->ClearPressPosition();
    }
}
