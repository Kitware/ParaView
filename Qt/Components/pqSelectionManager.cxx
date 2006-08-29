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
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

#include <QtDebug>

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMGenericViewDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSelectionProxy.h"
#include "vtkSMSourceProxy.h"

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
  // These proxies are created on per-connection basis.
  // SelectionDataSource is created once for every connection, while
  // SourceSelectionProxy is created new for every new selection.
  vtkSmartPointer<vtkSMSelectionProxy> SourceSelectionProxy;
  vtkSmartPointer<vtkSMSourceProxy> SelectionDataSource;
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
    Xs(0), Ys(0), Xe(0), Ye(0),
    InSetActiveSelection(false),
    InUpdateSelections(false)
    {
    this->VTKConnect = vtkEventQtSlotConnect::New(); 
    }

  ~pqSelectionManagerImplementation() 
    {
    this->VTKConnect->Delete();

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

  struct ClientSideDisplay
  {
    ClientSideDisplay(
      vtkSMProxy* source, vtkSMProxy* extractor, vtkSMProxy* display) :
      SourceProxy(source), Extractor(extractor), Display(display)
      {
      }
    ClientSideDisplay()
      {
      }

    vtkSMProxy* SourceProxy;
    vtkSmartPointer<vtkSMProxy> Extractor;
    vtkSmartPointer<vtkSMGenericViewDisplayProxy> Display;
  };

  typedef vtkstd::vector<ClientSideDisplay > ClientSideDisplaysType;
  ClientSideDisplaysType ClientSideDisplays;
  bool InSetActiveSelection;
  bool InUpdateSelections;
  vtkEventQtSlotConnect* VTKConnect;
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

  pqApplicationCore* core = pqApplicationCore::instance();

  pqServerManagerModel* model = core->getServerManagerModel();
  // We need to clear selection when a source is removed. The source
  // that was deleted might have been selected.
  QObject::connect(model, 
                   SIGNAL(sourceRemoved(pqPipelineSource*)),
                   this, 
                   SLOT(sourceRemoved(pqPipelineSource*)));

  // Cleanup when a selection helper is unregistered.
  pqServerManagerObserver* observer = core->getServerManagerObserver();
  QObject::connect(
    observer, SIGNAL(proxyUnRegistered(QString, QString, vtkSMProxy*)),
    this, SLOT(proxyUnRegistered(QString, QString, vtkSMProxy*)));
  QObject::connect(
    observer, SIGNAL(proxyRegistered(QString, QString, vtkSMProxy*)),
    this, SLOT(proxyRegistered(QString, QString, vtkSMProxy*)));

  // When server disconnects we must clean up the selection proxies
  // explicitly. This is needed since the internal selection proxies
  // aren't registered with the proxy manager.
  QObject::connect(
    model, SIGNAL(aboutToRemoveServer(pqServer*)),
    this, SLOT(cleanSelections()));
  QObject::connect(
    model, SIGNAL(serverRemoved(pqServer*)),
    this, SLOT(cleanSelections()));
}

