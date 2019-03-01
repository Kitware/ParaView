/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewSelectionModel.cxx

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
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMVectorProperty.h"
#include "vtkSelection.h"
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
  return qHash(index.Tuple[2]);
}

class pqSpreadSheetViewSelectionModel::pqInternal
{
public:
  pqSpreadSheetViewModel* Model;
};

//-----------------------------------------------------------------------------
pqSpreadSheetViewSelectionModel::pqSpreadSheetViewSelectionModel(
  pqSpreadSheetViewModel* amodel, QObject* _parent)
  : Superclass(amodel, _parent)
{
  this->UpdatingSelection = false;
  this->Internal = new pqInternal();
  this->Internal->Model = amodel;

  QObject::connect(amodel, SIGNAL(selectionChanged(const QItemSelection&)), this,
    SLOT(serverSelectionChanged(const QItemSelection&)));
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewSelectionModel::~pqSpreadSheetViewSelectionModel()
{
  delete this->Internal;
}

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
  foreach (const QModelIndex& idx, this->selectedIndexes())
  {
    currentRows.insert(idx.row());
  }
  foreach (const QModelIndex& idx, sel.indexes())
  {
    newRows.insert(idx.row());
  }

  QSet<int> toDeselectRows = currentRows - newRows;
  QSet<int> toSelectRows = newRows - currentRows;
  // cout << "Selecting: " << toSelectRows.size()
  //      << " De-Selection: " << toDeselectRows.size() <<  endl;
  foreach (int idx, toDeselectRows)
  {
    this->select(this->Internal->Model->index(idx, 0),
      QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
  }
  foreach (int idx, toSelectRows)
  {
    this->select(this->Internal->Model->index(idx, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
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

  // Update VTK Selection.
  // * Obtain the currently set sel on the selected source (if none, a
  //    new one is created).
  // * We then update the ids selected on the
  vtkSmartPointer<vtkSMSourceProxy> selSource;
  selSource.TakeReference(this->getSelectionSource());
  if (!selSource)
  {
    emit this->selection(0);
    return;
  }

  vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(selSource->GetProperty("IDs"));
  QList<QVariant> ids = pqSMAdaptor::getMultipleElementProperty(vp);
  int numElemsPerCommand = vp->GetNumberOfElementsPerCommand();
  if (command & QItemSelectionModel::Clear)
  {
    ids.clear();
  }
  if (command & QItemSelectionModel::Select || command & QItemSelectionModel::Deselect ||
    command & QItemSelectionModel::Toggle)
  {
    // Get the (process id, index) pairs for the indices indicated in the
    // selection.
    QSet<pqSpreadSheetViewModel::vtkIndex> vtkIndices =
      this->Internal->Model->getVTKIndices(sel.indexes());

    QSet<pqSpreadSheetViewModel::vtkIndex> curIndices;
    for (int cc = 0; (cc + numElemsPerCommand) <= ids.size();)
    {
      pqSpreadSheetViewModel::vtkIndex index(0, -1, 0);
      if (numElemsPerCommand == 3)
      {
        index.Tuple[0] = ids[cc].value<vtkIdType>();
        cc++;
        index.Tuple[1] = ids[cc].value<vtkIdType>();
        cc++;
        index.Tuple[2] = ids[cc].value<vtkIdType>();
        cc++;
      }
      else // numElemsPerCommand == 2
      {
        index.Tuple[1] = ids[cc].value<vtkIdType>();
        cc++;
        index.Tuple[2] = ids[cc].value<vtkIdType>();
        cc++;
      }
      curIndices.insert(index);
    }

    if (command & QItemSelectionModel::Select)
    {
      curIndices += vtkIndices;
    }
    if (command & QItemSelectionModel::Deselect)
    {
      curIndices -= vtkIndices;
    }
    if (command & QItemSelectionModel::Toggle)
    {
      QSet<pqSpreadSheetViewModel::vtkIndex> toSelect = vtkIndices - curIndices;
      QSet<pqSpreadSheetViewModel::vtkIndex> toDeselect = vtkIndices - toSelect;
      curIndices -= toDeselect;
      curIndices += toSelect;
    }

    ids.clear();
    QSet<pqSpreadSheetViewModel::vtkIndex>::iterator iter;
    for (iter = curIndices.begin(); iter != curIndices.end(); ++iter)
    {
      if (numElemsPerCommand == 3)
      {
        ids.push_back(iter->Tuple[0]);
        ids.push_back(iter->Tuple[1]);
        ids.push_back(iter->Tuple[2]);
      }
      else // numElemsPerCommand == 2
      {
        ids.push_back(iter->Tuple[1]);
        ids.push_back(iter->Tuple[2]);
      }
    }
  }

  if (ids.size() == 0)
  {
    selSource = 0;
  }
  else
  {
    pqSMAdaptor::setMultipleElementProperty(vp, ids);
    selSource->UpdateVTKObjects();
  }

  emit this->selection(selSource);
}

//-----------------------------------------------------------------------------
// Locate the selection source currently set on the representation being shown.
// If no selection exists, or selection present is not "updatable" by this
// model, we create a new selection.
vtkSMSourceProxy* pqSpreadSheetViewSelectionModel::getSelectionSource()
{
  pqDataRepresentation* repr = this->Internal->Model->activeRepresentation();
  if (!repr)
  {
    return 0;
  }

  // Convert field_type to selection field type if convert-able.
  int field_type = this->Internal->Model->getFieldType();
  int selection_field_type = -1;
  switch (field_type)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      selection_field_type = vtkSelectionNode::POINT;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      selection_field_type = vtkSelectionNode::CELL;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      selection_field_type = vtkSelectionNode::VERTEX;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      selection_field_type = vtkSelectionNode::EDGE;
      break;

    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      selection_field_type = vtkSelectionNode::ROW;
      break;

    default:
      return 0;
  }

  pqOutputPort* opport = repr->getOutputPortFromInput();
  vtkSMSourceProxy* selsource = opport->getSelectionInput();

  // We may be able to simply update the currently existing selection, if any.
  bool updatable = (selsource != 0);

  // If field types differ, not updatable.
  if (updatable &&
    pqSMAdaptor::getElementProperty(selsource->GetProperty("FieldType")).toInt() !=
      selection_field_type)
  {
    updatable = false;
  }

  // Determine what selection proxy name we want. If the name differs then not
  // updatable.
  const char* proxyname = "IDSelectionSource";
  vtkPVDataInformation* dinfo = opport->getDataInformation();
  const char* cdclassname = dinfo->GetCompositeDataClassName();
  if (cdclassname && strcmp(cdclassname, "vtkHierarchicalBoxDataSet") == 0)
  {
    proxyname = "HierarchicalDataIDSelectionSource";
  }
  else if (cdclassname)
  {
    proxyname = "CompositeDataIDSelectionSource";
  }

  if (updatable && strcmp(selsource->GetXMLName(), proxyname) != 0)
  {
    updatable = false;
  }

  if (updatable)
  {
    selsource->Register(0);
  }
  else
  {
    vtkSMSessionProxyManager* pxm = repr->proxyManager();
    selsource = vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", proxyname));
    pqSMAdaptor::setElementProperty(selsource->GetProperty("FieldType"), selection_field_type);
    selsource->UpdateVTKObjects();
  }

  return selsource;
}
