// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pvserver_common.h"

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Init current process type
  return RealMain(argc, argv, vtkProcessModule::PROCESS_DATA_SERVER);
}
