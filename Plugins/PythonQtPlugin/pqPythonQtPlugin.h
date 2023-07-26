// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqPythonQtPlugin_h
#define pqPythonQtPlugin_h

#include <QObject>

class pqPythonQtPlugin : public QObject
{
  Q_OBJECT
public:
  pqPythonQtPlugin(QWidget* p = 0);
  virtual ~pqPythonQtPlugin();

  virtual void startup();
  virtual void shutdown();

protected Q_SLOTS:

  void initialize();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
