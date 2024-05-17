// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBlockProxyWidget.h"

#include "pqPropertyWidget.h"

#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

//-----------------------------------------------------------------------------------
pqBlockProxyWidget::pqBlockProxyWidget(
  vtkSMProxy* proxy, QString selector, QWidget* parent, Qt::WindowFlags flags)
  : Superclass(proxy, parent, flags)
  , Selector(selector)
{
}
//-----------------------------------------------------------------------------------
pqBlockProxyWidget::~pqBlockProxyWidget() = default;

//-----------------------------------------------------------------------------------
void pqBlockProxyWidget::apply() const
{
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", this->proxy())
    .arg("selector", this->Selector.toUtf8().data());
  this->Superclass::applyInternal();
}

//-----------------------------------------------------------------------------------
void pqBlockProxyWidget::onChangeFinished()
{
  if (this->Superclass::applyChangesImmediately())
  {
    pqPropertyWidget* pqSender = qobject_cast<pqPropertyWidget*>(this->sender());
    if (pqSender)
    {
      SM_SCOPED_TRACE(PropertiesModified)
        .arg("proxy", this->proxy())
        .arg("selector", this->Selector.toUtf8().data());
      pqSender->apply();
    }
  }
  Q_EMIT this->changeFinished();
}
