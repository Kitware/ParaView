// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  vtkSMViewProxy* viewModule, pqServer* server, QObject* _parent /*=nullptr*/)
  : pqView(spreadsheetViewType(), group, name, viewModule, server, _parent)
{
  this->Internal = new pqInternal(new pqSpreadSheetViewModel(viewModule, this));
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)), this,
    SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)), this,
    SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));
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

  Q_FOREACH (pqRepresentation* rep, this->getRepresentations())
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
    this->Internal->Model->setActiveRepresentation(nullptr);
    Q_EMIT this->showing(nullptr);
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
    Q_EMIT this->selected(nullptr);
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
