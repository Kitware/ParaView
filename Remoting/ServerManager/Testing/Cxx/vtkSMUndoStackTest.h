// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMUndoStackTest_h
#define vtkSMUndoStackTest_h

#include <QtTest>

class vtkSMUndoStackTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void UndoRedo();
  void StackDepth();
};

#endif
