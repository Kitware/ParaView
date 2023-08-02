// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPPythonAdaptorAPI.h"

#include "vtkCPProcessor.h"
#include "vtkCPPythonScriptPipeline.h"

//----------------------------------------------------------------------------
vtkCPPythonAdaptorAPI::vtkCPPythonAdaptorAPI() = default;

//----------------------------------------------------------------------------
vtkCPPythonAdaptorAPI::~vtkCPPythonAdaptorAPI() = default;

//----------------------------------------------------------------------------
void vtkCPPythonAdaptorAPI::CoProcessorInitialize(const char* pythonFileName)
{
  if (!Superclass::CoProcessor)
  {
    Superclass::CoProcessor = vtkCPProcessor::New();
    Superclass::CoProcessor->Initialize();
  }
  // needed to initialize vtkCPDataDescription.
  Superclass::CoProcessorInitialize();

  if (pythonFileName)
  {
    vtkCPPythonScriptPipeline* pipeline = vtkCPPythonScriptPipeline::New();
    pipeline->Initialize(pythonFileName);
    Superclass::CoProcessor->AddPipeline(pipeline);
    pipeline->FastDelete();
  }
}

//-----------------------------------------------------------------------------
void vtkCPPythonAdaptorAPI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
