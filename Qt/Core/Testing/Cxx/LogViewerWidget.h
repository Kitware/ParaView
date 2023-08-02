// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LogViewerWidget_h
#define LogViewerWidget_h

#include <QObject>

class LogViewerWidgetTester : public QObject
{
  Q_OBJECT;
private Q_SLOTS:
  void basic();
};
#endif
