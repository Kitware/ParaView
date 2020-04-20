/*=========================================================================

Program:   ParaView
Module:    vtkSMPropertyHelperTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMPropertyHelperTest_h
#define vtkSMPropertyHelperTest_h

#include <QtTest>

class vtkSMPropertyHelperTest : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void Set();
};

#endif
