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
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"

#include "qdebug.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVSelectionInformation.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"

#include <vtkstd/algorithm>
#include <vtkstd/list>
#include <vtkstd/map>
#include <vtkstd/vector>

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
struct pqSelectionProxies
{
  vtkSmartPointer<vtkSMProxy> SelectorProxy;
  vtkSmartPointer<vtkSMProxy> GeometrySelectionProxy;
  vtkSmartPointer<vtkSMProxy> SourceSelectionProxy;
  vtkSmartPointer<vtkSMSourceProxy> GeometrySource;
};

//-----------------------------------------------------------------------------
struct pqSelectionDisplayProxies
{
  vtkSmartPointer<vtkSMDataObjectDisplayProxy> DisplayProxy;
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

  // traverse selection and extract prop ids (unique)
  void traverseSelection(vtkSelection* sel, vtkstd::list<int>& ids);
  
  vtkInteractorStyleRubberBandPick* RubberBand;
  vtkInteractorObserver* SavedStyle;
  pqRenderModule* RenderModule;
  pqRenderModule* SelectionRenderModule;
  vtkPQSelectionObserver* SelectionObserver;

  int Xs, Ys, Xe, Ye;

  typedef vtkstd::map<vtkIdType, pqSelectionProxies > 
     ServerSelectionsType;
  ServerSelectionsType ServerSelections;

  typedef vtkstd::map<vtkTypeUInt32, pqSelectionDisplayProxies > 
     DisplaysType;
  DisplaysType Displays;

  double Verts[32];
};

//-----------------------------------------------------------------------------
// traverse selection and extract prop ids (unique)
void pqSelectionManagerImplementation::traverseSelection(
  vtkSelection* sel, vtkstd::list<int>& ids)
{
  vtkInformation* properties = sel->GetProperties();
  if (properties && properties->Has(vtkSelection::PROP_ID()))
    {
    int id = properties->Get(vtkSelection::PROP_ID());
    if (vtkstd::find(ids.begin(), ids.end(), id) == ids.end())
      {
      ids.push_back(id);
      }
    }
  unsigned int numChildren = sel->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    this->traverseSelection(sel->GetChild(i), ids);
    }
}

//-----------------------------------------------------------------------------
pqSelectionManager::pqSelectionManager(QObject* _parent/*=null*/) :
  QObject(_parent)
{
  this->Implementation = new pqSelectionManagerImplementation;
  this->Implementation->RubberBand = vtkInteractorStyleRubberBandPick::New();

  this->Implementation->SelectionObserver = vtkPQSelectionObserver::New();
  this->Implementation->SelectionObserver->SelectionManager = this;
  this->Mode = INTERACT;

  pqApplicationCore* core = pqApplicationCore::instance();

  pqServerManagerModel* model = core->getServerManagerModel();
  // We need to clear selection when a source is removed. The source
  // that was deleted might have been selected.
  QObject::connect(model, 
                   SIGNAL(sourceRemoved(pqPipelineSource*)),
                   this, 
                   SLOT(sourceRemoved(pqPipelineSource*)));

  // Cleanup when a selection helper is unregistered.
  pqServerManagerObserver* observer = core->getPipelineData();
  QObject::connect(observer,
                   SIGNAL(proxyUnRegistered(QString, QString, vtkSMProxy*)),
                   this,
                   SLOT(proxyUnRegistered(QString, QString, vtkSMProxy*)));
}