//-----------------------------------------------------------------------------
pqSelectionManager::~pqSelectionManager()
{
  this->cleanSelections();
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
void pqSelectionManager::createDisplayProxies(vtkSMProxy* input, bool show/*=true*/)
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
      vtkSMDisplayProxy* disp = this->getDisplayProxy(rm, input, show);
      if (disp)
        {
        pqSMAdaptor::setElementProperty(disp->GetProperty("Visibility"), (show? 1: 0));
        disp->UpdateVTKObjects();
        }
      // push an EventuallyRender() request.
      rm->render();
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMDisplayProxy* pqSelectionManager::getDisplayProxy(pqRenderModule* rm,
                                                       vtkSMProxy* input,
                                                       bool create_new/*=true*/)
{
  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
  if (!rmp)
    {
    qDebug("No render module proxy specified. Cannot return display object");
    return 0;
    }

  vtkTypeUInt32 id = rmp->GetSelfID().ID;

  // These displays are not registered. We treat them a GUI only.
  // Python can surely change the "active_selection" which will
  // get shown correctly, but it cannot affect the display for
  // the selection.
  vtkSMDataObjectDisplayProxy* displayProxy = 0;
  pqSelectionManagerImplementation::DisplaysType::iterator iter = 
    this->Implementation->Displays.find(id);
  if (iter != this->Implementation->Displays.end())
    {
    displayProxy = iter->second.DisplayProxy;
    }

  if (!displayProxy && create_new)
    {
    displayProxy = vtkSMDataObjectDisplayProxy::SafeDownCast(
      rmp->CreateDisplayProxy());

    this->Implementation->Displays[id].DisplayProxy = displayProxy;
    displayProxy->Delete();

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
      QObject::disconnect(this->Implementation->RenderModule, 0, this, 0);
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

  QObject::connect(this->Implementation->RenderModule, SIGNAL(endRender()),
    this, SLOT(updateSelections()));

  // If no selection proxy for the current connection exists,
  // create one
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  vtkIdType connId = rmp->GetConnectionID();

  pqSelectionManagerImplementation::ServerSelectionsType::iterator iter = 
    this->Implementation->ServerSelections.find(connId);
  if (iter == this->Implementation->ServerSelections.end())
    {
    // Create a selection source for this connection.
    vtkSMSourceProxy* selectionSource = 
      vtkSMSourceProxy::SafeDownCast(
        pxm->NewProxy("selection_helpers", "SelectionSource"));
    selectionSource->SetConnectionID(connId);
    selectionSource->SetServers(vtkProcessModule::DATA_SERVER);
    selectionSource->UpdateVTKObjects();
    this->Implementation->ServerSelections[connId].SelectionDataSource = 
      selectionSource;
    selectionSource->Delete();
    }

}

//-----------------------------------------------------------------------------
void pqSelectionManager::clearSelection()
{
  pqSelectionManagerImplementation::ServerSelectionsType::iterator iter =
    this->Implementation->ServerSelections.begin();
  for(; iter != this->Implementation->ServerSelections.end(); iter++)
    {
    this->setActiveSelection(iter->first, NULL);
    }
  this->clearClientDisplays();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::cleanSelections()
{
  this->clearSelection();
  this->Implementation->ServerSelections.clear();
  this->Implementation->Displays.clear();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::setActiveSelection(
  vtkIdType cid, vtkSMSelectionProxy* selectionProxy)
{
  if (this->Implementation->InSetActiveSelection)
    {
    return;
    }

  this->Implementation->InSetActiveSelection = true;
  pqSelectionManagerImplementation::ServerSelectionsType::iterator iter =
    this->Implementation->ServerSelections.find(cid);
  if (iter == this->Implementation->ServerSelections.end())
    {
    return;
    }

  vtkSMProxy* old_selection = iter->second.SourceSelectionProxy;
  vtkSMProxy* selectionSource = iter->second.SelectionDataSource;
  if (selectionSource)
    {
    pqSMAdaptor::setProxyProperty(selectionSource->GetProperty("Selection"),
      selectionProxy);
    selectionSource->UpdateVTKObjects();
    }
  iter->second.SourceSelectionProxy = selectionProxy;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (old_selection)
    {
    // Unregister old selection.
    pxm->UnRegisterProxy("selection_objects", "active_selection", old_selection);
    }

  if (selectionProxy)
    {
    pxm->RegisterProxy("selection_objects", "active_selection", selectionProxy);
    }
  if (selectionProxy && selectionSource)
    {
    this->createDisplayProxies(selectionSource, true);
    }
  else
    {
    this->createDisplayProxies(selectionSource, false);
    }
  this->Implementation->InSetActiveSelection = false;
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
void pqSelectionManager::updateSelection(int* eventpos, pqRenderModule* rm)
{
  // Set the selection rectangle
  this->Implementation->Xe = eventpos[0];
  this->Implementation->Ye = eventpos[1];

 
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
  vtkIdType connId = rmp->GetConnectionID();

  vtkSMSelectionProxy* sourceSelection = vtkSMSelectionProxy::SafeDownCast(
      pxm->NewProxy("selection_helpers", "SelectionProxy"));
  sourceSelection->SetServers(vtkProcessModule::DATA_SERVER);
  sourceSelection->SetConnectionID(connId);

  pqSMAdaptor::setProxyProperty(sourceSelection->GetProperty("RenderModule"),
    rmp);
  pqSMAdaptor::setMultipleElementProperty(sourceSelection->GetProperty("Selection"),
    0, this->Implementation->Xs);
  pqSMAdaptor::setMultipleElementProperty(sourceSelection->GetProperty("Selection"),
    1, this->Implementation->Ys);
  pqSMAdaptor::setMultipleElementProperty(sourceSelection->GetProperty("Selection"),
    2, this->Implementation->Xe);
  pqSMAdaptor::setMultipleElementProperty(sourceSelection->GetProperty("Selection"),
    3, this->Implementation->Ye);
  sourceSelection->UpdateVTKObjects();
  sourceSelection->UpdateCameraPropertiesFromRenderModule();
  sourceSelection->UpdateVTKObjects();

  // We deliberately don't call UpdateSelection() explicitly here. We let the mechanism that
  // manages the UpdateSelection() during undo/redo/load state manage it.
  // At the end of any render, we inspect all Active Selection. If any is not up-to-date,
  // we call UpdateSelection() on it and do the tasks necessary afer a selection change 
  // (such as update of client displays etc).

  pqApplicationCore* core = pqApplicationCore::instance();
  core->getUndoStack()->BeginUndoSet("Selection");
  this->setActiveSelection(connId, sourceSelection); 
  core->getUndoStack()->EndUndoSet();
  sourceSelection->Delete();

  // Let the world know that the user has marked a selection.
  emit this->selectionMarked();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::clearClientDisplays()
{
  pqSelectionManagerImplementation::ClientSideDisplaysType::iterator iter =
    this->Implementation->ClientSideDisplays.begin();

  for(; iter!= this->Implementation->ClientSideDisplays.end(); iter++)
    {
    vtkSMProxy* extractor = iter->Extractor;
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      extractor->GetProperty("Input"));
    if (pp)
      {
      pp->RemoveAllProxies();
      }
    extractor->UpdateVTKObjects();
    }
  this->Implementation->ClientSideDisplays.clear();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::createNewClientDisplays(
  pqServerManagerSelection& selection)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  pqServerManagerSelection::iterator iter = selection.begin();
  for (; iter != selection.end(); iter++)
    {
    pqServerManagerModelItem* item = *iter;
    vtkSMProxy* source = static_cast<pqPipelineSource*>(item)->getProxy();

    vtkIdType connId = source->GetConnectionID();

    vtkSMProxy* selectionSource =
      this->Implementation->ServerSelections[connId].SelectionDataSource;

    vtkSMProxy* extractor = 
      pxm->NewProxy("selection_helpers", "ExtractSelectionBlock");
    extractor->SetConnectionID(connId);
    extractor->SetServers(source->GetServers());
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      extractor->GetProperty("Input"));
    if (pp)
      {
      pp->AddProxy(selectionSource);
      }

    vtkSMIntVectorProperty* sid = vtkSMIntVectorProperty::SafeDownCast(
      extractor->GetProperty("SourceID"));
    sid->SetElements1(source->GetID(0).ID); 
    
    extractor->UpdateVTKObjects();

    vtkSMGenericViewDisplayProxy* display = 
      vtkSMGenericViewDisplayProxy::SafeDownCast(
        pxm->NewProxy("viewdisplays", "GenericViewDisplay"));
    display->SetConnectionID(connId);
    display->SetServers(source->GetServers());
    pp = vtkSMProxyProperty::SafeDownCast(display->GetProperty("Input"));
    if (pp)
      {
      pp->AddProxy(extractor);
      }

    vtkSMIntVectorProperty* dataType = vtkSMIntVectorProperty::SafeDownCast(
      display->GetProperty("OutputDataType"));
    dataType->SetElements1(VTK_UNSTRUCTURED_GRID);

    display->UpdateVTKObjects();
    
    display->Update();

    this->Implementation->ClientSideDisplays.push_back( 
      pqSelectionManagerImplementation::ClientSideDisplay(
        source, extractor, display));
    extractor->Delete();
    display->Delete();
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::proxyRegistered(
  QString group, QString name, vtkSMProxy* proxy)
{
  // Notice registrations of (selection_objects, active_selection). 
  // If any selection gets registered under that, it becomes the
  // new active selection.
  if (group == "selection_objects" && name == "active_selection")
    {
    // We have a new active selection.
    this->activeSelectionRegistered(vtkSMSelectionProxy::SafeDownCast(proxy));
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::proxyUnRegistered(
  QString group, QString name, vtkSMProxy* proxy)
{
  if (group == "selection_objects")
    {
    /*
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
      */
    }
  if (group == "selection_objects" && name == "active_selection")
    {
    vtkSMSelectionProxy* sel = vtkSMSelectionProxy::SafeDownCast(proxy);
    if (this->Implementation->ServerSelections[proxy->GetConnectionID()].SourceSelectionProxy.GetPointer()
      == sel)
      {
      this->Implementation->ServerSelections[
        proxy->GetConnectionID()].SourceSelectionProxy = 0;
      this->setActiveSelection(sel->GetConnectionID(), 0);

      // HACK: this is a temporary hack to break cycles
      pqSMAdaptor::setProxyProperty(sel->GetProperty("RenderModule"), 0);
      sel->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::activeSelectionRegistered(vtkSMSelectionProxy* sel)
{
  // Unhook all previous handlers.
  this->Implementation->VTKConnect->Disconnect();

  // This buffer method is necessary since when the selection proxy is registered
  // it might not be actually created (which is the case while loading state).
  // If the proxy is not created, we cannot connect displays to it, (as
  // setActiveSelection would normally do). In that case we connect slots 
  // to update the selection after the proxy gets created (i.e. UpdateVTKObjects 
  // is called the first time).
  if (sel->GetNumberOfIDs() > 0)
    {
    this->setActiveSelection(sel->GetConnectionID(), sel);
    return;
    }

  // Hook up event listerners for vtkCommand::UpdateEvent
  this->Implementation->VTKConnect->Connect(
    sel, vtkCommand::UpdateEvent, 
    this, SLOT(onSelectionUpdateVTKObjects(vtkObject*)), 0,
    Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
// This is called after an active selection's VTK objects get created.
void pqSelectionManager::onSelectionUpdateVTKObjects(vtkObject* obj)
{
  vtkSMSelectionProxy* sel = vtkSMSelectionProxy::SafeDownCast(obj);
  if (sel)
    {
    this->activeSelectionRegistered(sel);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::sourceRemoved(pqPipelineSource* vtkNotUsed(source))
{
  this->clearSelection();
}

//-----------------------------------------------------------------------------
// This slot is called on endRender() on the active render module.
// In here, we update any out-of-date selections. Out-of-date selections
// are typically a result of state loading or undo/redo operation.
void pqSelectionManager::updateSelections()
{
 if (this->Implementation->InUpdateSelections)
   {
   return;
   }
 this->Implementation->InUpdateSelections  = true;

  pqSelectionManagerImplementation::ServerSelectionsType::iterator iter =
    this->Implementation->ServerSelections.begin();
  for(; iter != this->Implementation->ServerSelections.end(); iter++)
    {
    if (iter->second.SourceSelectionProxy)
      {
      vtkSMSelectionProxy* sel = vtkSMSelectionProxy::SafeDownCast(
        iter->second.SourceSelectionProxy);
      if (!sel->GetSelectionUpToDate())
        {
        // this call leads to further renders, 
        // hence the InUpdateSelections guards.
        sel->UpdateSelection();

        this->selectionChanged(iter->first);
        }
      }
    }

  this->Implementation->InUpdateSelections  = false;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::selectionChanged(vtkIdType cid)
{
  pqSelectionManagerImplementation::ServerSelectionsType::iterator iter =
    this->Implementation->ServerSelections.find(cid);
  if (iter == this->Implementation->ServerSelections.end())
    {
    return;
    }

  vtkSMSelectionProxy* sourceSelection = iter->second.SourceSelectionProxy;
  if (!sourceSelection)
    {
    return;
    }

  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();

  // Update the SelectionModel with the selected sources.
  pqServerManagerSelectionModel* selectionModel =
    pqApplicationCore::instance()->getSelectionModel();

  pqServerManagerSelection selection;

  vtkCollection* selectedProxies = vtkCollection::New();
  sourceSelection->GetSelectedSourceProxies(selectedProxies);

  // push the selection to the selection model
  for(int cc=0; cc < selectedProxies->GetNumberOfItems(); cc++)
    {
    vtkSMProxy* objProxy = vtkSMProxy::SafeDownCast(
      selectedProxies->GetItemAsObject(cc));
    if (objProxy)
      {
      // selected
      pqPipelineSource* pqSource = model->getPQSource(objProxy);
      if (pqSource)
        {
        selection.push_back(pqSource);
        }
      }
    }
  selectedProxies->Delete();
  selectionModel->select(
    selection, pqServerManagerSelectionModel::ClearAndSelect);

  this->clearClientDisplays();
  this->createNewClientDisplays(selection);

  emit this->selectionChanged(this);

  // Since selection changed, we need to trigger render to show the selection.
  QList<pqRenderModule*> rms = model->getRenderModules(model->getServer(cid));
  foreach(pqRenderModule* rm, rms)
    {
    rm->render();
    }

}

//-----------------------------------------------------------------------------
unsigned int pqSelectionManager::getNumberOfSelectedObjects()
{
  return this->Implementation->ClientSideDisplays.size();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::getSelectedObjects(QList<pqPipelineSource*> &proxies,
  QList<vtkDataObject*> &dataObjects)
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqSelectionManagerImplementation::ClientSideDisplaysType::iterator iter =
    this->Implementation->ClientSideDisplays.begin();

  for(; iter!= this->Implementation->ClientSideDisplays.end(); ++iter)
    {
    pqPipelineSource* source = model->getPQSource(iter->SourceProxy);
    if (source)
      {
      proxies.push_back(source);
      dataObjects.push_back(iter->Display->GetOutput());
      }
    }
}

//-----------------------------------------------------------------------------
int pqSelectionManager::getSelectedObject(
  unsigned int idx, vtkSMProxy*& proxy, vtkDataObject*& dataObject)
{
  proxy = 0;
  dataObject = 0;

  if (idx >= this->getNumberOfSelectedObjects())
    {
    return 0;
    }

  proxy = 
    this->Implementation->ClientSideDisplays[idx].SourceProxy;
  dataObject = 
    this->Implementation->ClientSideDisplays[idx].Display->GetOutput();

  return 1;
}
