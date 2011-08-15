/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetView.cxx

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
#include "pqSpreadSheetView.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"

// Qt Includes.
#include <QHeaderView>
#include <QPointer>
#include <QVBoxLayout>

// ParaView Includes.
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqSpreadSheetViewModel.h"
#include "pqSpreadSheetViewSelectionModel.h"
#include "pqSpreadSheetViewWidget.h"

//-----------------------------------------------------------------------------
class pqSpreadSheetView::pqInternal
{
public:
  pqInternal(pqSpreadSheetViewModel* model):Model(model), SelectionModel(model)
  {
  pqSpreadSheetViewWidget* table = new pqSpreadSheetViewWidget();
  table->setAlternatingRowColors(true);

  this->Table= table;
  this->Table->setModel(this->Model);
  this->Table->setAlternatingRowColors(true);
  this->Table->setCornerButtonEnabled(false);
  this->Table->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Table->setSelectionModel(&this->SelectionModel);
  this->Table->horizontalHeader()->setMovable(true);
  this->SingleColumnMode = false;

  // Do not show the sorting arrow as default
  this->Table->setSortingEnabled(false);
  }

  ~pqInternal()
    {
    delete this->Table;
    }

  QPointer<QWidget> Container;
  QPointer<QTableView> Table;
  pqSpreadSheetViewModel *Model;
  pqSpreadSheetViewSelectionModel SelectionModel;
  bool SingleColumnMode;
};


//-----------------------------------------------------------------------------
pqSpreadSheetView::pqSpreadSheetView(
 const QString& group, const QString& name,
    vtkSMViewProxy* viewModule, pqServer* server,
    QObject* _parent/*=NULL*/):
   pqView(spreadsheetViewType(), group, name, viewModule, server, _parent)
{
  this->Internal = new pqInternal(new pqSpreadSheetViewModel(viewModule, this));
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(
    this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));
  QObject::connect(this, SIGNAL(beginRender()), this, SLOT(onBeginRender()));
  QObject::connect(this, SIGNAL(endRender()), this, SLOT(onEndRender()));

  QObject::connect(
    &this->Internal->SelectionModel, SIGNAL(selection(vtkSMSourceProxy*)),
    this, SLOT(onCreateSelection(vtkSMSourceProxy*)));

  this->getConnector()->Connect(
    viewModule->GetProperty("SelectionOnly"),
    vtkCommand::ModifiedEvent,
    this, SLOT(onSelectionOnly()));
  this->onSelectionOnly();

  foreach(pqRepresentation* rep, this->getRepresentations())
    {
    this->onAddRepresentation(rep);
    }

  this->Internal->Container = new QWidget();
  this->Internal->Container->setObjectName("pqSpreadSheetContainer");
  QVBoxLayout *layout = new QVBoxLayout(this->Internal->Container);
  layout->setSpacing(2);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(this->Internal->Table);

}

//-----------------------------------------------------------------------------
pqSpreadSheetView::~pqSpreadSheetView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqSpreadSheetView::getWidget()
{
  return this->Internal->Container;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onAddRepresentation(pqRepresentation* repr)
{
  this->updateRepresentationVisibility(repr, repr->isVisible());
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::updateRepresentationVisibility(
  pqRepresentation* repr, bool visible)
{
  static bool __updating_visibility__ = false;
  if (__updating_visibility__)
    {
    return;
    }

  if (!visible && repr &&
    this->Internal->Model->activeRepresentation() == repr)
    {
    this->Internal->Model->setActiveRepresentation(NULL);
    emit this->showing(0);
    }

  if (!visible || !repr)
    {
    return;
    }

  __updating_visibility__ = true;

  // If visible, turn-off visibility of all other representations.
  QList<pqRepresentation*> reprs = this->getRepresentations();
  foreach (pqRepresentation* cur_repr, reprs)
    {
    if (cur_repr != repr)
      {
      cur_repr->setVisible(false);
      }
    }
  __updating_visibility__ = false;

  pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr);
  this->Internal->Model->setActiveRepresentation(dataRepr);
  emit this->showing(dataRepr);
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onBeginRender()
{
  // If in "selection-only" mode, and showing composite dataset, we want to make
  // sure that we are shown a block with non-empty cells/points (if possible).
  if (vtkSMPropertyHelper(this->getProxy(),"SelectionOnly").GetAsInt() != 0)
    {
    this->Internal->Model->resetCompositeDataSetIndex();
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onEndRender()
{
  // cout << "Render" << endl;
  //this->Internal->Model.forceUpdate();
  //this->Internal->Model->update();
  this->Internal->Table->viewport()->update();
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetView::canDisplay(pqOutputPort* opPort) const
{
  if(this->Superclass::canDisplay(opPort))
    {
    return true;
    }
  else
    {
    return (opPort && opPort->getServer()->GetConnectionID() ==
            this->getServer()->GetConnectionID());
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onCreateSelection(vtkSMSourceProxy* selSource)
{
  if(this->Internal->Table->selectionMode() == QAbstractItemView::NoSelection)
    return;

  pqDataRepresentation* repr = this->Internal->Model->activeRepresentation();
  if (repr)
    {
    pqOutputPort* opport = repr->getOutputPortFromInput();
    vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
      opport->getSource()->getProxy());
    input->CleanSelectionInputs(opport->getPortNumber());
    if (selSource)
      {
      input->SetSelectionInput(
        opport->getPortNumber(), selSource, 0);
      }
    emit this->selected(opport);
    }
  else
    {
    emit this->selected(0);
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onSelectionOnly()
{
  if (vtkSMPropertyHelper(this->getProxy(), "SelectionOnly").GetAsInt() != 0)
    {
    // The user is disallowed to make further (embedded / recursive) selection
    // once checkbox "Show Only Selected Elements" is checked.
    this->Internal->Table->setSelectionMode(QAbstractItemView::NoSelection);
    }
  else
    {
    // Once the checkbox is un-checked, the user to allowed to make selections.
    this->Internal->Table->setSelectionMode(
      QAbstractItemView::ExtendedSelection);
    }
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel* pqSpreadSheetView::getViewModel()
{
  return this->Internal->Model;
}
