/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetViewDecorator.cxx

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
#include "pqSpreadSheetView.h"
#include "pqSpreadSheetViewModel.h"
#include "pqSpreadSheetViewWidget.h"
#include "pqUndoStack.h"
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

  ~SpreadsheetConnection() = default;

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

namespace detail
{
static QCheckBox* addCheckableAction(QMenu* menu, const QString& text, const bool checked)
{
  QCheckBox* cb = new QCheckBox();
  cb->setObjectName("CheckBox");
  cb->setText(text);
  cb->setChecked(checked);
  // We need a layout to set margins - there are none for Plastic theme by default
  QHBoxLayout* layout = new QHBoxLayout();
  layout->addWidget(cb);
  layout->setContentsMargins(4, 2, 4, 2);
  QWidget* widget = new QWidget(menu);
  widget->setObjectName(text);
  widget->setLayout(layout);
  QWidgetAction* action = new QWidgetAction(menu);
  action->setText(text);
  action->setDefaultWidget(widget);
  menu->addAction(action);
  return cb;
}

static void updateAllCheckState(QCheckBox* allCheckbox, const std::vector<QCheckBox*>& cboxes)
{
  size_t checked = 0;
  for (auto cb : cboxes)
  {
    checked += (cb->isChecked() ? 1 : 0);
  }

  QSignalBlocker sblocker(allCheckbox);
  if (checked == cboxes.size())
  {
    allCheckbox->setCheckState(Qt::Checked);
    allCheckbox->setTristate(false);
  }
  else if (checked == 0)
  {
    allCheckbox->setCheckState(Qt::Unchecked);
    allCheckbox->setTristate(false);
  }
  else
  {
    allCheckbox->setCheckState(Qt::PartiallyChecked);
  }
}

static void populateMenu(pqSpreadSheetView* view, QMenu* menu)
{
  menu->clear();

  // add checkboxes for known columns.
  auto model = view->getViewModel();

  std::vector<std::pair<std::string, bool> > columnLabels;
  std::set<std::string> columnLabelsSet;
  for (int col = 0, max = model->columnCount(); col < max; ++col)
  {
    if (model->headerData(col, Qt::Horizontal, pqSpreadSheetViewModel::SectionInternal).toBool())
    {
      continue; // skip internal columns.
    }
    else
    {
      const std::string label =
        model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString().toLatin1().data();
      bool checked =
        model->headerData(col, Qt::Horizontal, pqSpreadSheetViewModel::SectionVisible).toBool();
      if (columnLabelsSet.insert(label).second == true)
      {
        columnLabels.push_back(std::make_pair(label, checked));
      }
    }
  }

  // add a separator
  columnLabels.push_back(std::make_pair(std::string(), false));

  // if there are any columns already hidden that are not already added, we
  // add them so that the user can always unhide them.
  auto proxy = view->getViewProxy();
  auto svp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("HiddenColumnLabels"));
  for (unsigned int cc = 0, max = svp->GetNumberOfElements(); cc < max; ++cc)
  {
    auto txt = svp->GetElement(cc);
    if (columnLabelsSet.insert(txt).second == true)
    {
      columnLabels.push_back(std::make_pair(txt, false));
    }
  }

  if (columnLabels.size() <= 1) // there's always 1 separator.
  {
    // menu is empty.
    return;
  }

  auto allCheckbox = addCheckableAction(menu, "All Columns", false);
  menu->addSeparator();
  auto checkboxes = std::make_shared<std::vector<QCheckBox*> >();

