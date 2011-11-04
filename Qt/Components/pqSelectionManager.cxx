/*=========================================================================

   Program: ParaView
   Module:    pqSelectionManager.cxx

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

#include "pqSelectionManager.h"

#include <QtDebug>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqTimeKeeper.h"
#include "vtkAlgorithm.h"
#include "vtkCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

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
    this,  SLOT(onItemRemoved(pqServerManagerModelItem*)));

  // When server disconnects we must clean up the selection proxies
  // explicitly. This is needed since the internal selection proxies
  // aren't registered with the proxy manager.
  QObject::connect(
    model, SIGNAL(aboutToRemoveServer(pqServer*)),
    this, SLOT(clearSelection()));
  QObject::connect(
    model, SIGNAL(serverRemoved(pqServer*)),
    this, SLOT(clearSelection()));

  pqApplicationCore::instance()->registerManager("SelectionManager", this);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this, SLOT(setActiveView(pqView*)));
  this->setActiveView(pqActiveObjects::instance().activeView());
}

//-----------------------------------------------------------------------------
pqSelectionManager::~pqSelectionManager()
{
  this->clearSelection();
  delete this->Implementation;
  pqApplicationCore::instance()->unRegisterManager("SelectionManager");
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
      this, SLOT(select(pqOutputPort*)));
    }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::onItemRemoved(pqServerManagerModelItem* item)
{
  if (this->Implementation->SelectedPort && 
    item == this->Implementation->SelectedPort->getSource())
    {
    // clear selection if the selected source is being deleted.
    this->clearSelection();
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
    this->Implementation->SelectedPort = 0;
    }

  emit this->selectionChanged(static_cast<pqOutputPort*>(0));
}

//-----------------------------------------------------------------------------
pqOutputPort* pqSelectionManager::getSelectedPort() const
{
  return this->Implementation->SelectedPort;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::select(pqOutputPort* selectedPort)
{
  // The active view is reporting that it made a selection, we update our state.
  if (this->Implementation->SelectedPort != selectedPort)
    {
    // Clear previous selection.
    // this->clearSelection() fires selectionChanged() signal. We don't want to
    // fire the signal twice unnecessarily, hence we block signals.
    bool oldVal = this->blockSignals(true);
    this->clearSelection();
    this->blockSignals(oldVal);
    }
  this->Implementation->SelectedPort = selectedPort;
  if (selectedPort)
    {
    selectedPort->renderAllViews(false);

    // update the servermanagermodel selection so that the pipeline browser
    // knows which source was selected.
    pqActiveObjects::instance().setActivePort(selectedPort);
    }

  emit this->selectionChanged(selectedPort);
}

//-----------------------------------------------------------------------------
vtkSMSourceProxy* pqSelectionManager::createSelectionSource(vtkSelection* sel, vtkSMSession* session)
{
  // Create a selection source proxy
  vtkSMSessionProxyManager* pm = vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  vtkSMSourceProxy* selectionSource = vtkSMSourceProxy::SafeDownCast(
    pm->NewProxy("sources", "PedigreeIDSelectionSource"));

  // Fill the selection source with the selection
  vtkSMStringVectorProperty* p = vtkSMStringVectorProperty::SafeDownCast(
    selectionSource->GetProperty("IDs"));
  p->SetNumberOfElements(0);
  vtkSMStringVectorProperty* sp = vtkSMStringVectorProperty::SafeDownCast(
    selectionSource->GetProperty("StringIDs"));
  sp->SetNumberOfElements(0);
  unsigned int curId = 0;
  unsigned int curStringId = 0;
  for (unsigned int c = 0; c < sel->GetNumberOfNodes(); ++c)
    {
    vtkSelectionNode* node = sel->GetNode(c);
    vtkAbstractArray* ids = node->GetSelectionList();
    if (ids)
      {
      // Set the ids from the selection
      vtkIdType numTuples = ids->GetNumberOfTuples();
      for (vtkIdType i = 0; i < numTuples; ++i)
        {
        vtkVariant v = ids->GetVariantValue(i);
        if (v.IsString())
          {
          sp->SetElement(2*curStringId+0, ids->GetName());
          sp->SetElement(2*curStringId+1, v.ToString());
          ++curStringId;
          }
        else
          {
          p->SetElement(2*curId+0, ids->GetName());
          p->SetElement(2*curId+1, v.ToString());
          ++curId;
          }
        }
      }
    }
  selectionSource->UpdateProperty("IDs");
  selectionSource->UpdateProperty("StringIDs");

  // Set field type to vertices by default.
  vtkSMPropertyHelper(selectionSource, "FieldType").Set(3);
  selectionSource->UpdateProperty("FieldType");

  return selectionSource;
}

