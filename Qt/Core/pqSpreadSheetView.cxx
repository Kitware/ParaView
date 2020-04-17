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
#include "pqNonEditableStyledItemDelegate.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "pqSpreadSheetViewModel.h"
#include "pqSpreadSheetViewSelectionModel.h"
#include "pqSpreadSheetViewWidget.h"

//-----------------------------------------------------------------------------
class pqSpreadSheetView::pqInternal
{
public:
  pqInternal(pqSpreadSheetViewModel* model)
    : Model(model)
    , SelectionModel(model)
    , EmptySelectionModel(model)
  {
    pqSpreadSheetViewWidget* table = new pqSpreadSheetViewWidget();
    table->setAlternatingRowColors(true);

    this->Table = table;
    this->Table->setModel(this->Model);
    this->Table->setAlternatingRowColors(true);
    this->Table->setCornerButtonEnabled(false);
    this->Table->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->Table->setSelectionModel(&this->SelectionModel);

    // Do not show the sorting arrow as default
    this->Table->setSortingEnabled(false);
  }

  ~pqInternal() { delete this->Table; }

  QPointer<QWidget> Container;
  QPointer<QTableView> Table;
  pqSpreadSheetViewModel* Model;
  pqSpreadSheetViewSelectionModel SelectionModel;

  // We use EmptySelectionModel as the selection model for the view when in
  // SelectionOnly mode i.e. when we showing only the selected elements.
  QItemSelectionModel EmptySelectionModel;
};

//-----------------------------------------------------------------------------
pqSpreadSheetView::pqSpreadSheetView(const QString& group, const QString& name,
  vtkSMViewProxy* viewModule, pqServer* server, QObject* _parent /*=NULL*/)
  : pqView(spreadsheetViewType(), group, name, viewModule, server, _parent)
{
  this->Internal = new pqInternal(new pqSpreadSheetViewModel(viewModule, this));
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)), this,
    SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)), this,
    SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));
  QObject::connect(this, SIGNAL(beginRender()), this, SLOT(onBeginRender()));
  QObject::connect(this, SIGNAL(endRender()), this, SLOT(onEndRender()));

  QObject::connect(&this->Internal->SelectionModel, SIGNAL(selection(vtkSMSourceProxy*)), this,
    SLOT(onCreateSelection(vtkSMSourceProxy*)));

  this->getConnector()->Connect(viewModule->GetProperty("SelectionOnly"), vtkCommand::ModifiedEvent,
    this, SLOT(onSelectionOnly()));
  this->onSelectionOnly();

  this->getConnector()->Connect(viewModule->GetProperty("CellFontSize"), vtkCommand::ModifiedEvent,
    this, SLOT(onFontSizeChanged()));

  this->getConnector()->Connect(viewModule->GetProperty("HeaderFontSize"),
    vtkCommand::ModifiedEvent, this, SLOT(onFontSizeChanged()));

  this->onFontSizeChanged();

  foreach (pqRepresentation* rep, this->getRepresentations())
  {
    this->onAddRepresentation(rep);
  }

  this->Internal->Container = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout(this->Internal->Container);
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
QWidget* pqSpreadSheetView::createWidget()
{
  // The viewport is already created. Just return that.
  return this->Internal->Container;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onAddRepresentation(pqRepresentation* repr)
{
  this->updateRepresentationVisibility(repr, repr->isVisible());
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::updateRepresentationVisibility(pqRepresentation* repr, bool visible)
{
  static bool updating_visibility__ = false;
  if (updating_visibility__)
  {
    return;
  }

  if (!visible && repr && this->Internal->Model->activeRepresentation() == repr)
  {
    this->Internal->Model->setActiveRepresentation(NULL);
    Q_EMIT this->showing(0);
  }

  if (!visible || !repr)
  {
    return;
  }

  pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr);
  this->Internal->Model->setActiveRepresentation(dataRepr);
  Q_EMIT this->showing(dataRepr);
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onBeginRender()
{
  // If in "selection-only" mode, and showing composite dataset, we want to make
  // sure that we are shown a block with non-empty cells/points (if possible).
  if (vtkSMPropertyHelper(this->getProxy(), "SelectionOnly").GetAsInt() != 0)
  {
    this->Internal->Model->resetCompositeDataSetIndex();
  }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onEndRender()
{
  // cout << "Render" << endl;
  // this->Internal->Model.forceUpdate();
  // this->Internal->Model->update();
  this->Internal->Table->viewport()->update();
  Q_EMIT this->viewportUpdated();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onCreateSelection(vtkSMSourceProxy* selSource)
{
  if (this->Internal->Table->selectionMode() == QAbstractItemView::NoSelection)
    return;

  pqDataRepresentation* repr = this->Internal->Model->activeRepresentation();
  if (repr)
  {
    pqOutputPort* opport = repr->getOutputPortFromInput();
    vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(opport->getSource()->getProxy());
    input->CleanSelectionInputs(opport->getPortNumber());
    if (selSource)
    {
      input->SetSelectionInput(opport->getPortNumber(), selSource, 0);
    }
    Q_EMIT this->selected(opport);
  }
  else
  {
    Q_EMIT this->selected(0);
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
    this->Internal->Table->setSelectionModel(&this->Internal->EmptySelectionModel);
  }
  else
  {
    // Once the checkbox is un-checked, the user to allowed to make selections.
    this->Internal->Table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->Internal->Table->setSelectionModel(&this->Internal->SelectionModel);
  }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onFontSizeChanged()
{
  int cellFontSize = vtkSMPropertyHelper(this->getProxy(), "CellFontSize", 9).GetAsInt();
  int headerFontSize = vtkSMPropertyHelper(this->getProxy(), "HeaderFontSize", 9).GetAsInt();

  // Setting header fonts directly does not work consistently, so we
  // set a stylesheet instead.
  QString style;
  style = QString("QTableView { font-size: %2pt; }\n"
                  "QHeaderView { font-size: %1pt; }")
            .arg(headerFontSize)
            .arg(cellFontSize);
  this->Internal->Table->setStyleSheet(style);
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel* pqSpreadSheetView::getViewModel()
{
  return this->Internal->Model;
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqSpreadSheetView::activeRepresentation() const
{
  const auto reprs = this->getRepresentations();
  for (auto repr : reprs)
  {
    pqDataRepresentation* drepr = qobject_cast<pqDataRepresentation*>(repr);
    if (drepr && drepr->isVisible())
    {
      return drepr;
    }
  }
  return nullptr;
}
