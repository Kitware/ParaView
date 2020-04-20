/*=========================================================================

  Program: ParaView
  Module:    pqSelectionManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqSelectionManager.h"

#include <QtDebug>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqLinksModel.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"
#include "vtkAlgorithm.h"
#include "vtkCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSelectionLink.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
class pqSelectionManagerImplementation
{
public:
  pqSelectionManagerImplementation() {}

  ~pqSelectionManagerImplementation() {}

  QSet<pqOutputPort*> SelectedPorts;
  QPointer<pqView> ActiveView;
};

//-----------------------------------------------------------------------------
pqSelectionManager::pqSelectionManager(QObject* _parent /*=null*/)
  : QObject(_parent)
{
  this->Implementation = new pqSelectionManagerImplementation;
  pqApplicationCore* core = pqApplicationCore::instance();

  pqServerManagerModel* model = core->getServerManagerModel();
  // We need to clear selection when a source is removed. The source
  // that was deleted might have been selected.
  QObject::connect(model, SIGNAL(itemRemoved(pqServerManagerModelItem*)), this,
    SLOT(onItemRemoved(pqServerManagerModelItem*)));

  // When server disconnects we must clean up the selection proxies
  // explicitly. This is needed since the internal selection proxies
  // aren't registered with the proxy manager.
  QObject::connect(model, SIGNAL(aboutToRemoveServer(pqServer*)), this, SLOT(clearSelection()));
  QObject::connect(model, SIGNAL(serverRemoved(pqServer*)), this, SLOT(clearSelection()));

  QObject::connect(
    model, SIGNAL(sourceAdded(pqPipelineSource*)), this, SLOT(onSourceAdded(pqPipelineSource*)));
  QObject::connect(model, SIGNAL(sourceRemoved(pqPipelineSource*)), this,
    SLOT(onSourceRemoved(pqPipelineSource*)));

  pqApplicationCore::instance()->registerManager("SelectionManager", this);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setActiveView(pqView*)));
  this->setActiveView(pqActiveObjects::instance().activeView());

  // When a selection link is added or removed, we need to update the pqSelectionManager
  // So it keeps the SelectedPorts up to date and render any selection that
  // may have been updated
  QObject::connect(pqApplicationCore::instance()->getLinksModel(), SIGNAL(linkAdded(int)), this,
    SLOT(onLinkAdded(int)));
  QObject::connect(pqApplicationCore::instance()->getLinksModel(),
    SIGNAL(linkRemoved(const QString&)), this, SLOT(onLinkRemoved()));
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
    QObject::connect(view, SIGNAL(selected(pqOutputPort*)), this, SLOT(select(pqOutputPort*)));
  }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::onItemRemoved(pqServerManagerModelItem* item)
{
  // return if removed item is not a pqPipelineSource
  if (qobject_cast<pqPipelineSource*>(item) == NULL)
  {
    return;
  }

  // Search for the source output ports in the SelectedPorts set
  foreach (pqOutputPort* port, this->Implementation->SelectedPorts)
  {
    if (port->getSource() == item)
    {
      // Remove it from set
      this->Implementation->SelectedPorts.remove(port);
      return;
    }
  }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::expandSelection(int layers)
{
  for (auto port : this->Implementation->SelectedPorts)
  {
    if (auto selsource = port->getSelectionInput())
    {
      vtkSMPropertyHelper helper(selsource, "NumberOfLayers");
      helper.Set(helper.GetAsInt() + layers);
      selsource->UpdateVTKObjects();
    }
    port->renderAllViews();
    Q_EMIT this->selectionChanged(port);
  }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::clearSelection(pqOutputPort* outputPort)
{
  if (outputPort == NULL)
  {
    // Clear all selection
    if (this->Implementation->SelectedPorts.count() > 0)
    {
      vtkSMSourceProxy* src = (*this->Implementation->SelectedPorts.begin())->getSourceProxy();
      src->CleanSelectionInputs((*this->Implementation->SelectedPorts.begin())->getPortNumber());

      // Render all cleaned output ports
      foreach (pqOutputPort* opport, this->Implementation->SelectedPorts)
      {
        if (opport)
        {
          opport->renderAllViews(false);
        }
      }

      // Clear the selectedPorts set
      this->Implementation->SelectedPorts.clear();

      SM_SCOPED_TRACE(CallFunction).arg("ClearSelection").arg("comment", "clear all selections");

      // inform selection have changed
      Q_EMIT this->selectionChanged(static_cast<pqOutputPort*>(0));
    }
  }
  else
  {
    // Clear selection of one output port
    vtkSMSourceProxy* src = outputPort->getSourceProxy();
    src->CleanSelectionInputs(outputPort->getPortNumber());

    // Remove output port from set
    this->Implementation->SelectedPorts.remove(outputPort);

    SM_SCOPED_TRACE(CallFunction)
      .arg("ClearSelection")
      .arg("Source", outputPort->getSourceProxy())
      .arg("comment", "clear selection for source");

    // Render cleaned output port
    outputPort->renderAllViews(false);

    // Inform selection have been changed
    Q_EMIT this->selectionChanged(outputPort);
  }
}

//-----------------------------------------------------------------------------
pqOutputPort* pqSelectionManager::getSelectedPort() const
{
  if (this->hasActiveSelection())
  {
    return *this->Implementation->SelectedPorts.begin();
  }
  else
  {
    return NULL;
  }
}

//-----------------------------------------------------------------------------
const QSet<pqOutputPort*>& pqSelectionManager::getSelectedPorts() const
{
  return this->Implementation->SelectedPorts;
}

//-----------------------------------------------------------------------------
bool pqSelectionManager::hasActiveSelection() const
{
  return this->Implementation->SelectedPorts.count() != 0;
}

//-----------------------------------------------------------------------------
void pqSelectionManager::select(pqOutputPort* selectedPort)
{
  // The port has a selection change, so update our state.

  // If current selected output ports does NOT contain new selected port,
  // we need to clear it.
  if (!this->Implementation->SelectedPorts.contains(selectedPort))
  {
    // Clear previous selection.
    // this->clearSelection() fires selectionChanged() signal. We don't want to
    // fire the signal twice unnecessarily, hence we block signals.
    bool oldVal = this->blockSignals(true);
    this->clearSelection();
    this->blockSignals(oldVal);
  }
  // If not, we need to render all selected output ports in case a link have been removed
  // hence some selection cleared without our knowing
  else
  {
    foreach (pqOutputPort* port, this->Implementation->SelectedPorts)
    {
      port->renderAllViews(false);
    }
  }

  // Cleanup the set, before filling it again
  this->Implementation->SelectedPorts.clear();

  if (selectedPort != NULL)
  {

    // Insert the selected port and render it
    this->Implementation->SelectedPorts.insert(selectedPort);
    selectedPort->renderAllViews(false);

    // Recover singleton
    pqLinksModel* model = pqApplicationCore::instance()->getLinksModel();
    pqServerManagerModel* psmm = pqApplicationCore::instance()->getServerManagerModel();

    // Recover links using selected port proxy as an input proxy in the link collection
    vtkNew<vtkCollection> selectionLinks;
    vtkSMSourceProxy* selectedProxy = selectedPort->getSourceProxy();
    model->FindLinksFromProxy(selectedProxy, vtkSMLink::INPUT, selectionLinks.Get());

    // insert the proxy in the checked proxy set
    QSet<vtkSMProxy*> checkedInputProxy;
    checkedInputProxy.insert(selectedProxy);

    // For each found selection link
    for (int i = 0; i < selectionLinks->GetNumberOfItems(); i++)
    {
      // check it is a selection link
      vtkSMSelectionLink* selectionLink =
        vtkSMSelectionLink::SafeDownCast(selectionLinks->GetItemAsObject(i));
      if (selectionLink != NULL)
      {
        for (unsigned int j = 0; j < selectionLink->GetNumberOfLinkedObjects(); j++)
        {
          // Find output proxy in the selection link
          if (selectionLink->GetLinkedObjectDirection(j) == vtkSMLink::OUTPUT)
          {
            vtkSMProxy* proxy = selectionLink->GetLinkedProxy(j);

            // if the output proxy has not been checked
            // look for links containing this proxy as an input
            // and add the result to the link collection
            if (!checkedInputProxy.contains(proxy))
            {
              model->FindLinksFromProxy(proxy, vtkSMLink::INPUT, selectionLinks.Get());
              checkedInputProxy.insert(proxy);
            }

            // Find the source associated to the output proxy
            pqPipelineSource* linkedSource = psmm->findItem<pqPipelineSource*>(proxy);

            // Check it is valid
            if (linkedSource != NULL &&
              linkedSource->getNumberOfOutputPorts() > selectedPort->getPortNumber())
            {
              // Recover the corresponding outputport
              pqOutputPort* linkedPort = linkedSource->getOutputPort(selectedPort->getPortNumber());

              // Render it
              linkedPort->renderAllViews(false);

              // Insert it
              this->Implementation->SelectedPorts.insert(linkedPort);
            }
          }
        }
      }
    }

    // update the servermanagermodel selection so that the pipeline browser
    // knows which source was selected.
    pqActiveObjects::instance().setActivePort(selectedPort);
  }

  // Inform about the selection
  Q_EMIT this->selectionChanged(selectedPort);
}

//-----------------------------------------------------------------------------
void pqSelectionManager::onSourceAdded(pqPipelineSource* source)
{
  QObject::connect(
    source, SIGNAL(selectionChanged(pqOutputPort*)), this, SIGNAL(selectionChanged(pqOutputPort*)));
}

//-----------------------------------------------------------------------------
void pqSelectionManager::onSourceRemoved(pqPipelineSource* source)
{
  QObject::disconnect(
    source, SIGNAL(selectionChanged(pqOutputPort*)), this, SIGNAL(selectionChanged(pqOutputPort*)));
}

//-----------------------------------------------------------------------------
void pqSelectionManager::onLinkAdded(int linkType)
{
  // Check it is a selection link
  if (linkType == pqLinksModel::Selection)
  {
    // Reupdate current selection in case it is concerned by the link
    this->select(this->getSelectedPort());
  }
}

//-----------------------------------------------------------------------------
void pqSelectionManager::onLinkRemoved()
{
  // When removing a link, the set of selected port became invalid
  // We have to look for a potential selection in the set of selected port
  foreach (pqOutputPort* port, this->Implementation->SelectedPorts)
  {
    vtkSMSourceProxy* proxy = port->getSourceProxy();
    for (unsigned int i = 0; i < proxy->GetNumberOfOutputPorts(); i++)
    {
      // if the port contains a selection
      if (port->getSourceProxy()->GetSelectionInput(i) != NULL)
      {
        // Reupdate current selection with the found selected port
        this->select(port);
        return;
      }
      // If not, render it in case it has just been cleaned
      else
      {
        port->renderAllViews(false);
      }
    }
  }
}

//-----------------------------------------------------------------------------
vtkBoundingBox pqSelectionManager::selectedDataBounds() const
{
  vtkBoundingBox bbox;
  foreach (pqOutputPort* port, this->Implementation->SelectedPorts)
  {
    double bds[6] = { 0, -1, 0, -1, 0, -1 };
    port->getSelectedDataInformation()->GetBounds(bds);
    bbox.AddBounds(bds);
  }
  return bbox;
}
