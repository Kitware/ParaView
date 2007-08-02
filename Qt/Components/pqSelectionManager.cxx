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

#include "vtkCollection.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"

//-----------------------------------------------------------------------------
class pqSelectionManagerImplementation
{
public:
  pqSelectionManagerImplementation()
    {
    }

  ~pqSelectionManagerImplementation() 
    {
    this->clearSelection();
    }

  void clearSelection()
    {
    if (this->SelectedPort)
      {
      vtkSMSourceProxy* src = vtkSMSourceProxy::SafeDownCast(
        this->SelectedPort->getSource()->getProxy());
      src->CleanSelectionInputs(this->SelectedPort->getPortNumber());
      }
    this->SelectedPort = 0;
    this->GlobalIDSelectionSource = 0;
    }

  vtkSMProxy* getSelectionSourceProxy()
    {
    if (this->SelectedPort)
      {
      vtkSMSourceProxy* src = vtkSMSourceProxy::SafeDownCast(
        this->SelectedPort->getSource()->getProxy());
      return src->GetSelectionInput(this->SelectedPort->getPortNumber());
      }
    return 0;
    }

  QPointer<pqOutputPort> SelectedPort;
  QPointer<pqView> ActiveView;

  vtkSmartPointer<vtkSMProxy> GlobalIDSelectionSource;
};

//-----------------------------------------------------------------------------
pqSelectionManager::pqSelectionManager(QObject* _parent/*=null*/) :
  QObject(_parent)
{
  this->Implementation = new pqSelectionManagerImplementation;
  pqApplicationCore* core = pqApplicationCore::instance();

  pqServerManagerModel* model = core->getServerManagerModel();
  // We need to clear selection when a source is removed. The source
  // that was deleted might have been selected.
  QObject::connect(
    model, SIGNAL(itemRemoved(pqServerManagerModelItem*)),
    this,  SLOT(clearSelection()));

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
void pqSelectionManager::setActiveView(pqView* view)
{
  if (this->Implementation->ActiveView)
    {
    QObject::disconnect(this->Implementation->ActiveView, 0, this, 0);
    }
  this->Implementation->ActiveView = view;
  if (view)
    {
    QObject::connect(view, SIGNAL(selected(pqOutputPort*)), 
      this, SLOT(onSelected(pqOutputPort*)));
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::clearSelection()
{
  // Actual cleaning is done by internal method,
  // this method additionally triggers renders and fires selection changed
  // event.
  pqOutputPort* opport = this->getSelectedPort();
  this->Implementation->clearSelection();
  if (opport)
    {
    opport->renderAllViews(false);
    }

  emit this->selectionChanged(this);
}

//-----------------------------------------------------------------------------
pqOutputPort* pqSelectionManager::getSelectedPort() const
{
  return this->Implementation->SelectedPort;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::sourceRemoved(pqPipelineSource* vtkNotUsed(source))
{
  this->clearSelection();
}

//-----------------------------------------------------------------------------
void pqSelectionManager::onSelected(pqOutputPort* selectedPort)
{
  // The active view is reporting that it made a selection, we update our state.
  if (this->Implementation->SelectedPort != selectedPort)
    {
    // Clear previous selection.
    this->clearSelection();
    }

  this->Implementation->SelectedPort = selectedPort;
  if (selectedPort)
    {
    selectedPort->renderAllViews(false);
    }

  emit this->selectionChanged(this);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSelectionManager::getSelectedIndicesWithProcessIDs() const
{
  vtkSMProxy* selectionSource = this->Implementation->getSelectionSourceProxy();
  if (!selectionSource)
    {
    return QList<QVariant>();
    }

  return pqSMAdaptor::getMultipleElementProperty(
    selectionSource->GetProperty("IDs"));
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