//-----------------------------------------------------------------------------
pqSelectionManager::~pqSelectionManager()
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  if (pxm)
    {
    pqApplicationCore* core = pqApplicationCore::instance();
    pqServerManagerObserver* observer = core->getPipelineData();
    if (observer)
      {
      QObject::disconnect(observer, 0, this,0);
      }
    pqSelectionManagerImplementation::ServerSelectionsType::iterator iter =
      this->Implementation->ServerSelections.begin();
    for(; iter != this->Implementation->ServerSelections.end(); iter++)
      {      
      pxm->UnRegisterProxy(
        "selection_objects", iter->second.SelectorProxy->GetSelfIDAsString());
      }

    pqSelectionManagerImplementation::DisplaysType::iterator iter2 =
      this->Implementation->Displays.begin();
    for(; iter2 != this->Implementation->Displays.end(); iter2++)
      {      
      pxm->UnRegisterProxy(
        "selection_objects", iter2->second.DisplayProxy->GetSelfIDAsString());
      }
    }
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
void pqSelectionManager::createDisplayProxies(vtkSMProxy* input)
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();

  int numRenModules = model->getNumberOfRenderModules();
  for (int i=0; i<numRenModules; i++)
    {
    pqRenderModule* rm = model->getRenderModule(i);
    vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
    if (rmp->GetConnectionID() == input->GetConnectionID())
      {
      this->getDisplayProxy(rm, input);
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMDisplayProxy* pqSelectionManager::getDisplayProxy(pqRenderModule* rm,
                                                       vtkSMProxy* input)
{
  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
  if (!rmp)
    {
    qDebug("No render module proxy specified. Cannot return display object");
    return 0;
    }

  vtkTypeUInt32 id = rmp->GetSelfID().ID;

  vtkSMDataObjectDisplayProxy* displayProxy = 0;
  pqSelectionManagerImplementation::DisplaysType::iterator iter = 
    this->Implementation->Displays.find(id);
  if (iter != this->Implementation->Displays.end())
    {
    displayProxy = iter->second.DisplayProxy;
    }

  if (!displayProxy)
    {
    displayProxy = vtkSMDataObjectDisplayProxy::SafeDownCast(
      rmp->CreateDisplayProxy());

    this->Implementation->Displays[id].DisplayProxy =
      displayProxy;

    vtkSMProxyManager* pm = vtkSMObject::GetProxyManager();
    pm->RegisterProxy(
      "selection_objects", displayProxy->GetSelfIDAsString(), displayProxy);
    //pm->RegisterProxy(
    //"displays", displayProxy->GetSelfIDAsString(), displayProxy);

    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      displayProxy->GetProperty("Input"));
    pp->AddProxy(input);

    // Add the display proxy to render module.
    pp = vtkSMProxyProperty::SafeDownCast(rmp->GetProperty("Displays"));
    pp->AddProxy(displayProxy);
    rmp->UpdateVTKObjects();

    // Set representation to wireframe
    displayProxy->SetRepresentationCM(1);

    // Do not color by array
    displayProxy->SetScalarVisibilityCM(0);

    // Set color to purple
    displayProxy->SetColorCM(1, 0, 1);

    // Set line thickness to 2
    displayProxy->SetLineWidthCM(2.0);


    vtkSMIntVectorProperty* pk = vtkSMIntVectorProperty::SafeDownCast(
      displayProxy->GetProperty("Pickable"));
    pk->SetElements1(0); // do not pick self

    displayProxy->UpdateVTKObjects();

    displayProxy->Delete();
    }

  return displayProxy;

}

//-----------------------------------------------------------------------------
void pqSelectionManager::setActiveRenderModule(pqRenderModule* rm)
{
  if (!rm)
    {
    return;
    }
  
  // make sure the active render module has the right interactor
  if (this->Mode == SELECT)
    {
    // the previous one should switch to the previous interactor, only
    // the current one uses the select interactor
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

  // If no selection proxy for the current connection exists,
  // create one
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  vtkIdType connId = rmp->GetConnectionID();
  vtkSMProxy* selectorProxy = 0;
  pqSelectionManagerImplementation::ServerSelectionsType::iterator iter = 
    this->Implementation->ServerSelections.find(connId);
  if (iter != this->Implementation->ServerSelections.end())
    {
    selectorProxy = iter->second.SelectorProxy;
    }

  if (!selectorProxy)
    {
    // This is the object that performs selection
    selectorProxy = pxm->NewProxy("selection_helpers", "VolumeSelector");
    if (!selectorProxy)
      {
      qDebug("An appropriate selection proxy could not be created. "
             "No selection is possible");
      return;
      }
    selectorProxy->SetConnectionID(connId);
    selectorProxy->SetServers(vtkProcessModule::DATA_SERVER);
    pxm->RegisterProxy(
      "selection_objects", selectorProxy->GetSelfIDAsString(), selectorProxy);
    this->Implementation->ServerSelections[connId].SelectorProxy = 
      selectorProxy;

    vtkSMProxy* geometrySelectionProxy = 
      pxm->NewProxy("selection_helpers", "Selection");
    geometrySelectionProxy->SetConnectionID(connId);
    geometrySelectionProxy->SetServers(vtkProcessModule::DATA_SERVER);
    geometrySelectionProxy->UpdateVTKObjects();

    // This is the object that stores the result of selection
    vtkSMProxyProperty* selectionProperty = 
      vtkSMProxyProperty::SafeDownCast(selectorProxy->GetProperty("Selection"));
    selectionProperty->AddProxy(geometrySelectionProxy);
    this->Implementation->ServerSelections[connId].GeometrySelectionProxy = 
      geometrySelectionProxy;

    selectorProxy->UpdateVTKObjects();
    selectorProxy->Delete();

    vtkSMSourceProxy* selectionGeomProxy = 
      vtkSMSourceProxy::SafeDownCast(
        pxm->NewProxy("sources", "SelectionSource"));
    selectionGeomProxy->SetConnectionID(connId);
    selectionGeomProxy->SetServers(vtkProcessModule::DATA_SERVER);

    selectionProperty = vtkSMProxyProperty::SafeDownCast(
      selectionGeomProxy->GetProperty("Selection"));
    selectionProperty->AddProxy(geometrySelectionProxy);
    selectionGeomProxy->UpdateVTKObjects();

    this->Implementation->ServerSelections[connId].GeometrySource = 
      selectionGeomProxy;

    selectionGeomProxy->Delete();
    geometrySelectionProxy->Delete();

    vtkSMProxy* sourceSelectionProxy = 
      pxm->NewProxy("selection_helpers", "Selection");
    sourceSelectionProxy->SetConnectionID(connId);
    sourceSelectionProxy->SetServers(vtkProcessModule::DATA_SERVER);
    sourceSelectionProxy->UpdateVTKObjects();

    this->Implementation->ServerSelections[connId].SourceSelectionProxy = 
      sourceSelectionProxy;
    
    sourceSelectionProxy->Delete();
    }

}

//-----------------------------------------------------------------------------
void pqSelectionManager::clearSelection()
{
  pqSelectionManagerImplementation::ServerSelectionsType::iterator iter =
    this->Implementation->ServerSelections.begin();
  for(; iter != this->Implementation->ServerSelections.end(); iter++)
    {
    vtkSMProxy* selectorProxy = iter->second.SelectorProxy;
    selectorProxy->InvokeCommand("Initialize");
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
      this->updateSelection(eventpos, this->Implementation->RenderModule);
      break;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::selectInFrustrum(int* eventpos, pqRenderModule* rm)
{
  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();

  vtkRenderWindowInteractor* rwi = rmp->GetInteractor();
  vtkRenderer *renderer = rwi->FindPokedRenderer(eventpos[0], eventpos[1]);

  // First, compute the frustrum from the given rectangle
  double x0 = (double)this->Implementation->Xs;
  double y0 = (double)this->Implementation->Ys;
  double x1 = (double)this->Implementation->Xe;
  double y1 = (double)this->Implementation->Ye;

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

  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > displays;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > geomFilters;
  vtkstd::vector<vtkSmartPointer<vtkSMProxy> > sources;

  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();

  int numProps = idInfo->GetLength();
  for (int i = 0; i < numProps; i++)
    {
    vtkClientServerID id = idInfo->GetID(i);
    // get the proxies corresponding to the picked display
    // proxy
    vtkSMProxy* objProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::INPUT);
    vtkSMProxy* dispProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::DISPLAY);
    vtkSMProxy* geomProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::GEOMETRY);
    if (!dispProxy)
      {
      continue;
      }
    if (!geomProxy)
      {
      continue;
      }
    // Make sure the proxy is visible and pickable
    vtkSMIntVectorProperty* enabled = vtkSMIntVectorProperty::SafeDownCast(
      dispProxy->GetProperty("Pickable"));
    if (enabled)
      {
      if (!enabled->GetElement(0))
        {
        continue;
        }
      }
    enabled = vtkSMIntVectorProperty::SafeDownCast(
      dispProxy->GetProperty("Visibility"));
    if (enabled)
      {
      if (!enabled->GetElement(0))
        {
        continue;
        }
      }
    // selected
    pqPipelineSource* pqSource = model->getPQSource(objProxy);
    if (pqSource)
      {
      displays.push_back(dispProxy);
      geomFilters.push_back(geomProxy);
      sources.push_back(objProxy);
      }
    }

  // now select cells
  vtkSMProxy* selectorProxy = 
    this->Implementation->ServerSelections[rmp->GetConnectionID()].SelectorProxy;
  if (!selectorProxy)
    {
    qDebug("No selection proxy was created on the data server. "
           "Cannot select cells");
    return;
    }

  vtkSMDoubleVectorProperty* cf = vtkSMDoubleVectorProperty::SafeDownCast(
    selectorProxy->GetProperty("CreateFrustum"));
  cf->SetElements(&this->Implementation->Verts[0]);

  selectorProxy->UpdateVTKObjects();
  selectorProxy->InvokeCommand("Initialize");

  vtkSMProxyProperty* dataSets = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("DataSets"));
  vtkSMProxyProperty* props = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("Props"));
  vtkSMProxyProperty* originalSources = vtkSMProxyProperty::SafeDownCast(
    selectorProxy->GetProperty("OriginalSources"));

  unsigned int numProxies = geomFilters.size();
  for (unsigned int i = 0; i < numProxies; i++)
    {
    vtkSMDataObjectDisplayProxy* dp = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(displays[i]);
    if (dp)
      {
      dataSets->AddProxy(geomFilters[i]);
      props->AddProxy(dp->GetActorProxy());
      originalSources->AddProxy(sources[i]);
      }
    }
  idInfo->Delete();

  selectorProxy->UpdateVTKObjects();

  selectorProxy->InvokeCommand("Select");

  // Cleanup
  dataSets->RemoveAllProxies();
  props->RemoveAllProxies();
  originalSources->RemoveAllProxies();
  selectorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::updateSelection(int* eventpos, pqRenderModule* rm)
{
  // Set the selection rectangle
  this->Implementation->Xe = eventpos[0];
  this->Implementation->Ye = eventpos[1];

  int x[2], y[2];
  x[0] = 
    (this->Implementation->Xs < this->Implementation->Xe) ? 
    this->Implementation->Xs : 
    this->Implementation->Xe;
  y[0] = 
    (this->Implementation->Ys < this->Implementation->Ye) ? 
    this->Implementation->Ys : 
    this->Implementation->Ye;
  x[1] = 
    (this->Implementation->Xs > this->Implementation->Xe) ?
    this->Implementation->Xs : 
    this->Implementation->Xe;
  y[1] = 
    (this->Implementation->Ys > this->Implementation->Ye) ? 
    this->Implementation->Ys : 
    this->Implementation->Ye;
  this->Implementation->Xs = x[0];
  this->Implementation->Xe = x[1];
  this->Implementation->Ys = y[0];
  this->Implementation->Ye = y[1];
  
  // Currently, we only support selection with a frustrum. Surface
  // selection is in the works
  this->selectInFrustrum(eventpos, rm);

  //this->sendSelection(selection, geometrySelectionProxy);

  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
  vtkIdType connId = rmp->GetConnectionID();

  vtkSMProxy* geometrySelectionProxy = 
    this->Implementation->ServerSelections[connId].GeometrySelectionProxy;
  //gather the selection results
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  vtkPVSelectionInformation* selInfo = vtkPVSelectionInformation::New();
  processModule->GatherInformation(
    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
    vtkProcessModule::RENDER_SERVER, 
    selInfo, 
    geometrySelectionProxy->GetID(0)
    );
  vtkSelection* sel = selInfo->GetSelection();
  vtkstd::list<int> ids;
  this->Implementation->traverseSelection(sel, ids);

  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqServerManagerSelectionModel* selectionModel =
    pqApplicationCore::instance()->getSelectionModel();

  pqServerManagerSelection selection;

  // push the selection to the selection model
  vtkstd::list<int>::iterator iter = ids.begin();
  for(; iter != ids.end(); iter++)
    {
    vtkClientServerID id;
    id.ID = *iter;
    vtkSMProxy* objProxy = 
      rmp->GetProxyFromPropID(&id, vtkSMRenderModuleProxy::INPUT);
    // selected
    pqPipelineSource* pqSource = model->getPQSource(objProxy);
    if (pqSource)
      {
      selection.push_back(pqSource);
      }
    }

  // render the selection
  vtkSMSourceProxy* geomProxy = 
    this->Implementation->ServerSelections[connId].GeometrySource;
  geomProxy->MarkModified(0);
  
  selInfo->Delete();
  selectionModel->select(
    selection, pqServerManagerSelectionModel::ClearAndSelect);

  this->createDisplayProxies(geomProxy);

  int numRenModules = model->getNumberOfRenderModules();
  for (int i=0; i<numRenModules; i++)
    {
    pqRenderModule* renModule = model->getRenderModule(i);
    vtkSMRenderModuleProxy* renModuleProxy = renModule->getRenderModuleProxy();
    if (connId == geomProxy->GetConnectionID())
      {
      renModule->render();
      }
    }

  vtkSMProxy* sourceSelection =
    this->Implementation->ServerSelections[connId].SourceSelectionProxy;
  vtkSMProxy* geomSelection =
    this->Implementation->ServerSelections[connId].GeometrySelectionProxy;

  vtkClientServerStream stream;
  vtkClientServerID converterID =
    processModule->NewStreamObject("vtkSelectionConverter", stream);
  stream << vtkClientServerStream::Invoke
         << converterID 
         << "Convert" 
         << geomSelection->GetID(0) 
         << sourceSelection->GetID(0)
         << vtkClientServerStream::End;
  processModule->DeleteStreamObject(converterID, stream);
  processModule->SendStream(sourceSelection->GetConnectionID(), 
                            sourceSelection->GetServers(), 
                            stream);

}

