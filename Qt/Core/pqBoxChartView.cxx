// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBoxChartView.h"

#include "pqDataRepresentation.h"
#include "pqRepresentation.h"
#include "pqSMAdaptor.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMProperty.h"

//-----------------------------------------------------------------------------
pqBoxChartView::pqBoxChartView(const QString& group, const QString& name,
  vtkSMContextViewProxy* viewModule, pqServer* server, QObject* p /*=nullptr*/)
  : Superclass(chartViewType(), group, name, viewModule, server, p)
{
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)), this,
    SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)), this,
    SLOT(onRemoveRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)), this,
    SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));
}

//-----------------------------------------------------------------------------
pqBoxChartView::~pqBoxChartView() = default;

//-----------------------------------------------------------------------------
void pqBoxChartView::onAddRepresentation(pqRepresentation* repr)
{
  this->updateRepresentationVisibility(repr, repr->isVisible());
}

//-----------------------------------------------------------------------------
void pqBoxChartView::onRemoveRepresentation(pqRepresentation*) {}

//-----------------------------------------------------------------------------
void pqBoxChartView::updateRepresentationVisibility(pqRepresentation* repr, bool visible)
{
  if (!visible && repr)
  {
    Q_EMIT this->showing(nullptr);
  }

  if (!visible || !repr)
  {
    return;
  }

  // If visible, turn-off visibility of all other representations.
  QList<pqRepresentation*> reprs = this->getRepresentations();
  Q_FOREACH (pqRepresentation* cur_repr, reprs)
  {
    if (cur_repr != repr)
    {
      cur_repr->setVisible(false);
    }
  }

  pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr);
  Q_EMIT this->showing(dataRepr);
}
