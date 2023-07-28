// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPipelineModelSelectionAdaptor_h
#define pqPipelineModelSelectionAdaptor_h

#include "pqSelectionAdaptor.h"

/**
 * pqPipelineModelSelectionAdaptor is an adaptor that connects a
 * QItemSelectionModel for a pqPipelineModel to pqActiveObjects.
 */
class PQCOMPONENTS_EXPORT pqPipelineModelSelectionAdaptor : public pqSelectionAdaptor
{
  Q_OBJECT

public:
  pqPipelineModelSelectionAdaptor(QItemSelectionModel* pipelineSelectionModel);
  ~pqPipelineModelSelectionAdaptor() override;

protected:
  QModelIndex mapFromItem(pqServerManagerModelItem*) const override;
  pqServerManagerModelItem* mapToItem(const QModelIndex& index) const override;

private:
  Q_DISABLE_COPY(pqPipelineModelSelectionAdaptor)
};
#endif
