// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTriggerOnIdleHelper_h
#define pqTriggerOnIdleHelper_h

#include <QObject>
#include <QPointer>

#include "pqComponentsModule.h"
#include "pqTimer.h"

class pqServer;

/**
 * Often times we need to call certain slots on idle. However, just relying on
 * Qt's idle is not safe since progress events often result in processing of
 * pending events, which may not be a safe place for such slots to be called.
 * In that such cases this class can be used. It emits the triggered() signal
 * when the server (set using setServer()) is indeed idle. Otherwise it simply
 * reschedules the triggering of another time.
 */
class PQCOMPONENTS_EXPORT pqTriggerOnIdleHelper : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqTriggerOnIdleHelper(QObject* parent = nullptr);
  ~pqTriggerOnIdleHelper() override;

  /**
   * get the server
   */
  pqServer* server() const;

Q_SIGNALS:
  void triggered();

public Q_SLOTS:
  void setServer(pqServer*);
  void trigger();

protected Q_SLOTS:
  void triggerInternal();

private:
  Q_DISABLE_COPY(pqTriggerOnIdleHelper)

  QPointer<pqServer> Server;
  pqTimer Timer;
};

#endif
