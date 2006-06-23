/*=========================================================================

   Program: ParaView
   Module:    pqSelectionManager.cxx

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

#include "pqSelectionManager.h"

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"

#include "qdebug.h"

#include "vtkCommand.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>

//---------------------------------------------------------------------------
// Observer for the start and end interaction events
class vtkPQSelectionObserver : public vtkCommand
{
public:
  static vtkPQSelectionObserver *New() 
    { return new vtkPQSelectionObserver; }

  virtual void Execute(vtkObject*, unsigned long event, void* )
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
    SelectionProxy(0),
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
  vtkSMSourceProxy* SelectionProxy;

  int Xs, Ys, Xe, Ye;

  typedef vtkstd::map<vtkIdType, vtkSmartPointer<vtkSMSourceProxy> > 
       ServerSelectionsType;
  ServerSelectionsType ServerSelections;

  double Verts[32];
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
    this->Mode = SELECT;
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

  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
  if (!rmp)
    {
    qDebug("No render module proxy specified. Cannot create selection object");
    return;
    }

  vtkIdType connId = rmp->GetConnectionID();
  vtkSMSourceProxy* selectionProxy = 0;
  pqSelectionManagerImplementation::ServerSelectionsType::iterator iter = 
    this->Implementation->ServerSelections.find(connId);
  if (iter != this->Implementation->ServerSelections.end())
    {
    selectionProxy = iter->second;
    }

  if (!selectionProxy)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    selectionProxy = vtkSMSourceProxy::SafeDownCast(
      pxm->NewProxy("sources", "PropAndCellSelect"));
    if (!selectionProxy)
      {
      qDebug("An appropriate selection proxy could not be created. "
             "No selection is possible");
      return;
      }
    selectionProxy->SetConnectionID(connId);
    selectionProxy->SetServers(vtkProcessModule::DATA_SERVER);
    this->Implementation->ServerSelections[connId] = selectionProxy;
    selectionProxy->Delete();
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::processEvents(unsigned long eventId)
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

  switch(eventId)
    {
    case vtkCommand::LeftButtonPressEvent:
      this->Implementation->Xs = eventpos[0];
      this->Implementation->Ys = eventpos[1];
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      this->updateSelection(eventpos, rmp);
      break;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::updateSelection(
  int* eventpos, vtkSMRenderModuleProxy* rmp)
{
  vtkRenderWindowInteractor* rwi = rmp->GetInteractor();
  vtkRenderer *renderer = rwi->FindPokedRenderer(eventpos[0], eventpos[1]);

  this->Implementation->Xe = eventpos[0];
  this->Implementation->Ye = eventpos[1];

  double x0 = (double)((this->Implementation->Xs < this->Implementation->Xe) ? this->Implementation->Xs : this->Implementation->Xe);
  double y0 = (double)((this->Implementation->Ys < this->Implementation->Ye) ? this->Implementation->Ys : this->Implementation->Ye);
  double x1 = (double)((this->Implementation->Xs > this->Implementation->Xe) ? this->Implementation->Xs : this->Implementation->Xe);
  double y1 = (double)((this->Implementation->Ys > this->Implementation->Ye) ? this->Implementation->Ys : this->Implementation->Ye);

  this->Implementation->Xs = (int)x0;
  this->Implementation->Ys = (int)y0;
  this->Implementation->Xe = (int)x1;
  this->Implementation->Ye = (int)y1;
  
  if (x0 == x1)
    {
    x0 -= 0.5;
    x1 += 0.5;
    }
  if (y0 == y1)
    {
    y0 -= 0.5;
    y1 += 0.5;
    }
  
  renderer->SetDisplayPoint(x0, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Implementation->Verts[0]);
  
  renderer->SetDisplayPoint(x0, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Implementation->Verts[4]);
  
  renderer->SetDisplayPoint(x0, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Implementation->Verts[8]);
  
  renderer->SetDisplayPoint(x0, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Implementation->Verts[12]);
  
  renderer->SetDisplayPoint(x1, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Implementation->Verts[16]);
  
  renderer->SetDisplayPoint(x1, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Implementation->Verts[20]);
  
  renderer->SetDisplayPoint(x1, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Implementation->Verts[24]);
  
  renderer->SetDisplayPoint(x1, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Implementation->Verts[28]);

  // pick with the given rectange. this will need some work when rendering
  // in parallel, specially with tiled display
  vtkPVClientServerIdCollectionInformation* idInfo = 
    rmp->Pick(this->Implementation->Xs, this->Implementation->Ys,
              this->Implementation->Xe, this->Implementation->Ye);

  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqServerManagerSelectionModel* selectionModel =
    pqApplicationCore::instance()->getSelectionModel();

  pqServerManagerSelection selection;
  int numProps = idInfo->GetLength();
  for (int i = 0; i < numProps; i++)
    {
    vtkClientServerID id = idInfo->GetID(i);
    // get the source proxy corresponding to the picked display
    // proxy
    vtkSMProxy *objProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::INPUT);
    pqPipelineSource* pqSource = model->getPQSource(objProxy);
    if (pqSource)
      {
      selection.push_back(pqSource);
      }
    }
  idInfo->Delete();

  selectionModel->select(
    selection, pqServerManagerSelectionModel::ClearAndSelect);

  vtkSMSourceProxy* selectionProxy = 
    this->Implementation->ServerSelections[rmp->GetConnectionID()];
  if (!selectionProxy)
    {
    qDebug("No selection proxy was created on the data server. "
           "Cannot select cells");
    }
}
