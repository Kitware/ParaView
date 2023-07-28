// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef ParaViewMainWindow_h
#define ParaViewMainWindow_h

#include <QMainWindow>

/// MainWindow for the default ParaView application.
class ParaViewMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  ParaViewMainWindow();
  ~ParaViewMainWindow() override;

protected:
  void dragEnterEvent(QDragEnterEvent* evt) override;
  void dropEvent(QDropEvent* evt) override;
  void showEvent(QShowEvent* evt) override;
  void closeEvent(QCloseEvent* evt) override;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void showHelpForProxy(const QString& groupname, const QString& proxyname);
  void showWelcomeDialog();
  void handleMessage(const QString&, int);
  void updateFontSize();

private:
  Q_DISABLE_COPY(ParaViewMainWindow)

  class pqInternals;
  pqInternals* Internals;
};

#endif
