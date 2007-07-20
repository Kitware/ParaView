/*=========================================================================

   Program: ParaView
   Module:    pqRubberBandHelper.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include <QWidget>
#include <QCursor>


// ParaView includes.
#include "vtkCommand.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkRenderWindowInteractor.h"

//---------------------------------------------------------------------------
// Observer for the start and end interaction events
class vtkPQSelectionObserver : public vtkCommand
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

//-----------------------------------------------------------------------------
pqRubberBandHelper::pqRubberBandHelper(QObject* _parent/*=null*/)
: QObject(_parent)
{
  this->RubberBandStyle = vtkInteractorStyleRubberBandPick::New();
  this->SelectionObserver = vtkPQSelectionObserver::New();
  this->SelectionObserver->RubberBandHelper = this;
  this->SavedStyle = NULL;
  this->RenderModule = NULL;
}

//-----------------------------------------------------------------------------
pqRubberBandHelper::~pqRubberBandHelper()
{
  if (this->RubberBandStyle)
    {
    this->RubberBandStyle->Delete();
    this->RubberBandStyle = 0;
    }
  if (this->SavedStyle)
    {
    this->SavedStyle->Delete();
    this->SavedStyle = 0;
    }
  if (this->SelectionObserver)
    {
    this->SelectionObserver->Delete();
    this->SelectionObserver = 0;
    }
}

//-----------------------------------------------------------------------------
int pqRubberBandHelper::setRubberBandOn(pqRenderView* rm)
{
  if (rm == 0)
    {
    rm = this->RenderModule;
    }
  if (rm == 0) 
    {
    return 0;
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
  this->SavedStyle = rwi->GetInteractorStyle();
  this->SavedStyle->Register(0);
  rwi->SetInteractorStyle(this->RubberBandStyle);
  
  rwi->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->SelectionObserver);
  rwi->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->SelectionObserver);

  this->RubberBandStyle->StartSelect();

  this->RenderModule->getWidget()->setCursor(Qt::CrossCursor);

  return 1;
}

//-----------------------------------------------------------------------------
int pqRubberBandHelper::setRubberBandOff(pqRenderView* rm)
{
  if (rm == 0)
    {
    rm = this->RenderModule;
    }
  if (rm == 0) 
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

  if (!this->SavedStyle)
    {
    qDebug("No previous style defined. Cannot switch to interaction.");
    return 0;
    }

  rwi->SetInteractorStyle(this->SavedStyle);
  rwi->RemoveObserver(this->SelectionObserver);
  this->SavedStyle->UnRegister(0);
  this->SavedStyle = 0;

  // set the interaction cursor
  this->RenderModule->getWidget()->setCursor(QCursor());

  return 1;
}

//-----------------------------------------------------------------------------
void pqRubberBandHelper::processEvents(unsigned long eventId)
{
  if (!this->RenderModule)
    {
    //qDebug("Selection is unavailable without visible data.");
    return;
    }

  vtkSMRenderViewProxy* rmp = 
    this->RenderModule->getRenderViewProxy();
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

      emit this->selectionFinished();
      break;
    }
}

