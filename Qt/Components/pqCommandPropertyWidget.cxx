// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCommandPropertyWidget.h"

#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqCommandPropertyWidget::pqCommandPropertyWidget(
  vtkSMProperty* smproperty, vtkSMProxy* smproxy, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  if (!smproperty)
  {
    qCritical() << "pqCommandPropertyWidget cannot be instantiated without a vtkSMProperty";
    return;
  }

  QPushButton* button = new QPushButton(
    QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel()), this);
  button->setObjectName("PushButton");
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(buttonClicked()));

  QHBoxLayout* layoutLocal = new QHBoxLayout(this);
  layoutLocal->setContentsMargins(0, 0, 0, 0);
  layoutLocal->addWidget(button);
  layoutLocal->addStretch();
  this->setShowLabel(false);
}

//-----------------------------------------------------------------------------
pqCommandPropertyWidget::~pqCommandPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqCommandPropertyWidget::buttonClicked()
{
  vtkSMProxy* smproxy = this->proxy();
  vtkSMProperty* smproperty = this->property();
  if (smproperty != nullptr && smproxy != nullptr)
  {
    const char* pname = smproxy->GetPropertyName(smproperty);
    if (pname)
    {
      smproxy->InvokeCommand(pname);

      // Update pipeline information, if possible, otherwise, simple update
      // information properties.
      vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(smproxy);
      if (source)
      {
        source->UpdatePipelineInformation();
      }
      else
      {
        smproxy->UpdatePropertyInformation();
      }
    }
  }
}
