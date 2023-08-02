// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDefaultMainWindow_h
#define pqDefaultMainWindow_h

#include "pqApplicationComponentsModule.h"
#include <QMainWindow>
class PQAPPLICATIONCOMPONENTS_EXPORT pqDefaultMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  pqDefaultMainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags{});
  ~pqDefaultMainWindow() override;

private:
  Q_DISABLE_COPY(pqDefaultMainWindow)

  class pqInternals;
  pqInternals* Internals;
};

#endif