  for (const auto& pair : columnLabels)
  {
    if (pair.first.empty())
    {
      menu->addSeparator();
    }
    else
    {
      const std::string& label = pair.first;
      const bool& checked = pair.second;
      auto callback = [view, label, checkboxes, allCheckbox](bool is_checked) {
        auto vproxy = view->getViewProxy();
        auto vsvp =
          vtkSMStringVectorProperty::SafeDownCast(vproxy->GetProperty("HiddenColumnLabels"));
        std::vector<std::string> values;
        for (unsigned int cc = 0, max = vsvp->GetNumberOfElements(); cc < max; ++cc)
        {
          values.push_back(vsvp->GetElement(cc));
        }
        if (is_checked)
        {
          values.erase(std::remove(values.begin(), values.end(), label), values.end());
        }
        else if (std::find(values.begin(), values.end(), label) == values.end())
        {
          values.push_back(label);
        }

        SM_SCOPED_TRACE(PropertiesModified).arg("proxy", vproxy);
        SCOPED_UNDO_SET("SpreadSheetView column visibilities");
        vsvp->SetElements(values);
        vproxy->UpdateVTKObjects();
        view->render();

        updateAllCheckState(allCheckbox, (*checkboxes.get()));
      };

      auto cb = addCheckableAction(menu, label.c_str(), checked);
      QObject::connect(cb, &QCheckBox::toggled, callback);
      checkboxes->push_back(cb);
    }
  }

  updateAllCheckState(allCheckbox, (*checkboxes.get()));
  QObject::connect(allCheckbox, &QCheckBox::stateChanged, [view, checkboxes, allCheckbox](
                                                            int checkState) {
    std::vector<std::string> hidden_columns;
    for (auto cb : (*checkboxes))
    {
      QSignalBlocker sblocker(cb);
      cb->setChecked(checkState == Qt::Checked);
      if (checkState != Qt::Checked)
      {
        // all columns are hidden.
        hidden_columns.push_back(cb->text().toLocal8Bit().data());
      }
    }

    // turn off tristate to avoid the `All Columns` checkbox from entering the
    // partially-checked state through user clicks.
    allCheckbox->setTristate(false);

    auto vproxy = view->getViewProxy();
    auto vsvp = vtkSMStringVectorProperty::SafeDownCast(vproxy->GetProperty("HiddenColumnLabels"));

    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", vproxy);
    SCOPED_UNDO_SET("SpreadSheetView column visibilities");
    vsvp->SetElements(hidden_columns);
    vproxy->UpdateVTKObjects();
    view->render();
  });
}
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
  internal.Source->addCustomEntry("None", NULL);
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
      internal.Attribute->addItem(enumDomain->GetEntryText(cc), enumDomain->GetEntryValue(cc));
    }
  }

  this->connect(internal.Attribute, SIGNAL(currentIndexChanged(int)), SIGNAL(uiModified()));
  this->connect(internal.ToggleCellConnectivity, SIGNAL(toggled(bool)), SIGNAL(uiModified()));
  this->connect(internal.SelectionOnly, SIGNAL(toggled(bool)), SIGNAL(uiModified()));

  internal.Links.setUseUncheckedProperties(true);
  QObject::connect(&internal.Links, &pqPropertyLinks::qtWidgetChanged, [proxy, &internal]() {
    SCOPED_UNDO_SET("SpreadSheetView changes");
    SM_SCOPED_TRACE(PropertiesModified).arg("proxy", proxy);
    internal.Links.accept();
  });

  internal.Links.addPropertyLink(this, "generateCellConnectivity", SIGNAL(uiModified()), proxy,
    proxy->GetProperty("GenerateCellConnectivity"));
  internal.Links.addPropertyLink(this, "showSelectedElementsOnly", SIGNAL(uiModified()), proxy,
    proxy->GetProperty("SelectionOnly"));
  internal.Links.addPropertyLink<SpreadsheetConnection>(
    this, "fieldAssociation", SIGNAL(uiModified()), proxy, proxy->GetProperty("FieldAssociation"));

  // when the ui is changed, let's render the view to update it.
  QObject::connect(
    &internal.Links, &pqPropertyLinks::qtWidgetChanged, [view]() { view->render(); });

  // Ownership of the menu is not transferred to the tool button.
  internal.ToggleColumnVisibility->setMenu(&internal.ColumnToggleMenu);

  QObject::connect(&internal.ColumnToggleMenu, &QMenu::aboutToShow, [view, &internal]() {
    // populate menu with actions for toggling column check state.
    detail::populateMenu(view, &internal.ColumnToggleMenu);
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
pqSpreadSheetViewDecorator::~pqSpreadSheetViewDecorator()
{
}

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
