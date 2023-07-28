// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDataTimeStepBehavior_h
#define pqDataTimeStepBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class pqPipelineSource;

/**
 * @ingroup Behaviors
 * pqDataTimeStepBehavior ensures that whenever a file is opened with more
 * than 1 timestep, the application time >= the time for the last timestep.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqDataTimeStepBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqDataTimeStepBehavior(QObject* parent = nullptr);

protected Q_SLOTS:
  /**
   * called when a reader is created.
   */
  void onReaderCreated(pqPipelineSource* reader);

private:
  Q_DISABLE_COPY(pqDataTimeStepBehavior)
};

#endif
