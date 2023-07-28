// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * An example of a very simple ParaView based application.
 * It still contains all of ParaView features, but only a subset of features
 * is shown to the user.
 *
 * It shows how to :
 * * Only expose a handful of filters
 * * Define your own menus and use only specific menus
 * * Choose which toolbar and dockwidget to show
 */

#ifndef myMainWindow_h
#define myMainWindow_h

#include <QMainWindow>
#include <QScopedPointer>

class myMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  myMainWindow();
  ~myMainWindow() override;

private:
  Q_DISABLE_COPY(myMainWindow)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
