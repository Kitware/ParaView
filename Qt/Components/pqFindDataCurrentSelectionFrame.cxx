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
#include "pqFindDataCurrentSelectionFrame.h"
#include "ui_pqFindDataCurrentSelectionFrame.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqSpreadSheetViewModel.h"
#include "vtkDataObject.h"
#include "vtkPVDataInformation.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include <QPointer>
#include <QSignalBlocker>

#include <cassert>

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

  pqInternals(pqFindDataCurrentSelectionFrame* self)
  {
    this->SelectionManager = qobject_cast<pqSelectionManager*>(
      pqApplicationCore::instance()->manager("SELECTION_MANAGER"));

    this->Ui.setupUi(self);
    this->Ui.gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    self->connect(this->SelectionManager, SIGNAL(selectionChanged(pqOutputPort*)),
      SLOT(showSelectedData(pqOutputPort*)));
    self->connect(
      this->Ui.showTypeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateFieldType()));
    this->showSelectedData(this->SelectionManager->getSelectedPort(), self);
  }

  ~pqInternals() { this->deleteSpreadSheet(); }

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

      vtkSMViewProxy* view =
        vtkSMViewProxy::SafeDownCast(pxm->NewProxy("views", "SpreadSheetView"));
      view->PrototypeOn();
      vtkSMPropertyHelper(view, "SelectionOnly").Set(1);
      vtkSMPropertyHelper(view, "Representations").Set(repr);
      vtkSMPropertyHelper(view, "ViewSize").Set(0, 1);
      vtkSMPropertyHelper(view, "ViewSize").Set(1, 1);
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
    if (port)
    {
      vtkSMPropertyHelper(this->RepresentationProxy, "Input")
        .Set(port->getSource()->getProxy(), port->getPortNumber());

      vtkSMSourceProxy* selectionSource = port->getSelectionInput();
      if (new_selection && selectionSource)
      {
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
          this->Ui.showTypeComboBox->setEnabled(true);
        }
        else
        {
          this->Ui.showTypeComboBox->setEnabled(false);
        }
      }
      this->updateFieldType();
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

  void updateSpreadSheet()
  {
    if (this->ViewProxy)
    {
      this->ViewProxy->StillRender();
    }
  }

  pqOutputPort* showingPort() const { return this->ShowingPort; }
};

//-----------------------------------------------------------------------------
pqFindDataCurrentSelectionFrame::pqFindDataCurrentSelectionFrame(
  QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
  , Internals(new pqInternals(this))
{
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
void pqFindDataCurrentSelectionFrame::updateSpreadSheet()
{
  this->Internals->updateSpreadSheet();
}
