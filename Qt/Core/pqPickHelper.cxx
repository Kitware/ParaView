/*=========================================================================

   Program: ParaView
   Module:    pqPickHelper.cxx

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
#include "pqPickHelper.h"

// ParaView Server Manager includes.
#include "pqRenderView.h"

// Qt Includes.
#include <QWidget>
#include <QCursor>
#include <QPointer>

// ParaView includes.
#include "vtkCommand.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "pqRubberBandHelper.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"

//---------------------------------------------------------------------------
// Observer for the start and end interaction events
class pqPickHelper::vtkPQPickObserver : public vtkCommand
{
public:
  static vtkPQPickObserver *New() 
    { return new vtkPQPickObserver; }

  virtual void Execute(vtkObject*, unsigned long event, void* )
    {
      if (this->PickHelper)
        {
        this->PickHelper->processEvents(event);
        }
    }

  vtkPQPickObserver() : PickHelper(0) 
    {
    }

  pqPickHelper* PickHelper;
};

//---------------------------------------------------------------------------
class pqPickHelper::pqInternal
{
public:
  //the style I use to draw the rubber band
  vtkSmartPointer<vtkInteractorStyleRubberBandPick> PickStyle;

  // Saved style to return to after rubber band finishes
  vtkSmartPointer<vtkInteractorObserver> SavedStyle;

  // Observer for mouse clicks.
  vtkSmartPointer<vtkPQPickObserver> PickObserver;

  // Current render view.
  QPointer<pqRenderView> RenderView;

  pqInternal(pqPickHelper* parent)
    {
    this->PickStyle = 
      vtkSmartPointer<vtkInteractorStyleRubberBandPick>::New();
    this->PickObserver = 
      vtkSmartPointer<vtkPQPickObserver>::New();
    this->PickObserver->PickHelper = parent;
    }
  
  ~pqInternal()
    {
    this->PickObserver->PickHelper = 0;
    }
};

//-----------------------------------------------------------------------------
pqPickHelper::pqPickHelper(QObject* _parent/*=null*/)
: QObject(_parent)
{
  this->Internal = new pqInternal(this);
  this->Mode = INTERACT;
  this->DisableCount = 0;
}

//-----------------------------------------------------------------------------
pqPickHelper::~pqPickHelper()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPickHelper::DisabledPush()
{
  this->DisableCount++;
  if (this->DisableCount == 1)
    {
    emit this->enabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqPickHelper::DisabledPop()
{
  if (this->DisableCount > 0)
    {
    this->DisableCount--;
    if (this->DisableCount == 0 && this->Internal->RenderView)
      {
      emit this->enabled(true);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPickHelper::setView(pqView* view)
{
  pqRenderView* renView = qobject_cast<pqRenderView*>(view);
  if (renView == this->Internal->RenderView)
    {
    // nothing to do.
    return;
    }

  if (this->Internal->RenderView && this->Mode == PICK)
    {
    // Ensure that we are not currently in selection mode.
    this->setPickOff();
    }

  this->Internal->RenderView = renView;
  this->Mode = INTERACT;
  emit this->enabled((renView!=0) && (this->DisableCount==0));
}

//-----------------------------------------------------------------------------
int pqPickHelper::setPickOn(int selectionMode)
{
  pqRenderView* rm = this->Internal->RenderView;
  if (rm == 0 || this->Mode == selectionMode)
    {
    return 0;
    }
  // Ensure that it is not already in a selection mode
  if(this->Mode != INTERACT)
    {
    this->setPickOff();
    }

  vtkSMRenderViewProxy* rmp = rm->getRenderViewProxy();
  if (!rmp)
    {
    qDebug("Pick is unavailable without visible data.");
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
  rwi->SetInteractorStyle(this->Internal->PickStyle);
  
  rwi->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->Internal->PickObserver);
  rwi->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->Internal->PickObserver);

  this->Internal->PickStyle->StartSelect();

  this->Internal->RenderView->getWidget()->setCursor(Qt::CrossCursor);

  this->Mode = selectionMode;
  emit this->modeChanged(this->Mode);
  emit this->picking(true);
  emit this->startPicking();
  return 1;
}

//-----------------------------------------------------------------------------
int pqPickHelper::setPickOff()
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
  rwi->RemoveObserver(this->Internal->PickObserver);
  this->Internal->SavedStyle = 0;

  // set the interaction cursor
  this->Internal->RenderView->getWidget()->setCursor(QCursor());
  this->Mode = INTERACT;
  emit this->modeChanged(this->Mode);
  emit this->picking(false);
  emit this->stopPicking();
  return 1;
}

//-----------------------------------------------------------------------------
pqRenderView* pqPickHelper::getRenderView() const
{
  return this->Internal->RenderView;
}

//-----------------------------------------------------------------------------
void pqPickHelper::pick()
{
  this->beginPick();
  this->processEvents(vtkCommand::LeftButtonReleaseEvent);
  this->endPick();
}

//-----------------------------------------------------------------------------
void pqPickHelper::beginPick()
{
  this->setPickOn(PICK);
}

//-----------------------------------------------------------------------------
void pqPickHelper::endPick()
{
  this->setPickOff();
}

//-----------------------------------------------------------------------------
void pqPickHelper::processEvents(unsigned long eventId)
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

  int* eventpos = rwi->GetEventPosition();
  switch(eventId)
    {
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
  
      double center[3];
      center[0] = 0.0;
      center[1] = 0.0;
      center[2] = 0.0;     

      if (this->Internal->RenderView) 
        {
        if(this->Mode == PICK)
          {
          vtkRenderer *renderer = rmp->GetRenderer();

          double display[3], *world, cameraFP[4];
          display[0] = (double)this->Xe;
          display[1] = (double)this->Ye;
          double z = rmp->GetZBufferValue(this->Xe, this->Ye);
          if (z >= 0.999999)
            {
            // Missed.
            // Get camera focal point and position. Convert to display (screen)
            // coordinates in order to get a depth value for z-buffer.
            vtkCamera* camera = renderer->GetActiveCamera();
            camera->GetFocalPoint(cameraFP); cameraFP[3] = 1.0;            
            renderer->SetWorldPoint(cameraFP);
            renderer->WorldToDisplay();
            double *displayCoord = renderer->GetDisplayPoint();
            z = displayCoord[2];
            }

          // now convert the display point to world coordinates
          display[2] = z;

          renderer->SetDisplayPoint(display);
          renderer->DisplayToWorld ();
          world = renderer->GetWorldPoint();  
          for (int i=0; i < 3; i++) 
            {
            center[i] = world[i] / world[3];
            }
          }

        emit this->pickFinished(center[0],center[1],center[2]);
        }
      break;
    }
}

