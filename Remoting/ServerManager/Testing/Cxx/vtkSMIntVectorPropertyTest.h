// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMIntVectorPropertyTest_h
#define vtkSMIntVectorPropertyTest_h

#include <QtTest>

class vtkSMIntVectorPropertyTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void SetNumberOfElements();
  void SetElement();
  void SetElements();
  void Copy();
};

#endif
