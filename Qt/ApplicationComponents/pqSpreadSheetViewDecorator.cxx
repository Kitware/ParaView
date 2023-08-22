// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSpreadSheetViewDecorator.h"
#include "ui_pqSpreadSheetViewDecorator.h"

// Qt Includes.
#include <QCheckBox>
#include <QClipboard>
#include <QDebug>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>

#include "pqDataRepresentation.h"
#include "pqEventDispatcher.h"
#include "pqExportReaction.h"
#include "pqOutputPort.h"
#include "pqPropertyLinks.h"
#include "pqSpreadSheetColumnsVisibility.h"
#include "pqSpreadSheetView.h"
#include "pqSpreadSheetViewModel.h"
#include "pqSpreadSheetViewWidget.h"
#include "pqUndoStack.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSMViewProxy.h"
#include "vtkSpreadSheetView.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <set>

namespace
{
// This is used to directly connect an enumeration property to the Qt property
// via `int` rather than their "string" names.
class SpreadsheetConnection : public pqPropertyLinksConnection
{
public:
  SpreadsheetConnection(QObject* qobject, const char* qproperty, const char* qsignal,
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, int smindex, bool use_unchecked_modified_event,
    QObject* parentObject)
    : pqPropertyLinksConnection(qobject, qproperty, qsignal, smproxy, smproperty, smindex,
        use_unchecked_modified_event, parentObject)
  {
  }

  ~SpreadsheetConnection() override = default;

  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    vtkSMPropertyHelper helper(this->propertySM());
    helper.SetUseUnchecked(use_unchecked);
    helper.Set(0, value.toInt());
  }

  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    vtkSMPropertyHelper helper(this->propertySM());
    helper.SetUseUnchecked(use_unchecked);
    return helper.GetAsInt();
  }

private:
  Q_DISABLE_COPY(SpreadsheetConnection);
};
}

class pqSpreadSheetViewDecorator::pqInternal : public Ui::pqSpreadSheetViewDecorator
{
public:
  pqPropertyLinks Links;
  QMenu ColumnToggleMenu;
};

