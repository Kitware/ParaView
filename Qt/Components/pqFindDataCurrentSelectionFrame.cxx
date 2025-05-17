// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqFindDataCurrentSelectionFrame.h"
#include "ui_pqFindDataCurrentSelectionFrame.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqSpreadSheetColumnsVisibility.h"
#include "pqSpreadSheetView.h"
#include "pqSpreadSheetViewModel.h"
#include "pqTimer.h"
#include "pqUndoStack.h"

#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkView.h"

#include <QCheckBox>
#include <QDebug>
#include <QMenu>
#include <QPointer>
#include <QWidget>

#include <cassert>
#include <memory>

namespace
{
// copied from `pqFindDataCreateSelectionFrame::populateSelectionTypeCombo`.
// `pqFindDataCreateSelectionFrame` class was removed. Until we rewrite this
// class as well, just copying this piece of code over.
void populateSelectionTypeCombo(QComboBox* cbox, pqOutputPort* port)
{
  if (port == nullptr)
  {
    cbox->clear();
    return;
  }

  // preserve the selection type, if possible.
  const QString currentText = cbox->currentText();

  cbox->clear();
  vtkPVDataInformation* dataInfo = port->getDataInformation();
  for (int attributeType = vtkDataObject::POINT;
       attributeType < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++attributeType)
  {
    if (dataInfo->IsAttributeValid(attributeType))
    {
      const char* label = vtkSMFieldDataDomain::GetAttributeTypeAsString(attributeType);
      switch (attributeType)
      {
        case vtkDataObject::POINT:
          cbox->addItem(QIcon(":/pqWidgets/Icons/pqPointData.svg"), label, attributeType);
          break;

        case vtkDataObject::CELL:
          cbox->addItem(QIcon(":/pqWidgets/Icons/pqCellData.svg"), label, attributeType);
          break;

        case vtkDataObject::VERTEX:
          cbox->addItem(QIcon(":/pqWidgets/Icons/pqCellData.svg"), label, attributeType);
          break;

        case vtkDataObject::EDGE:
          cbox->addItem(QIcon(":/pqWidgets/Icons/pqEdgeCenterData.svg"), label, attributeType);
          break;

        case vtkDataObject::ROW:
          cbox->addItem(QIcon(":/pqWidgets/Icons/pqSpreadsheet.svg"), label, attributeType);
          break;
      }
    }
  }

  int index = cbox->findText(currentText);
  if (index != -1)
  {
    cbox->setCurrentIndex(index);
  }
}
}

class pqFindDataCurrentSelectionFrame::pqInternals
{
  vtkSmartPointer<vtkSMProxy> RepresentationProxy;
  vtkSmartPointer<vtkSMViewProxy> ViewProxy;
  QPointer<pqSpreadSheetViewModel> Model;
  QPointer<pqOutputPort> ShowingPort;
  pqTimer UpdateSpreadSheetTimer;

  void deleteSpreadSheet()
  {
    this->ViewProxy = nullptr;
    this->RepresentationProxy = nullptr;
    delete this->Model;
    this->Ui.spreadsheet->setModel(nullptr);
  }

public:
  Ui::FindDataCurrentSelectionFrame Ui;
  QPointer<pqSelectionManager> SelectionManager;
  QMenu ColumnToggleMenu;

  pqInternals(pqFindDataCurrentSelectionFrame* self)
  {
    this->SelectionManager = qobject_cast<pqSelectionManager*>(
      pqApplicationCore::instance()->manager("SELECTION_MANAGER"));

    this->Ui.setupUi(self);
    this->Ui.gridLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    self->connect(this->SelectionManager, SIGNAL(selectionChanged(pqOutputPort*)),
      SLOT(showSelectedData(pqOutputPort*)));
    self->connect(
      this->Ui.showTypeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateFieldType()));
    self->connect(this->Ui.ToggleFieldData, SIGNAL(toggled(bool)), SLOT(showFieldData(bool)));
    self->connect(
      this->Ui.invertSelectionCheckBox, SIGNAL(toggled(bool)), SLOT(invertSelection(bool)));

    this->showSelectedData(this->SelectionManager->getSelectedPort(), self);

