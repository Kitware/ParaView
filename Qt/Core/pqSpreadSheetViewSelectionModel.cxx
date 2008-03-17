/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewSelectionModel.cxx

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

========================================================================*/
#include "pqSpreadSheetViewSelectionModel.h"

// Server Manager Includes.
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkIndexBasedBlockFilter.h"

// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqSpreadSheetViewModel.h"

static uint qHash(QPair<vtkIdType, vtkIdType> pair)
{
  return qHash(pair.second);
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

  QObject::connect(amodel->selectionModel(), 
    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
    this, SLOT(serverSelectionChanged()));
}


//-----------------------------------------------------------------------------
pqSpreadSheetViewSelectionModel::~pqSpreadSheetViewSelectionModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewSelectionModel::serverSelectionChanged()
{
  this->UpdatingSelection = true;
  this->select(this->Internal->Model->selectionModel()->selection(),
    QItemSelectionModel::ClearAndSelect);
  this->UpdatingSelection = false;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewSelectionModel::select(const QItemSelection& sel, 
    QItemSelectionModel::SelectionFlags command)
{
  if (!this->UpdatingSelection && command != QItemSelectionModel::NoUpdate)
    {
    // Update VTK Selection.
    // * Obtain the currently set sel on the selected source (if none, a
    //    new one is created).
    // * We then update the ids selected on the
    vtkSmartPointer<vtkSMSourceProxy> selSource ;
    selSource.TakeReference(this->getSelectionSource());
    if (selSource)
      {
      QList<QVariant> ids = pqSMAdaptor::getMultipleElementProperty(
        selSource->GetProperty("IDs"));
      if (command & QItemSelectionModel::Clear)
        {
        ids.clear();
        }

      if (command & QItemSelectionModel::Select || 
        command & QItemSelectionModel::Deselect || 
        command & QItemSelectionModel::Toggle)
        {
        // Get the (process id, index) pairs for the indices indicated in the 
        // selection.
        QSet<QPair<vtkIdType, vtkIdType> > vtkIndices 
          = this->Internal->Model->getVTKIndices(sel.indexes());

        QSet<QPair<vtkIdType, vtkIdType> > curIndices;
        for (int cc=0; cc < ids.size()/2; cc++)
          {
          curIndices.insert(QPair<vtkIdType, vtkIdType>(
              ids[2*cc].value<vtkIdType>(), ids[2*cc+1].value<vtkIdType>()));
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
          QSet<QPair<vtkIdType, vtkIdType> > toSelect = 
            vtkIndices - curIndices;
          QSet<QPair<vtkIdType, vtkIdType> > toDeselect =
            vtkIndices - toSelect;
          curIndices -= toDeselect;
          curIndices += toSelect;
          }

        ids.clear();
        QSet<QPair<vtkIdType, vtkIdType> >::iterator iter = curIndices.begin();
        for(; iter != curIndices.end(); ++iter)
          {
          QPair<vtkIdType, vtkIdType> pair = (*iter);
          ids.push_back(pair.first);
          ids.push_back(pair.second);
          }
        }
      if (ids.size() == 0)
        {
        selSource = 0;
        }
      else
        {
        pqSMAdaptor::setMultipleElementProperty(
          selSource->GetProperty("IDs"), ids);
        selSource->UpdateVTKObjects();
        }
      }
    emit this->selection(selSource);
    }
  else
    {
    this->Superclass::select(sel, command);
    }
}

//-----------------------------------------------------------------------------
// Locate the selection source currently set on the representation being shown.
// If no selection exists, or selection present is not "updatable" by this
// model, we create a new selection.
vtkSMSourceProxy* pqSpreadSheetViewSelectionModel::getSelectionSource()
{
  pqDataRepresentation* repr = this->Internal->Model->getRepresentation();
  if (!repr)
    {
    return 0;
    }

  int field_type = this->Internal->Model->getFieldType();
  if (field_type == vtkIndexBasedBlockFilter::FIELD)
    {
    return 0;
    }

  int composite_index = pqSMAdaptor::getElementProperty(
    repr->getProxy()->GetProperty("CompositeDataSetIndex")).toInt();

  // convert field_type to selection field type.
  field_type = (field_type == vtkIndexBasedBlockFilter::POINT)?
    vtkSelection::POINT : vtkSelection::CELL;

  pqOutputPort* opport = repr->getOutputPortFromInput();
  vtkSMSourceProxy* selsource = vtkSMSourceProxy::SafeDownCast(
    opport->getSource()->getProxy())->GetSelectionInput(
    opport->getPortNumber());

  // Check if the current selsource is "updatable".
  bool updatable = (selsource != 0) && (pqSMAdaptor::getElementProperty(
      selsource->GetProperty("FieldType")).toInt() == field_type) &&
    (pqSMAdaptor::getElementProperty(selsource->GetProperty("ContentType")).toInt() ==
     vtkSelection::INDICES) &&
    (pqSMAdaptor::getElementProperty(selsource->GetProperty("CompositeIndex")).toInt() 
     == composite_index);

  if (updatable)
    {
    selsource->Register(0);
    }
  else
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    selsource = vtkSMSourceProxy::SafeDownCast(
      pxm->NewProxy("sources", "SelectionSource"));
    selsource->SetConnectionID(repr->getServer()->GetConnectionID());
    selsource->SetServers(vtkProcessModule::DATA_SERVER);
    pqSMAdaptor::setElementProperty(
      selsource->GetProperty("FieldType"), field_type);
    pqSMAdaptor::setElementProperty(
      selsource->GetProperty("ContentType"), vtkSelection::INDICES);
    pqSMAdaptor::setElementProperty(
      selsource->GetProperty("CompositeIndex"), composite_index);
    selsource->UpdateVTKObjects();
    }

  return selsource;
}
