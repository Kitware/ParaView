// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkSMProxyLinkTest_h
#define vtkSMProxyLinkTest_h

#include <QtTest>

class vtkSMProxyLinkTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void LinkProxies();
  void AddLinkedProxy();
  void AddException();
};

#endif