    this->UpdateSpreadSheetTimer.setInterval(100);
    this->UpdateSpreadSheetTimer.setSingleShot(true);
    QObject::connect(
      &this->UpdateSpreadSheetTimer, &QTimer::timeout, [this]() { this->updateSpreadSheetNow(); });
  }

  ~pqInternals() { this->deleteSpreadSheet(); }

  vtkSmartPointer<vtkSMViewProxy> GetViewProxy() const { return ViewProxy; }

  QPointer<pqSpreadSheetViewModel> GetModel() const { return Model; }

  //---------------------------------------------------------------------------
  // setup the proxies needed for the spreadsheet view.
  // we will create a new view/representation is server changes, otherwise
  // we'll reuse the old one.
  void setupSpreadsheet(pqServer* server)
  {
    if (server == nullptr)
    {
      this->deleteSpreadSheet();
      return;
    }

    assert(server != nullptr);

    if (this->ViewProxy && this->ViewProxy->GetSession() != server->session())
    {
      this->deleteSpreadSheet();
    }

    assert(this->ViewProxy == nullptr || (this->ViewProxy->GetSession() == server->session()));
    if (!this->ViewProxy)
    {
      vtkSMSessionProxyManager* pxm = server->proxyManager();

      vtkSMProxy* repr = pxm->NewProxy("representations", "SpreadSheetRepresentation");
      repr->PrototypeOn();
      repr->UpdateVTKObjects();

      vtkNew<vtkSMParaViewPipelineController> controller;
      auto timeKeeper = controller->FindTimeKeeper(server->session());

      vtkSMViewProxy* view =
        vtkSMViewProxy::SafeDownCast(pxm->NewProxy("views", "SpreadSheetView"));
      view->PrototypeOn();
      vtkSMPropertyHelper(view, "SelectionOnly").Set(1);
      vtkSMPropertyHelper(view, "Representations").Set(repr);
      vtkSMPropertyHelper(view, "ViewSize").Set(0, 1);
      vtkSMPropertyHelper(view, "ViewSize").Set(1, 1);
      vtkSMPropertyHelper(view, "ViewTime")
        .Set(0, vtkSMPropertyHelper(timeKeeper, "Time").GetAsDouble());
      view->UpdateVTKObjects();

      this->ViewProxy.TakeReference(view);
      this->RepresentationProxy.TakeReference(repr);

      this->Model = new pqSpreadSheetViewModel(view, this->Ui.spreadsheet);
      this->Model->setActiveRepresentationProxy(repr);

      this->Ui.spreadsheet->setModel(this->Model);
    }
  }

  //---------------------------------------------------------------------------
  void showSelectedData(pqOutputPort* port, pqFindDataCurrentSelectionFrame* self)
  {
    bool new_selection = (port != this->ShowingPort);

    if (this->ShowingPort)
    {
      self->disconnect(this->ShowingPort->getSource());
    }
    this->ShowingPort = port;
    if (port)
    {
      self->connect(
        port->getSource(), SIGNAL(dataUpdated(pqPipelineSource*)), SLOT(updateSpreadSheet()));
    }
    this->setupSpreadsheet(port ? port->getServer() : nullptr);

    bool prev = this->Ui.showTypeComboBox->blockSignals(true);
    ::populateSelectionTypeCombo(this->Ui.showTypeComboBox, port);
    this->Ui.showTypeComboBox->blockSignals(prev);
    this->Ui.invertSelectionCheckBox->setEnabled(port != nullptr);
    if (port)
    {
      vtkSMPropertyHelper(this->RepresentationProxy, "Input")
        .Set(port->getSource()->getProxy(), port->getPortNumber());

      vtkSMSourceProxy* appendSelections = port->getSelectionInput();
      if (new_selection && appendSelections)
      {
        // Checking only one of the inputs of the appendSelections is sufficient
        vtkSMProxy* selectionSource = vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(0);
        // if the selection is a new one, we try to show the element type more
        // appropriate based on the type of selection itself, i.e. if points are
        // being selected, show points. If cells are being selected show cells.
        // We only do this if the selection is "new" so that is user is
        // interactively updating the selection and he picked a specified
        // attribute type, we don't force change it on him/her.
        int index = -1;
        if (auto ftProperty = selectionSource->GetProperty("FieldType"))
        {
          // old-style properties
          index = this->Ui.showTypeComboBox->findData(
            vtkSelectionNode::ConvertSelectionFieldToAttributeType(
              vtkSMPropertyHelper(ftProperty).GetAsInt()));
        }
        else if (auto etProperty = selectionSource->GetProperty("ElementType"))
        {
          // new-style properties.
          index = this->Ui.showTypeComboBox->findData(vtkSMPropertyHelper(etProperty).GetAsInt());
        }
        if (index != -1)
        {
          const QSignalBlocker blocker(this->Ui.showTypeComboBox);
          this->Ui.showTypeComboBox->setCurrentIndex(index);
          // this->Ui.showTypeComboBox->setEnabled(true);
        }
        else
        {
          this->Ui.showTypeComboBox->setEnabled(false);
        }
      }
      this->updateFieldType();

      bool checked =
        appendSelections && (vtkSMPropertyHelper(appendSelections, "InsideOut").GetAsInt() != 0);
      prev = this->Ui.invertSelectionCheckBox->blockSignals(true);
      this->Ui.invertSelectionCheckBox->setChecked(checked);
      this->Ui.invertSelectionCheckBox->blockSignals(prev);
    }
  }

  //---------------------------------------------------------------------------
  // update the field the spreadsheet is showing based on the value of the
  // combobox.
  void updateFieldType()
  {
    if (this->ViewProxy)
    {
      int fieldType =
        this->Ui.showTypeComboBox->itemData(this->Ui.showTypeComboBox->currentIndex()).toInt();
      vtkSMPropertyHelper(this->ViewProxy, "FieldAssociation").Set(fieldType);
      this->ViewProxy->UpdateVTKObjects();
      this->ViewProxy->StillRender();
    }
  }

  void showFieldData(bool show)
  {
    if (this->ViewProxy)
    {
      vtkSMPropertyHelper(this->ViewProxy, "ShowFieldData").Set(show);
      this->ViewProxy->UpdateVTKObjects();
      this->ViewProxy->StillRender();
    }
  }

  void updateSpreadSheet() { this->UpdateSpreadSheetTimer.start(); }

  void updateSpreadSheetNow()
  {
    if (this->ViewProxy)
    {
      // Ensure we're showing the right timestep.
      vtkNew<vtkSMParaViewPipelineController> controller;
      auto timeKeeper = controller->FindTimeKeeper(this->ViewProxy->GetSession());
      vtkSMPropertyHelper(this->ViewProxy, "ViewTime")
        .Set(0, vtkSMPropertyHelper(timeKeeper, "Time").GetAsDouble());
      this->ViewProxy->UpdateVTKObjects();
      this->ViewProxy->StillRender();
    }
    this->Ui.spreadsheet->viewport()->update();

    // cancel timer, since we just updated.
    this->UpdateSpreadSheetTimer.stop();
  }

  pqOutputPort* showingPort() const { return this->ShowingPort; }
};

