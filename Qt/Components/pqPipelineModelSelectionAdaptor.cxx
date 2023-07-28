// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPipelineModelSelectionAdaptor.h"

// Qt includes.
#include <QItemSelectionModel>
#include <QtDebug>

// ParaView includes.
#include "pqPipelineModel.h"

//-----------------------------------------------------------------------------
pqPipelineModelSelectionAdaptor::pqPipelineModelSelectionAdaptor(
  QItemSelectionModel* pipelineSelectionModel)
  : pqSelectionAdaptor(pipelineSelectionModel)
{
  if (!qobject_cast<const pqPipelineModel*>(this->getQModel()))
  {
    qDebug() << "QItemSelectionModel must be a selection model for "
                " pqPipelineModel.";
    return;
  }
}

//-----------------------------------------------------------------------------
pqPipelineModelSelectionAdaptor::~pqPipelineModelSelectionAdaptor() = default;

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModelSelectionAdaptor::mapFromItem(pqServerManagerModelItem* item) const
{
  const pqPipelineModel* pM = qobject_cast<const pqPipelineModel*>(this->getQModel());
  return pM->getIndexFor(item);
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqPipelineModelSelectionAdaptor::mapToItem(const QModelIndex& index) const
{
  const pqPipelineModel* pM = qobject_cast<const pqPipelineModel*>(this->getQModel());
  return pM->getItemFor(index);
}
