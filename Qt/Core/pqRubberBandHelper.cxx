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
#include "pqRenderView.h"

// Qt Includes.
#include <QCursor>
#include <QPointer>
#include <QTimer>
#include <QWidget>

// ParaView includes.
#include "vtkCommand.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkInteractorStyleRubberBandZoom.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSMRenderViewProxy.h"

#include "zoom.xpm"

//---------------------------------------------------------------------------
// Observer for the start and end interaction events
class pqRubberBandHelper::vtkPQSelectionObserver : public vtkCommand
{
public:
  static vtkPQSelectionObserver *New()
    { return new vtkPQSelectionObserver; }

  virtual void Execute(vtkObject*, unsigned long event, void* )
    {
      if (this->RubberBandHelper)
        {
        this->RubberBandHelper->processEvents(event);
        }
    }

  vtkPQSelectionObserver() : RubberBandHelper(0)
    {
    }

  pqRubberBandHelper* RubberBandHelper;
};

//-----------------------------------------------------------------------------
void pqRubberBandHelper::ReorderBoundingBox(int src[4], int dest[4])
{
  dest[0] = (src[0] < src[2])? src[0] : src[2];
  dest[1] = (src[1] < src[3])? src[1] : src[3];
  dest[2] = (src[0] < src[2])? src[2] : src[0];
  dest[3] = (src[1] < src[3])? src[3] : src[1];
}

//---------------------------------------------------------------------------
class pqRubberBandHelper::pqInternal
{
public:
  //the style I use to draw the rubber band
  vtkSmartPointer<vtkInteractorStyleRubberBandPick> RubberBandStyle;

  // style used for rubber band zoom.
  vtkSmartPointer<vtkInteractorStyleRubberBandZoom> ZoomStyle;

  // Saved style to return to after rubber band finishes
  vtkSmartPointer<vtkInteractorObserver> SavedStyle;

  // Observer for mouse clicks.
  vtkSmartPointer<vtkPQSelectionObserver> SelectionObserver;

  // Current render view.
  QPointer<pqRenderView> RenderView;

  QCursor ZoomCursor;

  pqInternal(pqRubberBandHelper* parent) :
    ZoomCursor(QPixmap(zoom_xpm), 11, 11)
    {
    this->RubberBandStyle =
      vtkSmartPointer<vtkInteractorStyleRubberBandPick>::New();
    this->ZoomStyle = vtkSmartPointer<vtkInteractorStyleRubberBandZoom>::New();
    this->SelectionObserver =
      vtkSmartPointer<vtkPQSelectionObserver>::New();
    this->SelectionObserver->RubberBandHelper = parent;
    }

  ~pqInternal()
    {
    this->SelectionObserver->RubberBandHelper = 0;
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
    return;
    }

  if (this->DisableCount == 0 && this->Internal->RenderView)
    {
    vtkSMRenderViewProxy* proxy =
      this->Internal->RenderView->getRenderViewProxy();
    emit this->enableSurfaceSelection(
      proxy->IsSelectVisibleCellsAvailable() == NULL);
    emit this->enableSurfacePointsSelection(
      proxy->IsSelectVisiblePointsAvailable() == NULL);
    emit this->enableFrustumSelection(true);
    emit this->enableFrustumPointSelection(true);
    emit this->enableZoom(true);
    emit this->enablePick(true);
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

  vtkRenderWindowInteractor* rwi = rmp->GetInteractor();
  if (!rwi)
    {
    qDebug("No interactor specified. Cannot switch to selection");
    return 0;
    }

  //start watching left mouse actions to get a begin and end pixel
  this->Internal->SavedStyle = rwi->GetInteractorStyle();

  if (selectionMode == ZOOM)
    {
    rwi->SetInteractorStyle(this->Internal->ZoomStyle);
    this->Internal->RenderView->getWidget()->setCursor(
      this->Internal->ZoomCursor);
    }
  else if (selectionMode != PICK_ON_CLICK)
    {
    rwi->SetInteractorStyle(this->Internal->RubberBandStyle);
    this->Internal->RubberBandStyle->StartSelect();
    this->Internal->RenderView->getWidget()->setCursor(Qt::CrossCursor);
    }

  rwi->AddObserver(vtkCommand::LeftButtonPressEvent,
    this->Internal->SelectionObserver);
  rwi->AddObserver(vtkCommand::LeftButtonReleaseEvent,
    this->Internal->SelectionObserver);


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

  vtkRenderWindowInteractor* rwi = rmp->GetInteractor();
  if (!rwi)
    {
    qDebug("No interactor specified. Cannot switch to interaction");
    return 0;
    }

  if (!this->Internal->SavedStyle)
    {
    qDebug("No previous style defined. Cannot switch to interaction.");
    return 0;
    }

  rwi->SetInteractorStyle(this->Internal->SavedStyle);
  rwi->RemoveObserver(this->Internal->SelectionObserver);
  this->Internal->SavedStyle = 0;

  // set the interaction cursor
  this->Internal->RenderView->getWidget()->setCursor(QCursor());
  this->Mode = INTERACT;
  emit this->selectionModeChanged(this->Mode);
  emit this->interactionModeChanged(true);
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
void pqRubberBandHelper::endSelection()
{
  this->setRubberBandOff();
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::processEvents(unsigned long eventId)
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

  vtkRenderWindowInteractor* rwi = rmp->GetInteractor();
  if (!rwi)
    {
    qDebug("No interactor specified. Cannot switch to selection");
    return;
    }

  bool ctrl = rwi->GetControlKey();
  int* eventpos = rwi->GetEventPosition();
  switch(eventId)
    {
    case vtkCommand::LeftButtonPressEvent:
      this->Xs = eventpos[0];
      if (this->Xs < 0)
        {
        this->Xs = 0;
        }
      this->Ys = eventpos[1];
      if (this->Ys < 0)
        {
        this->Ys = 0;
        }
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      this->Xe = eventpos[0];
      if (this->Xe < 0)
        {
        this->Xe = 0;
        }
      this->Ye = eventpos[1];
      if (this->Ye < 0)
        {
        this->Ye = 0;
        }

      int rect[4] = {this->Xs, this->Ys, this->Xe, this->Ye};
      int rectOut[4];
      this->ReorderBoundingBox(rect, rectOut);
      if (this->Internal->RenderView)
        {
        switch (this->Mode)
          {
        case SELECT:
          this->Internal->RenderView->selectOnSurface(rectOut, ctrl);
          break;

        case SELECT_POINTS:
          this->Internal->RenderView->selectPointsOnSurface(rectOut, ctrl);
          break;

        case FRUSTUM:
          this->Internal->RenderView->selectFrustum(rectOut);
          break;

        case FRUSTUM_POINTS:
          this->Internal->RenderView->selectFrustumPoints(rectOut);
          break;

        case BLOCKS:
          this->Internal->RenderView->selectBlock(rectOut, ctrl);
          break;

        case ZOOM:
          // nothing to do.
          this->Internal->RenderView->resetCenterOfRotationIfNeeded();
          break;

        case PICK:
          this->Internal->RenderView->pick(rect);
          break;

        case PICK_ON_CLICK:
          if (rect[0] == rect[2] && rect[1] == rect[3])
            {
            this->Internal->RenderView->pick(rect);
            }
          break;
          }
        }
      emit this->selectionFinished(rectOut[0], rectOut[1], rectOut[2], rectOut[3]);
      break;
    }
}

