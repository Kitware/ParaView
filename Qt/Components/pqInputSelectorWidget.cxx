/*=========================================================================

   Program: ParaView
   Module:  pqInputSelectorWidget.cxx

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
  l->setMargin(pqPropertiesPanel::suggestedMargin());
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
      auto data = this->ComboBox->itemData(index);
      auto ptr = reinterpret_cast<vtkSMProxy*>(data.value<void*>());
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

  auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
  auto smproxy = this->proxy();
  auto smproperty = this->property();

  Q_ASSERT(smproperty && smproxy && smmodel);

  auto server = smmodel->findServer(smproxy->GetSession());
  Q_ASSERT(server);

  auto pxm = server->proxyManager();
  auto prototype = pxm->GetPrototypeProxy(smproxy->GetXMLGroup(), smproxy->GetXMLName());
  Q_ASSERT(prototype);

  auto protoProperty = prototype->GetProperty(smproxy->GetPropertyName(smproperty));
  Q_ASSERT(protoProperty);

  vtkSMPropertyHelper helper(protoProperty);

  this->ComboBox->clear();
  this->ComboBox->addItem("(none)", QVariant::fromValue<void*>(nullptr));

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
