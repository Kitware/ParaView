// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMProxyTest_h
#define vtkSMProxyTest_h

#include <QtTest>

class vtkSMProxyTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void SetAnnotation();
  void GetProperty();
  void GetVTKClassName();
};

#endif
