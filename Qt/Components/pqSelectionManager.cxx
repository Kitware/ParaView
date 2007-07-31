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

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqRubberBandHelper.h"

#include "vtkCollection.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSelectionHelper.h"

//-----------------------------------------------------------------------------
class pqSelectionManagerImplementation
{
public:
  pqSelectionManagerImplementation() : 
    SelectedOutputPort(0)
    {
      this->ClientSideDisplayer = 0;
      this->SelectedProxy = 0;
      this->SelectionRenderModule = 0;

    }

  ~pqSelectionManagerImplementation() 
    {
    this->clearSelection();
    }

  void clearSelection()
    {
    this->SelectionSource = 0;
    this->GlobalIDSelectionSource = 0;

    if (this->ClientSideDisplayer)
      {
      this->ClientSideDisplayer->Delete();
      this->ClientSideDisplayer = 0;
      }
    this->SelectedProxy = 0;
    }

  vtkSMClientDeliveryRepresentationProxy* ClientSideDisplayer;
  vtkSMProxy* SelectedProxy;
  int SelectedOutputPort;

  vtkSmartPointer<vtkSMProxy> SelectedRepresentation;

  vtkSmartPointer<vtkSMProxy> SelectionSource;
  vtkSmartPointer<vtkSMProxy> GlobalIDSelectionSource;

  pqRenderView* SelectionRenderModule;

};

