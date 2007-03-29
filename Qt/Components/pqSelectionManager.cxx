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

#include <QtDebug>
#include <QWidget>
#include <QCursor>

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqRenderViewModule.h"
#include "pqSMAdaptor.h"
#include "pqServerManagerModel.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMGenericViewDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSelectionHelper.h"

// TODO:
// * need to support multiple server connections
// * should the selection displayed in multiple views?
// * selection when there is a render server

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
    Xs(0), Ys(0), Xe(0), Ye(0)
    {
      this->SelectionDisplayer = 0;
      this->SelectionRenderModule = 0;
      this->ClientSideDisplayer = 0;
      this->SelectedProxy = 0;
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
    this->clearSelection();
    }

  void clearSelection()
    {
    this->SelectionSource = 0;
    this->GlobalIDSelectionSource = 0;

    if (this->SelectionDisplayer)
      {
      this->SelectionDisplayer->Delete();
      this->SelectionDisplayer = 0;
      }
    if (this->ClientSideDisplayer)
      {
      this->ClientSideDisplayer->Delete();
      this->ClientSideDisplayer = 0;
      }
    this->SelectionRenderModule = 0;
    this->SelectedProxy = 0;
    }

  // traverse selection and extract prop ids (unique)
  vtkInteractorStyleRubberBandPick* RubberBand;
  vtkInteractorObserver* SavedStyle;
  pqRenderViewModule* RenderModule;
  vtkPQSelectionObserver* SelectionObserver;
  vtkSMDataObjectDisplayProxy* SelectionDisplayer;
  pqRenderViewModule* SelectionRenderModule;
  vtkSMGenericViewDisplayProxy* ClientSideDisplayer;
  vtkSMProxy* SelectedProxy;

  vtkSmartPointer<vtkSMProxy> SelectionSource;
  vtkSmartPointer<vtkSMProxy> GlobalIDSelectionSource;

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

  pqApplicationCore* core = pqApplicationCore::instance();

  pqServerManagerModel* model = core->getServerManagerModel();
  // We need to clear selection when a source is removed. The source
  // that was deleted might have been selected.
  QObject::connect(
    model, SIGNAL(sourceRemoved(pqPipelineSource*)),
    this,  SLOT(clearSelection()));
  QObject::connect(
    model, SIGNAL(viewModuleRemoved(pqGenericViewModule*)),
    this, SLOT(viewModuleRemoved(pqGenericViewModule*)));

  // When server disconnects we must clean up the selection proxies
  // explicitly. This is needed since the internal selection proxies
  // aren't registered with the proxy manager.
  QObject::connect(
    model, SIGNAL(aboutToRemoveServer(pqServer*)),
    this, SLOT(clearSelection()));
  QObject::connect(
    model, SIGNAL(serverRemoved(pqServer*)),
    this, SLOT(clearSelection()));
}

