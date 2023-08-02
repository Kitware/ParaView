// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxyEditorPropertyWidget.h"

#include "pqPropertiesPanel.h"
#include "pqPropertyLinks.h"
#include "pqProxyWidgetDialog.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqProxyEditorPropertyWidget::pqProxyEditorPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  this->setShowLabel(false);

  QPushButton* button = new QPushButton(tr("Edit"), this);
  button->setObjectName("PushButton");
  this->connect(button, SIGNAL(clicked()), SLOT(buttonClicked()));
  button->setEnabled(false);
  this->Button = button;

  // If ProxyEditorPropertyWidget hints are present, we'll add a checkbox to
  // control that property on the "other" proxy.
  if (vtkPVXMLElement* hints = smproperty->GetHints()
      ? smproperty->GetHints()->FindNestedElementByName("ProxyEditorPropertyWidget")
      : nullptr)
  {
    this->Checkbox = new QCheckBox(this);
    this->Checkbox->setText(
      QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel()));
    this->Checkbox->setEnabled(false);
    this->Checkbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->PropertyName = hints->GetAttributeOrDefault("property", "");
    this->connect(&this->links(), SIGNAL(qtWidgetChanged()), this, SIGNAL(changeAvailable()));
  }

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  if (this->Checkbox)
  {
    hbox->addWidget(this->Checkbox);
  }
  hbox->addWidget(button);

  this->addPropertyLink(this, "proxyToEdit", SIGNAL(dummySignal()), smproperty);
}

//-----------------------------------------------------------------------------
pqProxyEditorPropertyWidget::~pqProxyEditorPropertyWidget()
{
  delete this->Editor;
}

//-----------------------------------------------------------------------------
void pqProxyEditorPropertyWidget::setProxyToEdit(pqSMProxy smproxy)
{
  if (this->ProxyToEdit && this->Checkbox &&
    this->ProxyToEdit->GetProperty(this->PropertyName.toStdString().c_str()))
  {
    this->links().removePropertyLink(this->Checkbox, "checked", SIGNAL(toggled(bool)),
      this->ProxyToEdit, this->ProxyToEdit->GetProperty(this->PropertyName.toStdString().c_str()));
  }
  this->ProxyToEdit = smproxy;
  this->Button->setEnabled(smproxy != nullptr);
  if (this->Editor && this->Editor->proxy() != smproxy)
  {
    delete this->Editor;
  }

  if (this->Checkbox)
  {
    if (vtkSMProperty* smproperty =
          smproxy ? smproxy->GetProperty(this->PropertyName.toStdString().c_str()) : nullptr)
    {
      this->Checkbox->setEnabled(true);
      this->Checkbox->setToolTip(pqPropertyWidget::getTooltip(smproperty));
      this->links().addPropertyLink(
        this->Checkbox, "checked", SIGNAL(toggled(bool)), smproxy, smproperty);
    }
    else
    {
      this->Checkbox->setEnabled(false);
    }
  }
}

//-----------------------------------------------------------------------------
pqSMProxy pqProxyEditorPropertyWidget::proxyToEdit() const
{
  return pqSMProxy(this->ProxyToEdit.GetPointer());
}

//-----------------------------------------------------------------------------
void pqProxyEditorPropertyWidget::buttonClicked()
{
  if (!this->ProxyToEdit.GetPointer())
  {
    qCritical() << "No proxy to edit!";
    return;
  }

  if (this->Editor == nullptr)
  {
    this->Editor = new pqProxyWidgetDialog(this->ProxyToEdit.GetPointer(), this);
    this->Editor->setEnableSearchBar(this->Editor->hasAdvancedProperties());
    this->Editor->setSettingsKey(QString("pqProxyEditorPropertyWidget.%1.%2")
                                   .arg(this->proxy()->GetXMLName())
                                   .arg(this->property()->GetXMLName()));
    this->connect(this->Editor, SIGNAL(accepted()), SIGNAL(changeAvailable()));
  }
  this->Editor->setWindowTitle(this->Button->text());
  this->Editor->setObjectName("EditProxy");
  this->Editor->show();
  this->Editor->raise();
}
