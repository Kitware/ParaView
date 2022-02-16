/*=========================================================================

   Program: ParaView
   Module:  pqSpreadSheetViewSelectionModel.cxx

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

========================================================================*/
#include "pqSpreadSheetViewSelectionModel.h"

// Server Manager Includes.
#include "vtkPVDataInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMVectorProperty.h"
#include "vtkSelectionNode.h"

// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqSpreadSheetViewModel.h"

namespace
{
class pqScopedBool
{
  bool& Variable;

public:
  pqScopedBool(bool& var)
    : Variable(var)
  {
    this->Variable = true;
  }
  ~pqScopedBool() { this->Variable = false; }
};
}

static uint qHash(pqSpreadSheetViewModel::vtkIndex index)
{
  return qHash(index[2]);
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewSelectionModel::pqSpreadSheetViewSelectionModel(
  pqSpreadSheetViewModel* amodel, QObject* _parent)
  : Superclass(amodel, _parent)
{
  this->UpdatingSelection = false;
  this->Model = amodel;

  QObject::connect(amodel, SIGNAL(selectionChanged(const QItemSelection&)), this,
    SLOT(serverSelectionChanged(const QItemSelection&)));
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewSelectionModel::~pqSpreadSheetViewSelectionModel() = default;

//-----------------------------------------------------------------------------
void pqSpreadSheetViewSelectionModel::serverSelectionChanged(const QItemSelection& sel)
{
  if (this->UpdatingSelection)
  {
    return;
  }

  // turn on this->UpdatingSelection so that we don't attempt to update the
  // SM-selection as the UI is being updated.
  pqScopedBool updatingSelection(this->UpdatingSelection);

  // Don't simple call ClearAndSelect, that causes the UI to flicker. Instead
  // determine what changed and update those.
  QSet<int> currentRows, newRows;
  Q_FOREACH (const QModelIndex& idx, this->selectedIndexes())
  {
    currentRows.insert(idx.row());
  }
  Q_FOREACH (const QModelIndex& idx, sel.indexes())
  {
    newRows.insert(idx.row());
  }

  QSet<int> toDeselectRows = currentRows - newRows;
  QSet<int> toSelectRows = newRows - currentRows;
  Q_FOREACH (int idx, toDeselectRows)
  {
    this->select(
      this->Model->index(idx, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
  }
  Q_FOREACH (int idx, toSelectRows)
  {
    this->select(
      this->Model->index(idx, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewSelectionModel::select(
  const QItemSelection& sel, QItemSelectionModel::SelectionFlags command)
{
  this->Superclass::select(sel, command);
  if (this->UpdatingSelection || command == QItemSelectionModel::NoUpdate)
  {
    return;
  }

  // Obtain the currently set (or create a new one) append selections with its selection source
  // We then update the ids selected on the selection source
  vtkSmartPointer<vtkSMSourceProxy> appendSelections;
  appendSelections.TakeReference(this->getSelectionSource());
  if (!appendSelections)
  {
    Q_EMIT this->selection(nullptr);
    return;
  }

  auto selectionSource = vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(0);
  vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(selectionSource->GetProperty("IDs"));
  QList<QVariant> ids = pqSMAdaptor::getMultipleElementProperty(vp);
  int numElementsPerCommand = vp->GetNumberOfElementsPerCommand();
  if (command & QItemSelectionModel::Clear)
  {
    ids.clear();
  }
  if (command & QItemSelectionModel::Select || command & QItemSelectionModel::Deselect ||
    command & QItemSelectionModel::Toggle)
  {
    // Get the (process id, index) pairs for the indices indicated in the
    // selection.
    QSet<pqSpreadSheetViewModel::vtkIndex> vtkIndices = this->Model->getVTKIndices(sel.indexes());

    QSet<pqSpreadSheetViewModel::vtkIndex> currentIndices;
    for (int cc = 0; (cc + numElementsPerCommand) <= ids.size();)
    {
      pqSpreadSheetViewModel::vtkIndex index;
      if (numElementsPerCommand == 3)
      {
        index[0] = ids[cc++].value<vtkIdType>();
        index[1] = ids[cc++].value<vtkIdType>();
        index[2] = ids[cc++].value<vtkIdType>();
      }
      else // numElementsPerCommand == 2
      {
        index[0] = 0;
        index[1] = ids[cc++].value<vtkIdType>();
        index[2] = ids[cc++].value<vtkIdType>();
      }
      currentIndices.insert(index);
    }

    if (command & QItemSelectionModel::Select)
    {
      currentIndices += vtkIndices;
    }
    if (command & QItemSelectionModel::Deselect)
    {
      currentIndices -= vtkIndices;
    }
    if (command & QItemSelectionModel::Toggle)
    {
      QSet<pqSpreadSheetViewModel::vtkIndex> toSelect = vtkIndices - currentIndices;
      QSet<pqSpreadSheetViewModel::vtkIndex> toDeselect = vtkIndices - toSelect;
      currentIndices -= toDeselect;
      currentIndices += toSelect;
    }

    ids.clear();
    for (const auto& index : currentIndices)
    {
      if (numElementsPerCommand == 3)
      {
        ids.push_back(index[0]);
        ids.push_back(index[1]);
        ids.push_back(index[2]);
      }
      else // numElementsPerCommand == 2
      {
        ids.push_back(index[1]);
        ids.push_back(index[2]);
      }
    }
  }

  if (ids.empty())
  {
    appendSelections = nullptr;
  }
  else
  {
    pqSMAdaptor::setMultipleElementProperty(vp, ids);
    selectionSource->UpdateVTKObjects();

    // Map from selection source proxy name to trace function
    std::string functionName(selectionSource->GetXMLName());
    functionName.erase(functionName.size() - sizeof("SelectionSource") + 1);
    functionName.append("s");
    functionName.insert(0, "Select");

    // Trace the selection
    SM_SCOPED_TRACE(CallFunction)
      .arg(functionName.c_str())
      .arg("IDs", vtkSMPropertyHelper(selectionSource, "IDs").GetIntArray())
      .arg("FieldType", vtkSMPropertyHelper(selectionSource, "FieldType").GetAsInt())
      .arg("ContainingCells", vtkSMPropertyHelper(selectionSource, "ContainingCells").GetAsInt());
  }

  Q_EMIT this->selection(appendSelections);
}

//-----------------------------------------------------------------------------
// Locate the selection source currently set on the representation being shown.
// If no selection exists, or selection present is not "updatable" by this
// model, we create a new selection.
vtkSMSourceProxy* pqSpreadSheetViewSelectionModel::getSelectionSource()
{
  pqDataRepresentation* repr = this->Model->activeRepresentation();
  if (!repr)
  {
    return nullptr;
  }

  // Convert fieldType to selection field type if convert-able.
  int fieldType = this->Model->getFieldType();
  int selectionFieldType = -1;
  switch (fieldType)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      selectionFieldType = vtkSelectionNode::POINT;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      selectionFieldType = vtkSelectionNode::CELL;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      selectionFieldType = vtkSelectionNode::VERTEX;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      selectionFieldType = vtkSelectionNode::EDGE;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      selectionFieldType = vtkSelectionNode::ROW;
      break;

    default:
      return nullptr;
  }

  pqOutputPort* opport = repr->getOutputPortFromInput();
  vtkSMSourceProxy* appendSelections = opport->getSelectionInput();

  // Determine what selection proxy name we want.
  const char* proxyname = "IDSelectionSource";
  vtkPVDataInformation* dinfo = opport->getDataInformation();
  if (dinfo->IsCompositeDataSet())
  {
    proxyname = "CompositeDataIDSelectionSource";
  }

  // We may be able to simply update the currently existing selection.
  bool updatable = false;
  if (appendSelections != nullptr &&
    vtkSMPropertyHelper(appendSelections, "Input").GetNumberOfElements() == 1)
  {
    auto selectionSource = vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(0);
    updatable = strcmp(selectionSource->GetXMLName(), proxyname) == 0 &&
      pqSMAdaptor::getElementProperty(selectionSource->GetProperty("FieldType")).toInt() ==
        selectionFieldType;
  }

  if (updatable)
  {
    appendSelections->Register(nullptr);
  }
  else
  {
    // create a new selection source
    vtkSMSessionProxyManager* pxm = repr->proxyManager();
    vtkSmartPointer<vtkSMSourceProxy> selectionSource;
    selectionSource.TakeReference(
      vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", proxyname)));
    pqSMAdaptor::setElementProperty(selectionSource->GetProperty("FieldType"), selectionFieldType);
    selectionSource->UpdateVTKObjects();
    // create a new append Selections filter and append the selection source
    appendSelections = vtkSMSourceProxy::SafeDownCast(
      vtkSMSelectionHelper::NewAppendSelectionsFromSelectionSource(selectionSource));
  }

  return appendSelections;
}
