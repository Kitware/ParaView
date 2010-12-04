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
#include "pqServerManagerSelectionModel.h"

// Qt Includes.
#include <QCursor>
#include <QPointer>
#include <QTimer>
#include <QWidget>
#include <QMouseEvent>

// ParaView includes.
#include "vtkPVRenderView.h"
#include "vtkInteractorStyleRubberBandZoom.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

#include "zoom.xpm"

//---------------------------------------------------------------------------
class pqRubberBandHelper::pqInternal
{
public:
  // Current render view.
  QPointer<pqRenderView> RenderView;
  vtkSmartPointer<vtkCommand> Observer;
  int StartPosition[2];

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
  QTimer::singleShot(10, this, SLOT(emitEnabledSignals()));
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
  else
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
    vtkPVRenderView::INTERACTION_MODE_3D);
  rmp->UpdateVTKObjects();
  rmp->RemoveObserver(this->Internal->Observer);

  this->Internal->RenderView->getWidget()->removeEventFilter(this);

  // set the interaction cursor
  this->Internal->RenderView->getWidget()->setCursor(QCursor());
  this->Mode = INTERACT;
  emit this->selectionModeChanged(this->Mode);
  emit this->interactionModeChanged(true);
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
        QTimer::singleShot(0, this, SLOT(delayedSelectionChanged()));
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
      pqApplicationCore::instance()->getSelectionModel()->setCurrentItem(
        picked? picked->getOutputPortFromInput(): NULL,
        pqServerManagerSelectionModel::ClearAndSelect);
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
        pqApplicationCore::instance()->getSelectionModel()->setCurrentItem(
          picked->getOutputPortFromInput(),
          pqServerManagerSelectionModel::ClearAndSelect);
        }
      }
    break;
    }
  if (region)
    {
    emit this->selectionFinished(region[0], region[1], region[2], region[3]);
    }
}

