/*=========================================================================

Program:   ParaView
Module:    vtkSMStringVectorPropertyTest.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
