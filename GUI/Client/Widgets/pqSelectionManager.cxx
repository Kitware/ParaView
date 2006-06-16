/*=========================================================================

   Program:   ParaQ
   Module:    pqSelectionManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqSelectionManager.h"
#include "pqRenderModule.h"

#include "qdebug.h"

#include "vtkCommand.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMRenderModuleProxy.h"

//---------------------------------------------------------------------------
// Observer for the start and end interaction events
class vtkPQSelectionObserver : public vtkCommand
{
public:
  static vtkPQSelectionObserver *New() 
    { return new vtkPQSelectionObserver; }

  virtual void Execute(vtkObject*, unsigned long event, void* data)
    {
      if (this->SelectionManager)
        {
        this->SelectionManager->processEvents(event);
        }
    }

  vtkPQSelectionObserver() : SelectionManager(0) 
    {
    }

  pqSelectionManager* SelectionManager;
};

//-----------------------------------------------------------------------------
class pqSelectionManagerImplementation
{
public:
  pqSelectionManagerImplementation() : 
    RubberBand(0),
    SavedStyle(0),
    RenderModule(0),
    SelectionObserver(0),
    Xs(0), Ys(0), Xe(0), Ye(0)
    {
    }

  ~pqSelectionManagerImplementation() 
    {
      if (this->RubberBand)
        {
        this->RubberBand->Delete();
        this->RubberBand = 0;
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
  
  vtkInteractorStyleRubberBandPick* RubberBand;
  vtkInteractorObserver* SavedStyle;
  pqRenderModule* RenderModule;
  vtkPQSelectionObserver* SelectionObserver;

  int Xs, Ys, Xe, Ye;
};

//-----------------------------------------------------------------------------
pqSelectionManager::pqSelectionManager(QObject* _parent/*=null*/) :
  QObject(_parent)
{
  this->Implementation = new pqSelectionManagerImplementation;
  this->Implementation->RubberBand = vtkInteractorStyleRubberBandPick::New();

  this->Implementation->SelectionObserver = vtkPQSelectionObserver::New();
  this->Implementation->SelectionObserver->SelectionManager = this;
  this->Mode = INTERACT;
}

//-----------------------------------------------------------------------------
pqSelectionManager::~pqSelectionManager()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::switchToSelection()
{
  if (!this->Implementation->RenderModule)
    {
    qDebug("No render module specified. Cannot switch to selection");
    return;
    }

  if (this->setInteractorStyleToSelect(this->Implementation->RenderModule))
    {
    this->Mode = SELECT;;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::switchToInteraction()
{
  if (!this->Implementation->RenderModule)
    {
    qDebug("No render module specified. Cannot switch to interaction");
    return;
    }

  if (this->setInteractorStyleToInteract(this->Implementation->RenderModule))
    {
    this->Mode = INTERACT;
    }
}

//-----------------------------------------------------------------------------
int pqSelectionManager::setInteractorStyleToSelect(pqRenderModule* rm)
{
  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
  if (!rmp)
    {
    qDebug("No render module proxy specified. Cannot switch to selection");
    return 0;
    }

  vtkRenderWindowInteractor* rwi = rmp->GetInteractor();
  if (!rwi)
    {
    qDebug("No interactor specified. Cannot switch to selection");
    return 0;
    }

  //start watching left mouse actions to get a begin and end pixel
  rwi->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->Implementation->SelectionObserver);
  rwi->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->Implementation->SelectionObserver);

  this->Implementation->SavedStyle = rwi->GetInteractorStyle();
  this->Implementation->SavedStyle->Register(0);
  rwi->SetInteractorStyle(this->Implementation->RubberBand);
  
  this->Implementation->RubberBand->StartSelect();

  return 1;
}

//-----------------------------------------------------------------------------
int pqSelectionManager::setInteractorStyleToInteract(pqRenderModule* rm)
{
  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
  if (!rmp)
    {
    qDebug("No render module proxy specified. Cannot switch to interaction");
    return 0;
    }

  vtkRenderWindowInteractor* rwi = rmp->GetInteractor();
  if (!rwi)
    {
    qDebug("No interactor specified. Cannot switch to interaction");
    return 0;
    }

  if (!this->Implementation->SavedStyle)
    {
    qDebug("No previous style defined. Cannot switch to interaction.");
    return 0;
    }

  rwi->SetInteractorStyle(this->Implementation->SavedStyle);
  rwi->RemoveObserver(this->Implementation->SelectionObserver);
  this->Implementation->SavedStyle->UnRegister(0);
  this->Implementation->SavedStyle = 0;

  return 1;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::activeRenderModuleChanged(pqRenderModule* rm)
{
  if (!rm)
    {
    return;
    }
  
  if (this->Mode == SELECT)
    {
    if (this->Implementation->RenderModule)
      {
      this->setInteractorStyleToInteract(this->Implementation->RenderModule);
      }
    this->setInteractorStyleToSelect(rm);
    }

  this->Implementation->RenderModule = rm;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::processEvents(unsigned long event)
{
  if (!this->Implementation->RenderModule)
    {
    qDebug("No render module specified. Cannot switch to interaction");
    return;
    }

  vtkSMRenderModuleProxy* rmp = 
    this->Implementation->RenderModule->getRenderModuleProxy();
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

  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      this->Implementation->Xs = eventpos[0];
      this->Implementation->Ys = eventpos[1];
      break;
    case vtkCommand::LeftButtonReleaseEvent:      
      this->Implementation->Xe = eventpos[0];
      this->Implementation->Ye = eventpos[1];
      this->updateSelection(rmp);
      break;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::updateSelection(vtkSMRenderModuleProxy* rmp)
{
  vtkPVClientServerIdCollectionInformation* idInfo = 
    rmp->Pick(this->Implementation->Xs, this->Implementation->Ys,
              this->Implementation->Xe, this->Implementation->Ye);

  int numProps = idInfo->GetLength();
  for (int i = 0; i < numProps; i++)
    {
    vtkClientServerID id = idInfo->GetID(i);
    cout << id.ID << endl;
    //vtkSMProxy *objProxy = 
    //renderModuleProxy->GetProxyFromPropID(&ID, this->SelectionType);
    }
  idInfo->Delete();
}
