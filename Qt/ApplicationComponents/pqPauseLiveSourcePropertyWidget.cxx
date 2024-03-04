// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPauseLiveSourcePropertyWidget.h"

#include "pqLiveSourceItem.h"
#include "pqLiveSourceManager.h"
#include "pqPVApplicationCore.h"
#include "vtkPVXMLElement.h"

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

  QObject::connect(
    button, &QPushButton::clicked, this, &pqPauseLiveSourcePropertyWidget::onClicked);

  pqLiveSourceManager* lvManager = pqPVApplicationCore::instance()->liveSourceManager();
  pqLiveSourceItem* lvItem = lvManager->getLiveSourceItem(smproxy);
  QObject::connect(lvItem, &pqLiveSourceItem::stateChanged, button, &QPushButton::setChecked);
  button->setChecked(lvItem->isPaused());
}

//-----------------------------------------------------------------------------
pqPauseLiveSourcePropertyWidget::~pqPauseLiveSourcePropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqPauseLiveSourcePropertyWidget::onClicked(bool checked)
{
  pqLiveSourceManager* manager = pqPVApplicationCore::instance()->liveSourceManager();
  pqLiveSourceItem* lvItem = manager->getLiveSourceItem(this->proxy());
  lvItem->blockSignals(true);
  if (checked)
  {
    lvItem->pause();
  }
  else
  {
    lvItem->resume();
  }
  lvItem->blockSignals(false);
}
