// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMUndoStackTest.h"

#include "vtkInitializationHelper.h"
#include "vtkProcessModule.h"

extern int TestSMUndoStack(int argc, char* argv[])
{
  vtkInitializationHelper::Initialize(argc, argv, vtkProcessModule::PROCESS_CLIENT);
  vtkSMUndoStackTest test;
  int ret = QTest::qExec(&test, argc, argv);
  vtkInitializationHelper::Finalize();
  return ret;
}
