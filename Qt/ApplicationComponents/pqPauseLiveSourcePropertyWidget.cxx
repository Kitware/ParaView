// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPauseLiveSourcePropertyWidget.h"

#include "pqLiveSourceBehavior.h"

#include <QCoreApplication>
#include <QPushButton>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqPauseLiveSourcePropertyWidget::pqPauseLiveSourcePropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  auto l = new QVBoxLayout(this);
  l->setSpacing(0);
  l->setContentsMargins(0, 0, 0, 0);

  auto button =
    new QPushButton(QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel()));
  button->setCheckable(true);
  l->addWidget(button);

  this->setShowLabel(false);

  QObject::connect(button, &QPushButton::clicked, [button](bool checked) {
    if (checked)
    {
      pqLiveSourceBehavior::pause();
    }
    else
    {
      pqLiveSourceBehavior::resume();
    }
    button->setChecked(pqLiveSourceBehavior::isPaused());
  });
}

//-----------------------------------------------------------------------------
pqPauseLiveSourcePropertyWidget::~pqPauseLiveSourcePropertyWidget() = default;
