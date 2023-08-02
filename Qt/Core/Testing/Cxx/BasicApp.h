// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqRenderView.h"
#include "vtkObject.h"
#include <QMainWindow>
#include <QPointer>

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainWindow();
  bool compareView(const QString& referenceImage, double threshold, const QString& tempDirectory);
  QPointer<pqRenderView> RenderView;
public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void processTest();
};
