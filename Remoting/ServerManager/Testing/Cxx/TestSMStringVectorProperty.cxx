// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMStringVectorPropertyTest.h"

#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"

extern int TestSMStringVectorProperty(int argc, char* argv[])
{
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT);
  vtkSMStringVectorPropertyTest test;
  int ret = QTest::qExec(&test, argc, argv);
  vtkInitializationHelper::Finalize();
  return ret;
}
