// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef SpreadSheetMainWindow_h
#define SpreadSheetMainWindow_h

#include <QMainWindow>
#include <QScopedPointer>

/**
 * An example of a paraview main window showing only data in a spreadsheet.
 */
class SpreadSheetMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  SpreadSheetMainWindow();
  ~SpreadSheetMainWindow() override;

private:
  Q_DISABLE_COPY(SpreadSheetMainWindow)

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
