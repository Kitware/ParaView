// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "CPythonAdaptorAPI.h"
#include "vtkCPProcessor.h"
#include "vtkCPPythonAdaptorAPI.h"
#include "vtkCPPythonPipeline.h"

void coprocessorinitializewithpython(char* pythonFileName, int* pythonFileNameLength)
{
  vtkCPPythonAdaptorAPI::CoProcessorInitialize(nullptr);
  if (pythonFileName != nullptr || *pythonFileNameLength > 0)
  { // we put in a check here so that we avoid the warning below
    coprocessoraddpythonscript(pythonFileName, pythonFileNameLength);
  }
}

void coprocessoraddpythonscript(char* pythonFileName, int* pythonFileNameLength)
{
  if (pythonFileName == nullptr || *pythonFileNameLength == 0)
  {
    vtkGenericWarningMacro("Bad Python file name or length.");
    return;
  }
  int length = *pythonFileNameLength;

  char* cPythonFileName = new char[length + 1];
  memcpy(cPythonFileName, pythonFileName, sizeof(char) * length);
  cPythonFileName[length] = 0;

  if (auto pipeline = vtkCPPythonPipeline::CreateAndInitializePipeline(cPythonFileName))
  {
    vtkCPPythonAdaptorAPI::GetCoProcessor()->AddPipeline(pipeline);
  }
}
