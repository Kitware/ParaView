/*=========================================================================

   Program: ParaView
   Module:  pqFindDataWidget.cxx

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
#include "pqFindDataWidget.h"
#include "ui_pqFindDataWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"
#include "pqProxyWidget.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkPVDataInformation.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkSelectionNode.h"

#include <QMessageBox>
#include <QPushButton>
#include <QScopedValueRollback>

//=============================================================================
class pqFindDataWidget::pqInternals
{
  mutable bool InFindData = false;

public:
  Ui::FindDataWidget Ui;
  QPointer<pqServer> Server;
  QPointer<pqProxyWidget> ProxyWidget;

  void findData() const;
  void extract() const;
  void plotOverTime() const;
  void freeze() const;
  void handleSelectionChanged(bool hasSelection)
  {
    if (this->InFindData == false)
    {
      this->Ui.clear->click();
    }

    this->Ui.extract->setEnabled(hasSelection);
    this->Ui.freeze->setEnabled(hasSelection);
    this->Ui.plotOverTime->setEnabled(hasSelection);
  }

  static int selectedElementType(vtkSMProxy* selSource)
  {
    Q_ASSERT(selSource);
    if (selSource->GetProperty("ElementType") != nullptr)
    {
      return vtkSMPropertyHelper(selSource, "ElementType").GetAsInt();
    }
    else
    {
      return vtkSelectionNode::ConvertSelectionFieldToAttributeType(
        vtkSMPropertyHelper(selSource, "FieldType").GetAsInt());
    }
  }
};

//-----------------------------------------------------------------------------
void pqFindDataWidget::pqInternals::findData() const
{
  const QScopedValueRollback<bool> rollback(this->InFindData, true);

  Q_ASSERT(this->ProxyWidget != nullptr);
  auto helper = this->ProxyWidget->proxy();

  vtkSMPropertyHelper inputHelper(helper, "Input");
  auto producer = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy(0));
  auto port = inputHelper.GetOutputPort(0);
  Q_ASSERT(producer != nullptr); // ensured by enabled-state of buttons.

  auto selManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SELECTION_MANAGER"));

  vtkSMPropertyHelper queryHelper(helper, "QueryString");
  if (QString(queryHelper.GetAsString(0)).trimmed().isEmpty())
  {
    // no query string specified, clear selection.
    producer->SetSelectionInput(port, nullptr, 0);
    if (selManager)
    {
      selManager->select(nullptr);
    }
    return;
  }

  auto pxm = helper->GetSessionProxyManager();
  Q_ASSERT(pxm != nullptr);

  vtkSmartPointer<vtkSMSourceProxy> selectionSource;
  selectionSource.TakeReference(
    vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "SelectionQuerySource")));
  if (!selectionSource)
  {
    qCritical("Failed to create 'SelectionQuerySource' proxy. "
              "Cannot create selection.");
    return;
  }

  // We've intentionally named the properties on the `FindDataHelper` proxy
  // and "SelectionQuerySource" similarly, so we can use 'vtkSMProxy::Copy'.
  selectionSource->Copy(helper);
  selectionSource->UpdateVTKObjects();
  // create a new append Selections filter and append the selection source
  vtkSmartPointer<vtkSMSourceProxy> appendSelections;
  appendSelections.TakeReference(vtkSMSourceProxy::SafeDownCast(
    vtkSMSelectionHelper::NewAppendSelectionsFromSelectionSource(selectionSource)));
  // set selection input
  producer->SetSelectionInput(port, appendSelections, 0);

  auto fieldType =
    vtkSelectionNode::GetFieldTypeAsString(vtkSelectionNode::ConvertAttributeTypeToSelectionField(
      vtkSMPropertyHelper(selectionSource, "ElementType").GetAsInt()));

  SM_SCOPED_TRACE(CallFunction)
    .arg("QuerySelect")
    .arg("QueryString", vtkSMPropertyHelper(selectionSource, "QueryString").GetAsString())
    .arg("FieldType", fieldType)
    .arg("InsideOut", vtkSMPropertyHelper(selectionSource, "InsideOut").GetAsInt())
    .arg("comment", "create a query selection");

  // ugliness with selection manager -- need a better way of doing this!
  if (selManager)
  {
    auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
    auto pqproxy = smmodel->findItem<pqPipelineSource*>(producer);
    Q_ASSERT(pqproxy);
    selManager->select(pqproxy->getOutputPort(port));
  }
}

//-----------------------------------------------------------------------------
void pqFindDataWidget::pqInternals::freeze() const
{
  auto selManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SELECTION_MANAGER"));
  Q_ASSERT(selManager != nullptr);

  auto port = selManager->getSelectedPort();
  Q_ASSERT(port != nullptr);

  auto appendSelections = vtkSMSourceProxy::SafeDownCast(port->getSelectionInput());
  if (!appendSelections)
  {
    return;
  }

  if (port->getServer()->isRemote())
  {
    unsigned int numInputs = vtkSMPropertyHelper(appendSelections, "Input").GetNumberOfElements();
    for (unsigned int i = 0; i < numInputs; ++i)
    {
      auto selectionSource = vtkSMSourceProxy::SafeDownCast(
        vtkSMPropertyHelper(appendSelections, "Input").GetAsProxy(i));
      // BUG: 6783. Warn user when converting a Frustum|Threshold|Query selection to
      // an id based selection.
      if (strcmp(selectionSource->GetXMLName(), "FrustumSelectionSource") == 0 ||
        strcmp(selectionSource->GetXMLName(), "ThresholdSelectionSource") == 0 ||
        strcmp(selectionSource->GetXMLName(), "SelectionQuerySource") == 0)
      {
        // We need to determine how many ids are present approximately.
        auto dataSource = vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy());
        auto selInfo = dataSource->GetSelectionOutput(port->getPortNumber())->GetDataInformation();
        const auto elemType = pqInternals::selectedElementType(selectionSource);
        if (selInfo->GetNumberOfElements(elemType) > 10000)
        {
          auto userAnswer =
            QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Convert Selection"),
              tr("This selection conversion can potentially result in fetching a "
                 "large amount of data to the client.\n"
                 "Are you sure you want to continue?"),
              QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
          if (userAnswer == QMessageBox::Ok)
          {
            // Showing the warning once is enough
            break;
          }
          else
          {
            return;
          }
        }
      }
    }
  }

  // Convert selection input to indices based selection
  bool selectionChanged;
  vtkSmartPointer<vtkSMSourceProxy> newAppendSelections;
  newAppendSelections.TakeReference(vtkSMSourceProxy::SafeDownCast(
    vtkSMSelectionHelper::ConvertAppendSelections(vtkSelectionNode::INDICES, appendSelections,
      vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy()), port->getPortNumber(),
      selectionChanged)));
  if (selectionChanged)
  {
    port->setSelectionInput(newAppendSelections, 0);
  }
}

//-----------------------------------------------------------------------------
void pqFindDataWidget::pqInternals::extract() const
{
  auto selManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SELECTION_MANAGER"));
  Q_ASSERT(selManager != nullptr);

  auto port = selManager->getSelectedPort();
  Q_ASSERT(port != nullptr);

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "ExtractSelection", port->getSource(), port->getPortNumber());
}

//-----------------------------------------------------------------------------
void pqFindDataWidget::pqInternals::plotOverTime() const
{
  auto selManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SELECTION_MANAGER"));
  Q_ASSERT(selManager != nullptr);

  auto port = selManager->getSelectedPort();
  Q_ASSERT(port != nullptr);

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter(
    "filters", "ExtractSelectionOverTime", port->getSource(), port->getPortNumber());
}

//=============================================================================
pqFindDataWidget::pqFindDataWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqFindDataWidget::pqInternals())
{
  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);
  internals.Ui.verticalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
  internals.Ui.verticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

  // change the findData button palette so it is green when it is enabled.
  // pqCoreUtilities::initializeClickMeButton(internals.Ui.findData);

  // hookup expander buttons
  QObject::connect(
    internals.Ui.createExpander, &pqExpanderButton::toggled, [&internals](bool checked) {
      internals.Ui.container->setVisible(checked);
      internals.Ui.findData->setVisible(checked);
      internals.Ui.reset->setVisible(checked);
      internals.Ui.clear->setVisible(checked);
    });

  QObject::connect(internals.Ui.informationExpander, &pqExpanderButton::toggled,
    internals.Ui.informationContainer, &QWidget::setVisible);
  QObject::connect(internals.Ui.selectionDisplayExpander, &pqExpanderButton::toggled,
    internals.Ui.selectionDisplayProperties, &QWidget::setVisible);

  // hookup findData / reset / clear
  QObject::connect(internals.Ui.reset, &QAbstractButton::clicked, [&internals](bool) {
    if (internals.ProxyWidget)
    {
      internals.ProxyWidget->reset();
    }
    internals.Ui.reset->setEnabled(false);
    internals.Ui.findData->setEnabled(false);
  });

  QObject::connect(internals.Ui.clear, &QAbstractButton::clicked, [&internals](bool) {
    if (internals.ProxyWidget)
    {
      internals.ProxyWidget->proxy()->ResetPropertiesToDefault(vtkSMProxy::ONLY_XML);
      internals.ProxyWidget->reset();
    }
    internals.Ui.reset->setEnabled(false);
    internals.Ui.findData->setEnabled(false);
    internals.Ui.clear->setEnabled(false);
  });

  QObject::connect(internals.Ui.findData, &QAbstractButton::clicked, [&internals](bool) {
    if (internals.ProxyWidget)
    {
      internals.ProxyWidget->apply();
      internals.findData();
    }
    internals.Ui.reset->setEnabled(false);
    // keep findData enabled, there's no reason the user can't hit the button
    // again to generate a new selection with same property values. We should
    // allow for it since the data may have changed.
    // internals.Ui.findData->setEnabled(false);
  });

  QObject::connect(
    internals.Ui.freeze, &QAbstractButton::clicked, [&internals]() { internals.freeze(); });
  QObject::connect(
    internals.Ui.extract, &QAbstractButton::clicked, [&internals]() { internals.extract(); });
  QObject::connect(internals.Ui.plotOverTime, &QAbstractButton::clicked,
    [&internals]() { internals.plotOverTime(); });

  QObject::connect(internals.Ui.informationContainer, &pqFindDataCurrentSelectionFrame::showing,
    [&internals](pqOutputPort* port) {
      if (port)
      {
        internals.Ui.informationExpander->setText(
          QString("Selected Data (%1)").arg(port->prettyName()));
      }
      else
      {
        internals.Ui.informationExpander->setText("Selected Data (none)");
      }
    });

  // if source is remove, ensure are not using it either as checked or unchecked
  // input to the helper.
  auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    smmodel, &pqServerManagerModel::sourceRemoved, this, &pqFindDataWidget::aboutToRemove);

  this->connect(&pqActiveObjects::instance(), &pqActiveObjects::serverChanged, this,
    &pqFindDataWidget::setServer);

  // if selection changes, clear the find data parameters to avoid confusion,
  // unless the selection was made using Find Data.
  auto selManager =
    qobject_cast<pqSelectionManager*>(pqApplicationCore::instance()->manager("SELECTION_MANAGER"));
  if (selManager)
  {
    QObject::connect(selManager, &pqSelectionManager::selectionChanged, this,
      &pqFindDataWidget::handleSelectionChanged);
  }
}

//-----------------------------------------------------------------------------
pqFindDataWidget::~pqFindDataWidget() = default;

//-----------------------------------------------------------------------------
pqServer* pqFindDataWidget::server() const
{
  const auto& internals = (*this->Internals);
  return internals.Server;
}

//-----------------------------------------------------------------------------
void pqFindDataWidget::setServer(pqServer* aserver)
{
  auto& internals = (*this->Internals);
  if (aserver == internals.Server)
  {
    return;
  }

  internals.Server = aserver;
  delete internals.ProxyWidget;

  // disable all buttons.
  internals.Ui.reset->setEnabled(false);
  internals.Ui.findData->setEnabled(false);
  internals.Ui.clear->setEnabled(false);

  if (!aserver)
  {
    return;
  }

  // setup ProxyWidget for selection creation.
  auto pxm = aserver->proxyManager();
  auto proxy = pxm->NewProxy("misc", "FindDataHelper");
  Q_ASSERT(proxy != nullptr);

  internals.ProxyWidget = new pqProxyWidget(proxy);
  internals.ProxyWidget->updatePanel();
  internals.ProxyWidget->setApplyChangesImmediately(false);
  internals.Ui.container->layout()->addWidget(internals.ProxyWidget);
  proxy->FastDelete();

  // enable buttons on modification.
  QObject::connect(
    internals.ProxyWidget.data(), &pqProxyWidget::changeAvailable, [&internals, proxy]() {
      vtkSMUncheckedPropertyHelper inputHelper(proxy, "Input");
      const bool hasInput = (inputHelper.GetAsProxy(0) != nullptr);
      vtkSMUncheckedPropertyHelper queryStringHelper(proxy, "QueryString");
      const bool hasQueryString =
        (queryStringHelper.GetAsString() != nullptr ? strlen(queryStringHelper.GetAsString()) : 0) >
        0;
      internals.Ui.findData->setEnabled(hasInput && hasQueryString);

      internals.Ui.reset->setEnabled(true);
      internals.Ui.clear->setEnabled(true);
    });
}

//-----------------------------------------------------------------------------
void pqFindDataWidget::aboutToRemove(pqPipelineSource* source)
{
  auto& internals = (*this->Internals);
  if (auto helper = internals.ProxyWidget ? internals.ProxyWidget->proxy() : nullptr)
  {
    vtkSMPropertyHelper inputHelper(helper, "Input");
    if (vtkSMPropertyHelper(helper, "Input").GetAsProxy(0) == source->getProxy() ||
      vtkSMUncheckedPropertyHelper(helper, "Input").GetAsProxy(0) == source->getProxy())
    {
      internals.Ui.clear->click();
    }
  }
}

//-----------------------------------------------------------------------------
void pqFindDataWidget::handleSelectionChanged(pqOutputPort* port)
{
  auto& internals = (*this->Internals);
  internals.handleSelectionChanged(port != nullptr);
}
