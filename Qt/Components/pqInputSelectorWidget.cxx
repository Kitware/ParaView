// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqInputSelectorWidget.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqPropertyLinksConnection.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <QComboBox>
#include <QDebug>
#include <QVBoxLayout>

namespace
{

class CustomConnection : public pqPropertyLinksConnection
{
public:
  using pqPropertyLinksConnection::pqPropertyLinksConnection;

protected:
  void setServerManagerValue(bool use_unchecked, const QVariant& value) override
  {
    vtkSMPropertyHelper helper(this->propertySM());
    helper.SetUseUnchecked(use_unchecked);

    auto smproxy = value.value<pqSMProxy>();
    if (auto port = vtkSMOutputPort::SafeDownCast(smproxy))
    {
      helper.Set(port->GetSourceProxy(), port->GetPortIndex());
    }
    else
    {
      helper.SetNumberOfElements(0);
    }
  }

  QVariant currentServerManagerValue(bool use_unchecked) const override
  {
    vtkSMPropertyHelper helper(this->propertySM());
    helper.SetUseUnchecked(use_unchecked);
    if (auto proxy = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0)))
    {
      return QVariant::fromValue(pqSMProxy(proxy->GetOutputPort(helper.GetOutputPort(0))));
    }
    return QVariant();
  }
};
}

//-----------------------------------------------------------------------------
pqInputSelectorWidget::pqInputSelectorWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , ComboBox(new QComboBox())
{
  this->setProperty(smproperty);

  auto l = new QVBoxLayout(this);
  l->setContentsMargins(pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin());
  l->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  l->addWidget(this->ComboBox);

  this->UpdateComboBoxTimer.setSingleShot(true);
  this->UpdateComboBoxTimer.setInterval(0);
  this->connect(&this->UpdateComboBoxTimer, SIGNAL(timeout()), SLOT(updateComboBox()));

  /**
   * Populate combo-box when sources change.
   */
  auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
  Q_ASSERT(smmodel != nullptr);
  this->UpdateComboBoxTimer.connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(start()));
  this->UpdateComboBoxTimer.connect(
    smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)), SLOT(start()));
  this->UpdateComboBoxTimer.connect(
    smmodel, SIGNAL(nameChanged(pqServerManagerModelItem*)), SLOT(start()));
  this->UpdateComboBoxTimer.connect(smmodel, SIGNAL(dataUpdated(pqPipelineSource*)), SLOT(start()));
  this->updateComboBox();

  QObject::connect(
    this->ComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
      auto changedData = this->ComboBox->itemData(index);
      auto ptr = reinterpret_cast<vtkSMProxy*>(changedData.value<void*>());
      this->ChosenPort = ptr;
      Q_EMIT this->selectedInputChanged();
    });

  this->links().addPropertyLink<CustomConnection>(
    this, "selectedInput", SIGNAL(selectedInputChanged()), smproxy, smproperty);
}

//-----------------------------------------------------------------------------
pqInputSelectorWidget::~pqInputSelectorWidget() = default;

//-----------------------------------------------------------------------------
void pqInputSelectorWidget::setSelectedInput(pqSMProxy smproxy)
{
  if (this->ChosenPort.GetPointer() != smproxy.GetPointer())
  {
    this->ChosenPort = smproxy.GetPointer();
    this->UpdateComboBoxTimer.start();
  }
}

//-----------------------------------------------------------------------------
pqSMProxy pqInputSelectorWidget::selectedInput() const
{
  return this->ChosenPort.GetPointer();
}

//-----------------------------------------------------------------------------
void pqInputSelectorWidget::updateComboBox()
{
  const QSignalBlocker blocker(this->ComboBox);
  this->ComboBox->clear();
  this->ComboBox->addItem(QString("(%1)").arg(tr("none")), QVariant::fromValue<void*>(nullptr));

  auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
  auto smproxy = this->proxy();
  auto smproperty = this->property();

  Q_ASSERT(smproperty && smproxy && smmodel);

  auto server = smmodel->findServer(smproxy->GetSession());
  if (!server)
  {
    return;
  }

  auto pxm = server->proxyManager();
  auto prototype = pxm->GetPrototypeProxy(smproxy->GetXMLGroup(), smproxy->GetXMLName());
  Q_ASSERT(prototype);

  auto protoProperty = prototype->GetProperty(smproxy->GetPropertyName(smproperty));
  Q_ASSERT(protoProperty);

  vtkSMPropertyHelper helper(protoProperty);

  const auto sources = smmodel->findItems<pqPipelineSource*>(server);
  for (auto& source : sources)
  {
    if (source->getProxy() == smproxy)
    {
      // avoid adding ourself as the input.
      continue;
    }
    for (const auto& port : source->getOutputPorts())
    {
      helper.Set(source->getProxy(), port->getPortNumber());
      if (protoProperty->IsInDomains())
      {
        if (source->getNumberOfOutputPorts() > 1)
        {
          this->ComboBox->addItem(
            QString("%1:%2").arg(source->getSMName()).arg(port->getPortName()),
            QVariant::fromValue<void*>(port->getOutputPortProxy()));
        }
        else
        {
          this->ComboBox->addItem(
            source->getSMName(), QVariant::fromValue<void*>(port->getOutputPortProxy()));
        }
      }
    }
  }

  const int chosenIndex =
    this->ComboBox->findData(QVariant::fromValue<void*>(this->ChosenPort.GetPointer()));
  this->ComboBox->setCurrentIndex(chosenIndex == -1 ? 0 : chosenIndex);
}
