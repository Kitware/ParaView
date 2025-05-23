// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMyApplicationStarter_h
#define pqMyApplicationStarter_h

#include <QObject>

class pqMyApplicationStarter : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqMyApplicationStarter(QObject* parent = nullptr);
  ~pqMyApplicationStarter();

  // Callback for shutdown.
  void onShutdown();

  // Callback for startup.
  void onStartup();

public Q_SLOTS:
  void playAnimation();

private:
  Q_DISABLE_COPY(pqMyApplicationStarter)
};

#endif