//-----------------------------------------------------------------------------
void pqSelectionManager::sendSelection(vtkSelection* sel, vtkSMProxy* proxy)
{
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  ostrstream res;
  vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, sel);
  res << ends;
  vtkClientServerStream stream;
  vtkClientServerID parserID =
    processModule->NewStreamObject("vtkSelectionSerializer", stream);
  stream << vtkClientServerStream::Invoke
         << parserID << "Parse" << res.str() << proxy->GetID(0)
         << vtkClientServerStream::End;
  processModule->DeleteStreamObject(parserID, stream);

  processModule->SendStream(proxy->GetConnectionID(), 
                            proxy->GetServers(), 
                            stream);
  delete[] res.str();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::proxyUnRegistered(
  QString group, QString name, vtkSMProxy*)
{
  if (group == "selection_objects")
    {
    pqSelectionManagerImplementation::ServerSelectionsType::iterator iter =
      this->Implementation->ServerSelections.begin();
    for(; iter != this->Implementation->ServerSelections.end(); iter++)
      {
      vtkSMProxy* proxy = iter->second.SelectorProxy;
      if (name == proxy->GetSelfIDAsString())
        {
        this->Implementation->ServerSelections.erase(iter);
        return;
        }
      }

    pqSelectionManagerImplementation::DisplaysType::iterator iter2 =
      this->Implementation->Displays.begin();
    for(; iter2 != this->Implementation->Displays.end(); iter2++)
      {      
      vtkSMProxy* proxy = iter2->second.DisplayProxy;
      if (name == proxy->GetSelfIDAsString())
        {
        this->Implementation->Displays.erase(iter2);
        return;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::sourceRemoved(pqPipelineSource* vtkNotUsed(source))
{
}
