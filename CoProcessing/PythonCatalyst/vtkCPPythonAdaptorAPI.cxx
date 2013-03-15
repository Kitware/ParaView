/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPPythonAdaptorAPI.h"

#include "vtkCPPythonProcessor.h"
#include "vtkCPPythonScriptPipeline.h"

//----------------------------------------------------------------------------
void vtkCPPythonAdaptorAPI::CoProcessorInitialize(const char* pythonFileName)
{
  if (!Superclass::CoProcessor)
    {
    Superclass::CoProcessor = vtkCPPythonProcessor::New();
    Superclass::CoProcessor->Initialize();

    vtkCPPythonScriptPipeline* pipeline = vtkCPPythonScriptPipeline::New();
    pipeline->Initialize(pythonFileName);

    Superclass::CoProcessor->AddPipeline(pipeline);
    pipeline->Delete();
    }

  Superclass::CoProcessorInitialize();
}