//-----------------------------------------------------------------------------
pqSelectionManager::~pqSelectionManager()
{
  this->clearSelection();
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::switchToSelection()
{
  if (!this->Implementation->RenderModule)
    {
    qDebug("Selection is unavailable without visible data.");
    return;
    }

  if (this->setInteractorStyleToSelect(this->Implementation->RenderModule))
    {
    this->Mode = SELECT;
    // set the selection cursor
    this->Implementation->RenderModule->getWidget()->setCursor(Qt::CrossCursor);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::switchToSelectThrough()
{
  if (!this->Implementation->RenderModule)
    {
    qDebug("Selection is unavailable without visible data.");
    return;
    }

  if (this->setInteractorStyleToSelect(this->Implementation->RenderModule))
    {
    this->Mode = FRUSTUM;
    // set the selection cursor
    this->Implementation->RenderModule->getWidget()->setCursor(Qt::CrossCursor);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::switchToInteraction()
{
  if (!this->Implementation->RenderModule)
    {
    //qDebug("No render module specified. Cannot switch to interaction");
    return;
    }

  if (this->setInteractorStyleToInteract(this->Implementation->RenderModule))
    {
    this->Mode = INTERACT;
    // set the interaction cursor
    this->Implementation->RenderModule->getWidget()->setCursor(QCursor());
    }
}

//-----------------------------------------------------------------------------
int pqSelectionManager::setInteractorStyleToSelect(pqRenderViewModule* rm)
{
  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
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
  this->Implementation->SavedStyle = rwi->GetInteractorStyle();
  this->Implementation->SavedStyle->Register(0);
  rwi->SetInteractorStyle(this->Implementation->RubberBand);
  
  rwi->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->Implementation->SelectionObserver);
  rwi->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->Implementation->SelectionObserver);

  this->Implementation->RubberBand->StartSelect();

  return 1;
}

//-----------------------------------------------------------------------------
int pqSelectionManager::setInteractorStyleToInteract(pqRenderViewModule* rm)
{
  vtkSMRenderModuleProxy* rmp = rm->getRenderModuleProxy();
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
void pqSelectionManager::setActiveView(pqGenericViewModule* view)
{
  pqRenderViewModule* rm = qobject_cast<pqRenderViewModule*>(view);
  if (!rm)
    {
    return;
    }
  
  // make sure the active render module has the right interactor
  if (this->Mode == SELECT || this->Mode == FRUSTUM)
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
}

//-----------------------------------------------------------------------------
void pqSelectionManager::clearSelection()
{
  if (this->Implementation->SelectionDisplayer &&
      this->Implementation->SelectionRenderModule)
    {
    vtkSMRenderModuleProxy* renderModuleP = 
      this->Implementation->SelectionRenderModule->getRenderModuleProxy();
    renderModuleP->RemoveDisplay(this->Implementation->SelectionDisplayer);
    renderModuleP->UpdateVTKObjects();
    }
  this->Implementation->clearSelection();

  emit this->selectionChanged(this);
}

//-----------------------------------------------------------------------------
void pqSelectionManager::processEvents(unsigned long eventId)
{
  if (!this->Implementation->RenderModule)
    {
    //qDebug("Selection is unavailable without visible data.");
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
      if (this->Implementation->Xs < 0) 
        {
        this->Implementation->Xs = 0;
        }
      this->Implementation->Ys = eventpos[1];
      if (this->Implementation->Ys < 0) 
        {
        this->Implementation->Ys = 0;
        }
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      this->Implementation->Xe = eventpos[0];
      if (this->Implementation->Xe < 0) 
        {
        this->Implementation->Xe = 0;
        }
      this->Implementation->Ye = eventpos[1];
      if (this->Implementation->Ye < 0) 
        {
        this->Implementation->Ye = 0;
        }
      this->select();
      break;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::select()
{
  int rectangle[4];

  rectangle[0] = this->Implementation->Xs;
  rectangle[1] = this->Implementation->Ys;
  rectangle[2] = this->Implementation->Xe;
  rectangle[3] = this->Implementation->Ye;

  emit this->beginNonUndoableChanges();

  this->selectOnSurface(rectangle);

  emit this->endNonUndoableChanges();

  emit this->selectionMarked();
}

//-----------------------------------------------------------------------------
vtkSMGenericViewDisplayProxy* pqSelectionManager::getClientSideDisplayer(
  pqPipelineSource* source) const
{
  if (this->Implementation->SelectedProxy == source->getProxy())
    {
    return this->Implementation->ClientSideDisplayer;
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSelectionManager::getSelectedSource() const
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  if (this->Implementation->SelectedProxy)
    {
    return model->getPQSource(this->Implementation->SelectedProxy);
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::sourceRemoved(pqPipelineSource* vtkNotUsed(source))
{
  this->clearSelection();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::viewModuleRemoved(pqGenericViewModule* vm)
{
  pqRenderViewModule* rm = qobject_cast<pqRenderViewModule*>(vm);
  if (!rm)
    {
    return;
    }
  if (rm == this->Implementation->SelectionRenderModule)
    {
    this->clearSelection();
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::createSelectionDisplayer(vtkSMProxy* input)
{
  pqRenderViewModule* rvm = this->Implementation->RenderModule;
  vtkSMRenderModuleProxy* renderModuleP = rvm->getRenderModuleProxy();
  vtkIdType connId = renderModuleP->GetConnectionID();
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  this->Implementation->SelectionRenderModule = rvm;

  // Create a display module
  vtkSMDataObjectDisplayProxy* displayP = 
    vtkSMDataObjectDisplayProxy::SafeDownCast(
      renderModuleP->CreateDisplayProxy());
  displayP->SetConnectionID(connId);
  displayP->SetServers(vtkProcessModule::DATA_SERVER);
  // Connect it to the extract filter
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    displayP->GetProperty("Input"));
  pp->AddProxy(input);
  // Add the display proxy to render module.
  renderModuleP->AddDisplay(displayP);
  // Set representation to wireframe
  displayP->SetRepresentationCM(1);
  // Do not color by array
  displayP->SetScalarVisibilityCM(0);
  // Set color to purple
  displayP->SetColorCM(1, 0, 1);
  // Set line thickness to 2
  displayP->SetLineWidthCM(2.0);
  vtkSMIntVectorProperty* pkPp = vtkSMIntVectorProperty::SafeDownCast(
    displayP->GetProperty("Pickable"));
  pkPp->SetElements1(0); // do not pick self
  displayP->UpdateVTKObjects();

  this->Implementation->SelectionDisplayer = displayP;

  vtkSMGenericViewDisplayProxy* csDisplayer = 
    vtkSMGenericViewDisplayProxy::SafeDownCast(
      pxm->NewProxy("displays", "GenericViewDisplay"));
  csDisplayer->SetConnectionID(connId);
  csDisplayer->SetServers(vtkProcessModule::DATA_SERVER);
  pp = vtkSMProxyProperty::SafeDownCast(csDisplayer->GetProperty("Input"));
  if (pp)
    {
    pp->AddProxy(input);
    }

  pqSMAdaptor::setEnumerationProperty(csDisplayer->GetProperty("ReductionType"),
                                      "UNSTRUCTURED_APPEND");
  csDisplayer->UpdateVTKObjects();
  csDisplayer->Update();
  this->Implementation->ClientSideDisplayer = csDisplayer;
}

//-----------------------------------------------------------------------------
// These should go to a vtkSMSelectionHelper
static void pqSelectionManagerReorderBoundingBox(int src[4], int dest[4])
{
  dest[0] = (src[0] < src[2])? src[0] : src[2];
  dest[1] = (src[1] < src[3])? src[1] : src[3];
  dest[2] = (src[0] < src[2])? src[2] : src[0];
  dest[3] = (src[1] < src[3])? src[3] : src[1];
}


//-----------------------------------------------------------------------------
void pqSelectionManager::selectOnSurface(int screenRectangle[4])
{ 
  // Keep track of the render module that contains the previous
  // selection.
  pqRenderViewModule* prevRvm = 0;
  if (this->Implementation->SelectionRenderModule)
    {
    prevRvm = this->Implementation->SelectionRenderModule;
    }
  this->clearSelection();

  pqRenderViewModule* rvm = this->Implementation->RenderModule;
  vtkSMRenderModuleProxy* renderModuleP = rvm->getRenderModuleProxy();

  // Make sure the selection rectangle is in the right order.
  int displayRectangle[4];
  pqSelectionManagerReorderBoundingBox(screenRectangle, displayRectangle);

  // Perform the selection.
  vtkSmartPointer<vtkCollection> selectedProxies = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selections =
    vtkSmartPointer<vtkCollection>::New();

  vtkSMSelectionHelper::SelectOnSurface(renderModuleP,
                                        displayRectangle,
                                        selectedProxies,
                                        selections);

  if (selectedProxies->GetNumberOfItems() == 0)
    {
    prevRvm->render();
    return;
    }
  // For now, we are using only the first selected object.
  vtkSelection* selectionForOne = vtkSelection::SafeDownCast(
    selections->GetItemAsObject(0));
  vtkSMProxy* objP = vtkSMProxy::SafeDownCast(
    selectedProxies->GetItemAsObject(0));

  vtkIdType connId = renderModuleP->GetConnectionID();

  // Convert to volume cell selection
  vtkSelection* volumeSelection = vtkSelection::New();
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
    connId, selectionForOne, volumeSelection);

  // Now pass the selection ids to a selection source.
  vtkSMProxy* selectionSourceP = 
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(connId,
                                                          volumeSelection);
  this->Implementation->SelectionSource = selectionSourceP;

  // Obtain a global id selection as well.
  vtkSelection* volumeSelectionGI = vtkSelection::New();
  vtkSMSelectionHelper::ConvertSurfaceSelectionToGlobalIDVolumeSelection(
    connId, selectionForOne, volumeSelectionGI);

  vtkSMProxy* selectionSourceGI = 
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(connId,
      volumeSelectionGI);
  this->Implementation->GlobalIDSelectionSource = selectionSourceGI;
  selectionSourceGI->Delete();
  volumeSelectionGI->Delete();

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  // Apply the extraction filter
  // Create ExtractSelection
  vtkSMProxy* extractFilterP =  pxm->NewProxy("filters", "ExtractSelection");
  extractFilterP->SetConnectionID(connId);
  extractFilterP->SetServers(vtkProcessModule::DATA_SERVER);
  // Connect inputs
  vtkSMProxyProperty* inputPp = vtkSMProxyProperty::SafeDownCast(
    extractFilterP->GetProperty("Input"));
  inputPp->AddProxy(objP);
  vtkSMProxyProperty* selectionPp = vtkSMProxyProperty::SafeDownCast(
    extractFilterP->GetProperty("Selection"));
  selectionPp->AddProxy(selectionSourceP);
  extractFilterP->UpdateVTKObjects();

  // Now display it
  this->createSelectionDisplayer(extractFilterP);

  rvm->render();
  if (prevRvm && prevRvm != rvm)
    { 
    // We need to clear the selection from the previous rvm
    // by rendering it
    prevRvm->render();
    }

  // Update the SelectionModel with the selected sources.
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqServerManagerSelectionModel* selectionModel =
    pqApplicationCore::instance()->getSelectionModel();
  pqPipelineSource* pqSource = model->getPQSource(objP);
  selectionModel->setCurrentItem(
    pqSource, pqServerManagerSelectionModel::ClearAndSelect);

  this->Implementation->SelectedProxy = objP;

  // Notify everyone that the selection changed.
  emit this->selectionChanged(this);

  // Cleanup
  selectionSourceP->Delete();
  volumeSelection->Delete();
  extractFilterP->Delete();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSelectionManager::getSelectedIndicesWithProcessIDs() const
{
  if (!this->Implementation->SelectionSource.GetPointer())
    {
    return QList<QVariant>();
    }
  return pqSMAdaptor::getMultipleElementProperty(
    this->Implementation->SelectionSource->GetProperty("IDs"));
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSelectionManager::getSelectedGlobalIDs() const
{
  if (!this->Implementation->GlobalIDSelectionSource.GetPointer())
    {
    return QList<QVariant>();
    }
  QList<QVariant> reply;
  QList<QVariant> values = pqSMAdaptor::getMultipleElementProperty(
    this->Implementation->GlobalIDSelectionSource->GetProperty("IDs"));
  for (int cc=1; cc < values.size(); cc+=2)
    {
    reply.push_back(values[cc]);
    }
  return reply;
}
