/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqCopyReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqApplyBehavior.h"
#include "pqObjectBuilder.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqProxySelection.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
#include "vtkAlgorithm.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyClipboard.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTransferFunctionManager.h"

#include <QPointer>
#include <QSet>

// necessary for using QPointer in a QSet
template <class T>
static uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}

QSet<QPointer<pqPipelineSource>> pqCopyReaction::FilterSelection;

//-----------------------------------------------------------------------------
pqCopyReaction::pqCopyReaction(QAction* parentObject, bool paste_mode, bool pipeline_mode)
  : Superclass(parentObject)
  , Paste(paste_mode)
  , CreatePipeline(pipeline_mode)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqCopyReaction::~pqCopyReaction() = default;

//-----------------------------------------------------------------------------
void pqCopyReaction::updateEnableState()
{
  if (this->Paste && !this->CreatePipeline)
  {
    QObject* clipboard = pqApplicationCore::instance()->manager("SOURCE_ON_CLIPBOARD");
    pqPipelineSource* active = pqActiveObjects::instance().activeSource();
    this->parentAction()->setEnabled(
      clipboard != nullptr && active != clipboard && active != nullptr);
  }
  else if (this->Paste && this->CreatePipeline)
  {
    this->parentAction()->setEnabled(this->canPastePipeline());
  }
  else if (!this->Paste && !this->CreatePipeline)
  {
    this->parentAction()->setEnabled(pqActiveObjects::instance().activeSource() != nullptr);
  }
  else
  {
    this->parentAction()->setEnabled(pqCopyReaction::canCopyPipeline());
  }
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copy()
{
  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  if (!activeSource)
  {
    qDebug("Could not find an active source to copy to.");
    return;
  }

  // since pqApplicationCore uses QPointer for the managers, we don't have to
  // worry about unregistering the source when it is deleted.
  pqApplicationCore* appCore = pqApplicationCore::instance();
  // need to remove any previous SOURCE_ON_CLIPBOARD else pqApplicationCore
  // warns.
  appCore->unRegisterManager("SOURCE_ON_CLIPBOARD");
  appCore->registerManager("SOURCE_ON_CLIPBOARD", activeSource);
}

//-----------------------------------------------------------------------------
void pqCopyReaction::paste()
{
  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  pqPipelineSource* clipboard =
    qobject_cast<pqPipelineSource*>(pqApplicationCore::instance()->manager("SOURCE_ON_CLIPBOARD"));
  if (!clipboard)
  {
    qDebug("No source on clipboard to copy from.");
    return;
  }
  pqCopyReaction::copy(activeSource->getProxy(), clipboard->getProxy());
  activeSource->renderAllViews();
}

namespace
{
// checks that source is a downstream filter of some item in selection
bool checkDownstream(QList<pqPipelineSource*>& selection, pqPipelineSource* source)
{
  for (auto src2 : selection)
  {
    if (src2 == source)
    {
      continue;
    }
    auto consumers = src2->getAllConsumers();
    if (consumers.contains(source))
    {
      return true;
    }
    return checkDownstream(consumers, source);
  }

  return false;
}

// for a given filter, checks if their parent filter is contained in selection
template <typename T>
bool selectionContainsParent(T& selection, pqPipelineSource* source)
{
  for (auto& item : selection)
  {
    auto parent = qobject_cast<pqPipelineSource*>(item);
    if (parent == source)
    {
      continue;
    }
    auto consumers = parent->getAllConsumers();
    if (consumers.contains(source))
    {
      return true;
    }
  }
  return false;
}
}

//-----------------------------------------------------------------------------
bool pqCopyReaction::canCopyPipeline()
{
  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  if (!selModel || selModel->GetNumberOfSelectedProxies() == 0)
  {
    return false;
  }

  pqProxySelection selection;
  pqProxySelectionUtilities::copy(selModel, selection);
  selection = pqProxySelectionUtilities::getPipelineProxies(selection);
  for (auto& item1 : selection)
  {
    auto source1 = qobject_cast<pqPipelineSource*>(item1);
    // make sure a data source wasn't selected
    // only want to copy filters
    if (!source1 || source1->getSourceProxy()->GetNumberOfProducers() == 0)
    {
      return false;
    }

    // we don't want to copy selected filters if they're not contiguous
    // check for other filters in the selection that are downstream of
    // source1, but their input is not in the selection
    auto consumers1 = source1->getAllConsumers();
    for (auto& item2 : selection)
    {
      if (item1 == item2)
      {
        continue;
      }
      auto source2 = qobject_cast<pqPipelineSource*>(item2);
      if (!source2 ||
        (checkDownstream(consumers1, source2) && !selectionContainsParent(selection, source2)))
      {
        return false;
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
bool pqCopyReaction::canPastePipeline()
{
  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  if (!activeSource)
  {
    return false;
  }

  if (FilterSelection.empty())
  {
    return false;
  }

  auto activeSourceDI = activeSource->getSourceProxy()->GetDataInformation();
  for (auto source : FilterSelection)
  {
    if (!source)
    {
      // Since we have an invalid entry in FilterSelection (i.e., some filter
      // has been deleted since the initial copyPipeline(), we'll just clear
      // out FilterSelection)
      FilterSelection.clear();
      return false;
    }

    // now check that the copied filters can accept the type of the active source
    auto obj = vtkAlgorithm::SafeDownCast(source->getSourceProxy()->GetClientSideObject());
    if (obj)
    {
      auto filterInfo = obj->GetInputPortInformation(0);
      if (filterInfo->Has(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()) &&
        filterInfo->Length(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()) > 0)
      {
        auto length = filterInfo->Length(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
        bool found = false;
        for (int i = 0; i < length; i++)
        {
          const char* inputType = filterInfo->Get(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), i);
          if (activeSourceDI->DataSetTypeIsA(inputType))
          {
            found = true;
          }
        }
        if (!found)
        {
          return false;
        }
      }
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copyPipeline()
{
  if (!canCopyPipeline())
  {
    return;
  }

  vtkSMProxySelectionModel* selModel = pqActiveObjects::instance().activeSourcesSelectionModel();
  if (!selModel || selModel->GetNumberOfSelectedProxies() == 0)
  {
    qDebug("Could not find active sources to copy.");
    return;
  }

  FilterSelection.clear();
  pqProxySelection selection;
  pqProxySelectionUtilities::copy(selModel, selection);
  selection = pqProxySelectionUtilities::getPipelineProxies(selection);
  for (auto& item : selection)
  {
    // make sure we haven't selected a data source
    // only want to copy filters
    if (auto source = qobject_cast<pqPipelineSource*>(item))
    {
      if (source->getSourceProxy()->GetNumberOfProducers() > 0)
      {
        FilterSelection.insert(source);
      }
    }
  }
}

namespace
{
//-----------------------------------------------------------------------------
void copyDescendants(pqObjectBuilder* builder, pqPipelineSource* source, pqPipelineSource* parent,
  QSet<QPointer<pqPipelineSource>>& selection, int port = 0)
{
  vtkSMSourceProxy* proxy = source->getSourceProxy();

  // Create a copy
  pqPipelineSource* child =
    builder->createFilter(proxy->GetXMLGroup(), proxy->GetXMLName(), parent, port);
  selection.remove(source);

  // Copy properties into the new filter
  pqCopyReaction::copy(child->getSourceProxy(), proxy);

  // Run the filter
  child->updatePipeline();

  // Deactivate Apply button for this filter
  child->setModifiedState(pqProxy::UNMODIFIED);

  pqView* view = pqActiveObjects::instance().activeView();

  // Update the representation of the output
  for (int outputPort = 0; outputPort < source->getNumberOfOutputPorts(); ++outputPort)
  {
    pqDataRepresentation* repr =
      builder->createDataRepresentation(child->getOutputPort(outputPort), view);
    if (repr)
    {
      vtkSMPVRepresentationProxy::SetupLookupTable(repr->getProxy());
    }
  }

  // Hide the parent filter if its representation is SURFACE or SURFACE_WITH_EDGES
  if (pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(child))
  {
    pqApplyBehavior::hideInputIfRequired(filter, view);
  }

  // Recurse
  auto consumers = source->getAllConsumers();
  for (int i = 0; i < consumers.size(); ++i)
  {
    auto consumer = consumers[i];
    if (selection.contains(consumer))
    {
      copyDescendants(builder, consumer, child, selection, i);
    }
  }
}
}

//-----------------------------------------------------------------------------
void pqCopyReaction::pastePipeline()
{
  if (!canPastePipeline())
  {
    return;
  }

  pqPipelineSource* activeSource = pqActiveObjects::instance().activeSource();
  if (FilterSelection.empty())
  {
    qDebug("No source on clipboard to copy from.");
    return;
  }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();

  QSet<QPointer<pqPipelineSource>> selection = FilterSelection;
  BEGIN_UNDO_SET(QString("Paste Pipeline"));

  do
  {
    for (auto source : selection)
    {
      if (selectionContainsParent(selection, source))
      {
        continue;
      }
      copyDescendants(builder, source, activeSource, selection);
      break;
    }
  } while (!selection.empty());
  END_UNDO_SET();
  activeSource->renderAllViews();
}

//-----------------------------------------------------------------------------
void pqCopyReaction::copy(vtkSMProxy* dest, vtkSMProxy* source)
{
  if (dest && source)
  {
    BEGIN_UNDO_SET(tr("Copy Properties"));

    vtkNew<vtkSMProxyClipboard> clipboard;
    clipboard->Copy(source);
    clipboard->Paste(dest);

    END_UNDO_SET();
  }
}
