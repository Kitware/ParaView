// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqDisplayPanel.h"
#include "pqDataRepresentation.h"
#include "pqPipelineSource.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

pqDisplayPanel::pqDisplayPanel(pqRepresentation* display, QWidget* p)
  : QWidget(p)
  , Representation(display)
{
  pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(display);
  if (dataRepr)
  {
    pqPipelineSource* input = dataRepr->getInput();
    QObject::connect(input, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(dataUpdated()));
    this->dataUpdated();
  }
}

pqDisplayPanel::~pqDisplayPanel() = default;

pqRepresentation* pqDisplayPanel::getRepresentation()
{
  return this->Representation;
}

void pqDisplayPanel::reloadGUI() {}

void pqDisplayPanel::dataUpdated() {}

void pqDisplayPanel::updateAllViews()
{
  if (this->Representation)
  {
    this->Representation->renderViewEventually();
  }
}
