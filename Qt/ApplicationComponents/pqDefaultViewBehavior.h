// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDefaultViewBehavior_h
#define pqDefaultViewBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QPointer>

#include "pqTimer.h" // for pqTimer
#include "vtkType.h" // for vtkTypeUInt32.

class pqServer;

/**
 * @ingroup Behaviors
 * pqDefaultViewBehavior ensures that when a new server connection is made,
 * the default view of the user-specified type is automatically created.
 * This also has the logic to warn before server connection timesout.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqDefaultViewBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqDefaultViewBehavior(QObject* parent = nullptr);

protected Q_SLOTS:
  void onServerCreation(pqServer*);
  void fiveMinuteTimeoutWarning();
  void finalTimeoutWarning();

  void showWarnings();

private:
  Q_DISABLE_COPY(pqDefaultViewBehavior)

  vtkTypeUInt32 ServerCapabilities;
  vtkTypeUInt32 ClientCapabilities;
  QPointer<pqServer> Server;
  pqTimer WarningsTimer;
};

#endif
