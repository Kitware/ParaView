// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCrashRecoveryBehavior_h
#define pqCrashRecoveryBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QTimer>

class pqServer;

/**
 * @ingroup Behaviors
 * pqCrashRecoveryBehavior manages saving/loading of crash recovery state. If
 * you want your application to be able to recover from crashes, simply
 * instantiate this behavior and ensure that state saving/loading works
 * correctly.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCrashRecoveryBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCrashRecoveryBehavior(QObject* parent = nullptr);
  ~pqCrashRecoveryBehavior() override;

protected Q_SLOTS:
  void delayedSaveRecoveryState();
  void saveRecoveryState();
  void onServerAdded(pqServer* server);
  void onServerDisconnect();

private:
  Q_DISABLE_COPY(pqCrashRecoveryBehavior)

  QTimer Timer;
};

#endif
