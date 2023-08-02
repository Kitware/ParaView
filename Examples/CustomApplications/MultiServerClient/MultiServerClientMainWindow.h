// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef MultiServerClientMainWindow_h
#define MultiServerClientMainWindow_h

#include <QMainWindow>

class pqServer;
class pqPipelineBrowserWidget;
class QComboBox;

class MultiServerClientMainWindow : public QMainWindow
{
  Q_OBJECT
  typedef QMainWindow Superclass;

public:
  MultiServerClientMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  ~MultiServerClientMainWindow();

protected Q_SLOTS:
  void addServerInFiltering(pqServer*);
  void applyPipelineFiltering(int);
  void applyPipelineFiltering2(int);

private:
  Q_DISABLE_COPY(MultiServerClientMainWindow)

  pqPipelineBrowserWidget* pipelineBrowser;
  QComboBox* comboBox;
  pqPipelineBrowserWidget* pipelineBrowser2;
  QComboBox* comboBox2;
};
#endif
