// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef myMainWindow_h
#define myMainWindow_h

#include <QMainWindow>

// MainWindow for the default ParaView application.
class myMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  myMainWindow();
  ~myMainWindow();

protected Q_SLOTS:
  void showHelpForProxy(const QString& groupname, const QString& proxyname);

private:
  Q_DISABLE_COPY(myMainWindow)

  class pqInternals;
  pqInternals* Internals;
};

#endif
