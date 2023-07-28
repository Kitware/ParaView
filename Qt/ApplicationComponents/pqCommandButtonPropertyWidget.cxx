// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqCommandButtonPropertyWidget.h"

#include "pqPVApplicationCore.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

#include <QCoreApplication>
#include <QPushButton>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqCommandButtonPropertyWidget::pqCommandButtonPropertyWidget(
  vtkSMProxy* smProxy, vtkSMProperty* proxyProperty, QWidget* pWidget)
  : pqPropertyWidget(smProxy, pWidget)
  , Property(proxyProperty)
{
  QVBoxLayout* l = new QVBoxLayout;
  l->setSpacing(0);
  l->setContentsMargins(0, 0, 0, 0);

  QPushButton* button =
    new QPushButton(QCoreApplication::translate("ServerManagerXML", proxyProperty->GetXMLLabel()));
  connect(button, SIGNAL(clicked()), this, SLOT(buttonClicked()));
  l->addWidget(button);

  this->setShowLabel(false);
  this->setLayout(l);
}

//-----------------------------------------------------------------------------
pqCommandButtonPropertyWidget::~pqCommandButtonPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqCommandButtonPropertyWidget::buttonClicked()
{
  auto aproxy = this->proxy();
  if (this->Property->GetCommand() != nullptr)
  {
    const char* pname = aproxy->GetPropertyName(this->Property);
    SM_SCOPED_TRACE(CallMethod).arg("proxy", aproxy).arg("InvokeCommand").arg(pname);
    this->proxy()->InvokeCommand(pname);
  }
  else
  {
    // if property has no command, then we simply recreate the proxies.
    SM_SCOPED_TRACE(CallMethod).arg("proxy", aproxy).arg("RecreateVTKObjects");
    this->proxy()->RecreateVTKObjects();
  }

  // Trigger a rendering
  pqPVApplicationCore::instance()->render();
}