//-----------------------------------------------------------------------------
pqSelectionManager::pqSelectionManager(QObject* _parent/*=null*/) :
  QObject(_parent)
{
  this->Implementation = new pqSelectionManagerImplementation;
  this->RubberBandHelper = new pqRubberBandHelper;

  QObject::connect(
    this->RubberBandHelper, SIGNAL(selectionFinished()),
    this, SLOT(select()));

  this->Mode = pqRubberBandHelper::INTERACT;

  pqApplicationCore* core = pqApplicationCore::instance();

  pqServerManagerModel* model = core->getServerManagerModel();
  // We need to clear selection when a source is removed. The source
  // that was deleted might have been selected.
  QObject::connect(
    model, SIGNAL(itemRemoved(pqServerManagerModelItem*)),
    this,  SLOT(clearSelection()));
  QObject::connect(
    model, SIGNAL(viewRemoved(pqView*)),
    this, SLOT(viewRemoved(pqView*)));

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
  delete this->RubberBandHelper;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::switchToSelection()
{
  if (this->RubberBandHelper->setRubberBandOn())
    {
    this->Mode = pqRubberBandHelper::SELECT;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::switchToInteraction()
  {
  if (this->RubberBandHelper->setRubberBandOff())
    {
    this->Mode = pqRubberBandHelper::INTERACT;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::setActiveView(pqView* view)
{
  pqRenderView* rm = qobject_cast<pqRenderView*>(view);
  if (!rm)
    {
    return;
    }
  
  // make sure the active render module has the right interactor
  if (this->Mode == pqRubberBandHelper::SELECT)
    {
    // the previous view should revert to the previous interactor,
    this->RubberBandHelper->setRubberBandOff();
    // the current view then starts using the select interactor
    this->RubberBandHelper->setRubberBandOn(rm);
    }

  this->RubberBandHelper->RenderModule = rm;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::clearSelection()
{
  pqOutputPort* opport = this->getSelectedPort();

  if (this->Implementation->SelectedRepresentation.GetPointer())
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->Implementation->SelectedRepresentation->GetProperty("Selection"));
    if (pp)
      {
      pp->RemoveAllProxies();
      this->Implementation->SelectedRepresentation->UpdateVTKObjects();
      }
    this->Implementation->SelectedRepresentation = 0;
    }

  this->Implementation->clearSelection();

  if (opport)
    {
    opport->renderAllViews(false);
    }

  emit this->selectionChanged(this);
}

//-----------------------------------------------------------------------------
void pqSelectionManager::select()
{
  int rectangle[4];

  rectangle[0] = this->RubberBandHelper->Xs;
  rectangle[1] = this->RubberBandHelper->Ys;
  rectangle[2] = this->RubberBandHelper->Xe;
  rectangle[3] = this->RubberBandHelper->Ye;

  emit this->beginNonUndoableChanges();

  this->selectOnSurface(rectangle);

  emit this->endNonUndoableChanges();

  emit this->selectionMarked();
}

//-----------------------------------------------------------------------------
vtkSMClientDeliveryRepresentationProxy* pqSelectionManager::getClientSideDisplayer(
  pqPipelineSource* source) const
{
  if (this->Implementation->SelectedProxy == source->getProxy())
    {
    return this->Implementation->ClientSideDisplayer;
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqOutputPort* pqSelectionManager::getSelectedPort() const
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  if (this->Implementation->SelectedProxy)
    {
    pqPipelineSource* src = model->findItem<pqPipelineSource*>(
      this->Implementation->SelectedProxy);
    return src? src->getOutputPort(this->Implementation->SelectedOutputPort) : 0;
    }
  
  return 0;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::sourceRemoved(pqPipelineSource* vtkNotUsed(source))
{
  this->clearSelection();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::viewRemoved(pqView* vm)
{
  pqRenderView* rm = qobject_cast<pqRenderView*>(vm);
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
void pqSelectionManager::selectOnSurface(int screenRectangle[4])
{
  this->clearSelection();

  pqRenderView* rvm = this->RubberBandHelper->RenderModule;
  vtkSMRenderViewProxy* renderModuleP = rvm->getRenderViewProxy();

  // Make sure the selection rectangle is in the right order.
  int displayRectangle[4];
  pqRubberBandHelper::ReorderBoundingBox(
    screenRectangle, 
    displayRectangle);

  // Perform the selection.
  vtkSmartPointer<vtkCollection> selectedRepresentations = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> surfaceSelections = 
    vtkSmartPointer<vtkCollection>::New();
  if (!renderModuleP->SelectOnSurface(
    displayRectangle[0], displayRectangle[1], displayRectangle[2],
    displayRectangle[3], selectedRepresentations, surfaceSelections, false))
    {
    // nothing selected.
    return;
    }

  //rvm->render();
  if (selectedRepresentations->GetNumberOfItems() <= 0 ||
    surfaceSelections->GetNumberOfItems() <=0)
    {
    return;
    }

  this->Implementation->SelectedRepresentation = 
    vtkSMProxy::SafeDownCast(selectedRepresentations->GetItemAsObject(0));
  this->Implementation->SelectionRenderModule = rvm;
  this->Implementation->SelectedProxy = 
    pqSMAdaptor::getProxyProperty(this->Implementation->
      SelectedRepresentation->GetProperty("Input"));
  this->Implementation->SelectedOutputPort =
    vtkSMInputProperty::SafeDownCast(
      this->Implementation->SelectedRepresentation->GetProperty("Input"))->
    GetOutputPortForConnection(0);

  // The selection source is the source that is set as the "Selection" input on
  // the representation.
  this->Implementation->SelectionSource =
    pqSMAdaptor::getProxyProperty(this->Implementation->
      SelectedRepresentation->GetProperty("Selection"));

  // Obtain a global id selection as well.
  vtkIdType connId = 
    this->Implementation->SelectedProxy->GetConnectionID();
  vtkSelection* volumeSelectionGI = vtkSelection::New();
  vtkSMSelectionHelper::ConvertSurfaceSelectionToGlobalIDVolumeSelection(
    connId,
    vtkSelection::SafeDownCast(surfaceSelections->GetItemAsObject(0)), 
    volumeSelectionGI);

  vtkSMProxy* selectionSourceGI = 
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(connId,
      volumeSelectionGI);
  this->Implementation->GlobalIDSelectionSource = selectionSourceGI;
  selectionSourceGI->Delete();
  volumeSelectionGI->Delete();

  // Update the SelectionModel with the selected sources.
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqServerManagerSelectionModel* selectionModel =
    pqApplicationCore::instance()->getSelectionModel();
  pqPipelineSource* pqSource = model->findItem<pqPipelineSource*>(
    this->Implementation->SelectedProxy);
  selectionModel->setCurrentItem(
    pqSource->getOutputPort(this->Implementation->SelectedOutputPort), 
    pqServerManagerSelectionModel::ClearAndSelect);

  pqOutputPort* opport = this->getSelectedPort();
  if (opport)
    {
    opport->renderAllViews(false);
    }
  emit this->selectionChanged(this);
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

