// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqViewTypePropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqServer.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqViewTypePropertyWidget::pqViewTypePropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  this->ComboBox = new QComboBox(this);
  this->ComboBox->setObjectName("ComboBox");
  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->addWidget(this->ComboBox);

  this->ComboBox->addItem(tr("None"), QVariant("None"));
  this->ComboBox->addItem(tr("Empty"), QVariant("Empty"));

  // fill combo-box.
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().activeServer()
    ? pqActiveObjects::instance().activeServer()->proxyManager()
    : nullptr;

  if (pxm)
  {
    vtkPVProxyDefinitionIterator* iter =
      pxm->GetProxyDefinitionManager()->NewSingleGroupIterator("views");
    for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      // add label instead of proxy name to make this more user friendly.
      const char* proxyName = iter->GetProxyName();
      vtkSMProxy* prototype = pxm->GetPrototypeProxy("views", proxyName);
      if (prototype)
      {
        this->ComboBox->addItem(
          QCoreApplication::translate("ServerManagerXML", prototype->GetXMLLabel()),
          iter->GetProxyName());
      }
    }
    iter->Delete();
    this->ComboBox->model()->sort(0, Qt::AscendingOrder);

    this->connect(this->ComboBox, SIGNAL(currentIndexChanged(int)), SIGNAL(valueChanged()));
    this->addPropertyLink(this, "value", SIGNAL(valueChanged()), smproperty);
  }
  else
  {
    this->ComboBox->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
pqViewTypePropertyWidget::~pqViewTypePropertyWidget() = default;

//-----------------------------------------------------------------------------
QString pqViewTypePropertyWidget::value() const
{
  return this->ComboBox->itemData(this->ComboBox->currentIndex()).toString();
}

//-----------------------------------------------------------------------------
void pqViewTypePropertyWidget::setValue(const QString& val)
{
  int index = this->ComboBox->findData(val);
  if (index == -1)
  {
    // add the value being specified to the combo-box.
    index = this->ComboBox->count();
    this->ComboBox->addItem(val, val);
  }
  this->ComboBox->setCurrentIndex(index);
}
