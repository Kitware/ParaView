// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef myMainWindow_h
#define myMainWindow_h

#include <QMainWindow>

class myMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  myMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~myMainWindow();

protected Q_SLOTS:

private:
  Q_DISABLE_COPY(myMainWindow)
};

#endif
