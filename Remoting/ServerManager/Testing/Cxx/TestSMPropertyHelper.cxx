// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMPropertyHelperTest.h"

#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"

extern int TestSMPropertyHelper(int argc, char* argv[])
{
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT);
  vtkSMPropertyHelperTest test;
  int ret = QTest::qExec(&test, argc, argv);
  vtkInitializationHelper::Finalize();
  return ret;
}