//-----------------------------------------------------------------------------
pqSpreadSheetViewDecorator::pqSpreadSheetViewDecorator(pqSpreadSheetView* view)
  : Superclass(view->widget()) // we make our parent the view's widget.
  , Spreadsheet(view)
  , Internal(new pqSpreadSheetViewDecorator::pqInternal())
{
  auto model = view->getViewModel();
  auto proxy = view->getViewProxy();

  QWidget* container = view->widget();
  QWidget* header = new QWidget(container);
  QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(container->layout());

  auto& internal = *this->Internal;
  internal.setupUi(header);
  internal.Source->setAutoUpdateIndex(false);
  internal.Source->addCustomEntry("None", nullptr);
  internal.Source->fillExistingPorts();

  internal.spinBoxPrecision->setValue(model->getDecimalPrecision());
  QObject::connect(this->Internal->spinBoxPrecision, SIGNAL(valueChanged(int)), this,
    SLOT(displayPrecisionChanged(int)));
  QObject::connect(this->Internal->ToggleFixed, SIGNAL(toggled(bool)), this,
    SLOT(toggleFixedRepresentation(bool)));

  if (auto enumDomain =
        proxy->GetProperty("FieldAssociation")->FindDomain<vtkSMEnumerationDomain>())
  {
    for (int cc = 0, max = enumDomain->GetNumberOfEntries(); cc < max; ++cc)
    {
      internal.Attribute->addItem(
        QCoreApplication::translate("ServerManagerXML", enumDomain->GetEntryText(cc)),
        enumDomain->GetEntryValue(cc));
    }
  }
  this->onCurrentAttributeChange(0);

  this->connect(internal.Attribute, SIGNAL(currentIndexChanged(int)), SIGNAL(uiModified()));
  this->connect(internal.ToggleCellConnectivity, SIGNAL(toggled(bool)), SIGNAL(uiModified()));
  this->connect(internal.ToggleFieldData, SIGNAL(toggled(bool)), SIGNAL(uiModified()));
  this->connect(internal.SelectionOnly, SIGNAL(toggled(bool)), SIGNAL(uiModified()));

  this->connect(internal.Attribute, SIGNAL(currentIndexChanged(int)), this,
    SLOT(onCurrentAttributeChange(int)));

  internal.Links.setUseUncheckedProperties(true);
  QObject::connect(&internal.Links, &pqPropertyLinks::qtWidgetChanged, [proxy, &internal]() {
    SCOPED_UNDO_SET("SpreadSheetView changes");
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy);
    internal.Links.accept();
  });

  internal.Links.addPropertyLink(this, "generateCellConnectivity", SIGNAL(uiModified()), proxy,
    proxy->GetProperty("GenerateCellConnectivity"));
  internal.Links.addPropertyLink(
    this, "showFieldData", SIGNAL(uiModified()), proxy, proxy->GetProperty("ShowFieldData"));
  internal.Links.addPropertyLink(this, "showSelectedElementsOnly", SIGNAL(uiModified()), proxy,
    proxy->GetProperty("SelectionOnly"));
  internal.Links.addPropertyLink<SpreadsheetConnection>(
    this, "fieldAssociation", SIGNAL(uiModified()), proxy, proxy->GetProperty("FieldAssociation"));

  // when the ui is changed, let's render the view to update it.
  QObject::connect(
    &internal.Links, &pqPropertyLinks::qtWidgetChanged, [view]() { view->render(); });

  // Ownership of the menu is not transferred to the tool button.
  internal.ToggleColumnVisibility->setMenu(&internal.ColumnToggleMenu);

  QObject::connect(&internal.ColumnToggleMenu, &QMenu::aboutToShow, [view, model, &internal]() {
    // populate menu with actions for toggling column check state.
    pqSpreadSheetColumnsVisibility::populateMenu(
      view->getViewProxy(), model, &internal.ColumnToggleMenu);
  });

  QObject::connect(this->Internal->Source, SIGNAL(currentIndexChanged(pqOutputPort*)), this,
    SLOT(currentIndexChanged(pqOutputPort*)));
  QObject::connect(this->Spreadsheet, SIGNAL(showing(pqDataRepresentation*)), this,
    SLOT(showing(pqDataRepresentation*)));

  this->Internal->ExportSpreadsheet->setDefaultAction(this->Internal->actionExport);
  new pqExportReaction(this->Internal->ExportSpreadsheet->defaultAction());

  layout->insertWidget(0, header);

  // get the actual repr currently shown by the view.
  this->showing(this->Spreadsheet->activeRepresentation());

  // install event filter from the main widget of the spreadsheet to catch the shortcut
  if (view->widget()->parentWidget() && view->widget()->parentWidget()->parentWidget())
  {
    view->widget()->parentWidget()->parentWidget()->installEventFilter(this);
  }
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewDecorator::~pqSpreadSheetViewDecorator() = default;

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::setPrecision(int p)
{
  this->Internal->spinBoxPrecision->setValue(p);
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::setFixedRepresentation(bool isFixed)
{
  this->Internal->ToggleFixed->setChecked(isFixed);
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetViewDecorator::generateCellConnectivity() const
{
  auto& internal = *this->Internal;
  return internal.ToggleCellConnectivity->isChecked();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::setGenerateCellConnectivity(bool val)
{
  auto& internal = *this->Internal;
  internal.ToggleCellConnectivity->setChecked(val);
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetViewDecorator::showFieldData() const
{
  auto& internal = *this->Internal;
  return internal.ToggleFieldData->isChecked();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::setShowFieldData(bool val)
{
  auto& internal = *this->Internal;
  internal.ToggleFieldData->setChecked(val);
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetViewDecorator::showSelectedElementsOnly() const
{
  auto& internal = *this->Internal;
  return internal.SelectionOnly->isChecked();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::setShowSelectedElementsOnly(bool val)
{
  auto& internal = *this->Internal;
  internal.SelectionOnly->setChecked(val);
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewDecorator::fieldAssociation() const
{
  auto& internal = *this->Internal;
  return internal.Attribute->currentData().toInt();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::setFieldAssociation(int val)
{
  auto& internal = *this->Internal;
  auto idx = internal.Attribute->findData(val);
  if (idx != -1)
  {
    internal.Attribute->setCurrentIndex(idx);
  }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::showing(pqDataRepresentation* repr)
{
  if (repr)
  {
    this->Internal->Source->setCurrentPort(repr->getOutputPortFromInput());
  }
  else
  {
    this->Internal->Source->setCurrentPort(nullptr);
  }
  this->Internal->Attribute->setEnabled(repr != nullptr);
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::currentIndexChanged(pqOutputPort* port)
{
  SCOPED_UNDO_SET("SpreadSheetView visibility");
  if (port)
  {
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    if (controller->Show(
          port->getSourceProxy(), port->getPortNumber(), this->Spreadsheet->getViewProxy()))
    {
      this->Spreadsheet->render();
    }
  }
  else
  {
    if (auto activeRepr = this->Spreadsheet->activeRepresentation())
    {
      assert(activeRepr->isVisible());
      auto inputPort = activeRepr->getOutputPortFromInput();
      vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
      controller->Hide(
        inputPort->getSourceProxy(), inputPort->getPortNumber(), this->Spreadsheet->getViewProxy());
      this->Spreadsheet->render();
    }
  }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::onCurrentAttributeChange(int index)
{
  int currentFieldAssociation = this->Internal->Attribute->itemData(index).toInt();

  if (currentFieldAssociation == vtkDataObject::FIELD_ASSOCIATION_NONE)
  {
    this->Internal->ToggleFieldData->setEnabled(false);
    this->Internal->ToggleFieldData->setChecked(false);
  }
  else
  {
    this->Internal->ToggleFieldData->setEnabled(true);
  }

  bool enableCellConnectivity = (currentFieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS);
  this->Internal->ToggleCellConnectivity->setEnabled(enableCellConnectivity);
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::displayPrecisionChanged(int precision)
{
  this->Spreadsheet->getViewModel()->setDecimalPrecision(precision);
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::toggleFixedRepresentation(bool fixed)
{
  this->Spreadsheet->getViewModel()->setFixedRepresentation(fixed);
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetViewDecorator::allowChangeOfSource() const
{
  return this->Internal->Source->isEnabled();
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::setAllowChangeOfSource(bool val)
{
  this->Internal->Source->setEnabled(val);
  this->Internal->Source->setVisible(val);
  this->Internal->label->setVisible(val);
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetViewDecorator::eventFilter(QObject* object, QEvent* e)
{
  if (e->type() == QEvent::KeyPress)
  {
    auto kev = static_cast<QKeyEvent*>(e);
    if (kev->matches(QKeySequence::Copy))
    {
      this->copyToClipboard();
      return true;
    }
  }

  return Superclass::eventFilter(object, e);
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::copyToClipboard()
{
  bool wasChecked = this->Internal->SelectionOnly->isChecked();
  this->Internal->SelectionOnly->setChecked(true);

  pqEventDispatcher::processEventsAndWait(100);
  auto table = this->Spreadsheet->getViewModel()->GetRowsAsString();

  QApplication::clipboard()->setText(table);

  this->Internal->SelectionOnly->setChecked(wasChecked);
}
