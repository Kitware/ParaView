// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef SourceToolbarActions_h
#define SourceToolbarActions_h

#include <QActionGroup>
/// This example illustrates adding a toolbar to ParaView to create a sphere and
/// a cylinder source.
class SourceToolbarActions : public QActionGroup
{
  Q_OBJECT
public:
  SourceToolbarActions(QObject* p);

public Q_SLOTS:
  /// Callback for each action triggered.
  void onAction(QAction* a);
};

#endif
