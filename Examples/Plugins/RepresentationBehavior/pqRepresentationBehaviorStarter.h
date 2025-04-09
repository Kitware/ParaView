// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRepresentationBehaviorStarter_h
#define pqRepresentationBehaviorStarter_h

#include <QObject>

class pqRepresentationBehaviorStarter : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqRepresentationBehaviorStarter(QObject* parent = nullptr);
  ~pqRepresentationBehaviorStarter();

  // Callback for shutdown.
  void onShutdown();

  // Callback for startup.
  void onStartup();

private:
  Q_DISABLE_COPY(pqRepresentationBehaviorStarter)
};

#endif
