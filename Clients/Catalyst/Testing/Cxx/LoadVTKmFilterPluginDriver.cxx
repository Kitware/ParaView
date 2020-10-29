/*=========================================================================

  Program:   ParaView
  Module:    LoadVTKmFilterPluginDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkNew.h"
#include "vtkPolyData.h"

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
#include "vtkCPPythonStringPipeline.h"
#endif

static const char* script=R"==(
from paraview.simple import *
LoadDistributedPlugin("VTKmFilters", remote=False, ns=globals())
assert VTKmContour

def DoCoProcessing(datadescription):
    print("in DoCoProcessing")
    print("VTKmFilters plugin has been loaded successfully!")

def RequestDataDescription(datadescription):
    datadescription.GetInputDescriptionByName('input').GenerateMeshOn()
)==";

int LoadVTKmFilterPluginDriver(int, char* argv[])
{
  vtkNew<vtkCPProcessor> processor;
  processor->Initialize();

#if VTK_MODULE_ENABLE_ParaView_PythonCatalyst
  vtkNew<vtkCPPythonStringPipeline> pipeline;
  pipeline->Initialize(script);
  processor->AddPipeline(pipeline);
#endif

  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("input");
  dataDescription->SetTimeData(0, 0.0);
  dataDescription->ForceOutputOn();
  if (processor->RequestDataDescription(dataDescription) != 0)
  {
    vtkNew<vtkPolyData> pd;
    vtkCPInputDataDescription* idd = dataDescription->GetInputDescriptionByName("input");
    idd->SetGrid(pd);
    processor->CoProcess(dataDescription);
  }
  processor->Finalize();
  return EXIT_SUCCESS;
}
