// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMDoubleVectorPropertyTest_h
#define vtkSMDoubleVectorPropertyTest_h

#include <QtTest>

class vtkSMDoubleVectorPropertyTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void SetNumberOfElements();
  void SetElement();
  void SetElements();
  void Copy();
};

#endif
