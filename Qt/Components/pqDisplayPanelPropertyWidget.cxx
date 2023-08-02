// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqDisplayPanelPropertyWidget.h"

#include <QVBoxLayout>

pqDisplayPanelPropertyWidget::pqDisplayPanelPropertyWidget(
  pqDisplayPanel* panel, QWidget* parentObject)
  : pqPropertyWidget(panel->getRepresentation()->getProxy(), parentObject)
{
  this->DisplayPanel = panel;

  QVBoxLayout* layoutLocal = new QVBoxLayout;
  layoutLocal->setContentsMargins(0, 0, 0, 0);
  layoutLocal->addWidget(panel);
  setLayout(layoutLocal);
}

pqDisplayPanelPropertyWidget::~pqDisplayPanelPropertyWidget() = default;

pqDisplayPanel* pqDisplayPanelPropertyWidget::getDisplayPanel() const
{
  return this->DisplayPanel;
}
