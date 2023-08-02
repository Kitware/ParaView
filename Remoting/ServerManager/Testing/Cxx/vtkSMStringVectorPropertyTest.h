// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMStringVectorPropertyTest_h
#define vtkSMStringVectorPropertyTest_h

#include <QtTest>

class vtkSMStringVectorPropertyTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void SetNumberOfElements();
  void SetElement();
  void SetElements();
  void Copy();
};

#endif