//-----------------------------------------------------------------------------
pqFindDataCurrentSelectionFrame::pqFindDataCurrentSelectionFrame(
  QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
  , Internals(new pqInternals(this))
{
  auto& internals = *this->Internals;
  internals.Ui.ToggleColumnVisibility->setMenu(&internals.ColumnToggleMenu);
  QObject::connect(&internals.ColumnToggleMenu, &QMenu::aboutToShow,
    [&internals]()
    {
      pqSpreadSheetColumnsVisibility::populateMenu(
        internals.GetViewProxy(), internals.GetModel(), &internals.ColumnToggleMenu);
    });
}

//-----------------------------------------------------------------------------
pqFindDataCurrentSelectionFrame::~pqFindDataCurrentSelectionFrame()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqFindDataCurrentSelectionFrame::showSelectedData(pqOutputPort* port)
{
  this->Internals->showSelectedData(port, this);
  Q_EMIT this->showing(port);
}

//-----------------------------------------------------------------------------
pqOutputPort* pqFindDataCurrentSelectionFrame::showingPort() const
{
  return this->Internals->showingPort();
}

//-----------------------------------------------------------------------------
void pqFindDataCurrentSelectionFrame::updateFieldType()
{
  this->Internals->updateFieldType();
}

//-----------------------------------------------------------------------------
void pqFindDataCurrentSelectionFrame::showFieldData(bool show)
{
  this->Internals->showFieldData(show);
}

//-----------------------------------------------------------------------------
void pqFindDataCurrentSelectionFrame::invertSelection(bool val)
{
  Ui::FindDataCurrentSelectionFrame& ui = this->Internals->Ui;
  ui.invertSelectionCheckBox->blockSignals(true);
  ui.invertSelectionCheckBox->setChecked(val);
  ui.invertSelectionCheckBox->blockSignals(false);

  pqOutputPort* port = this->showingPort();
  if (port)
  {
    vtkSMSourceProxy* selectionSource = port->getSelectionInput();
    if (selectionSource)
    {
      vtkSMPropertyHelper(selectionSource, "InsideOut").Set(val ? 1 : 0);
      selectionSource->UpdateVTKObjects();
      port->renderAllViews();

      this->Internals->updateSpreadSheet();
    }
  }
}

//-----------------------------------------------------------------------------
void pqFindDataCurrentSelectionFrame::updateSpreadSheet()
{
  this->Internals->updateSpreadSheet();
}
