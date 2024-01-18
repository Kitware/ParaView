// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonQtPlugin_h
#define pqPythonQtPlugin_h

#include <QObject>

class pqPythonQtPlugin : public QObject
{
  Q_OBJECT
public:
  pqPythonQtPlugin(QWidget* p = nullptr);
  ~pqPythonQtPlugin() override;

  virtual void startup();
  virtual void shutdown();

protected Q_SLOTS:

  void initialize();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
