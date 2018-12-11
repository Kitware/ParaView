/*=========================================================================

  Program:   ParaView
  Module:    CPXMLPWriterPipeline.cxx

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
#include "vtkCPXMLPWriterPipeline.h"
#include "vtkCellTypeSource.h"
#include "vtkCompositeDataSet.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include <vtksys/SystemTools.hxx>

int CPXMLPWriterPipeline(int argc, char* argv[])
{
  vtkNew<vtkCPDataDescription> dd;
  dd->SetTimeData(10, 10);

  vtkNew<vtkImageData> imageData;
  int extent[6] = { 0, 2, 0, 2, 0, 2 };
  imageData->SetExtent(extent);
  dd->AddInput("ImageData");
  dd->GetInputDescriptionByName("ImageData")->SetGrid(imageData);
  dd->GetInputDescriptionByName("ImageData")->SetWholeExtent(extent);

  if (const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/cth.vtr"))
  {
    vtkNew<vtkXMLRectilinearGridReader> rectilinearGridReader;
    rectilinearGridReader->SetFileName(fname);
    rectilinearGridReader->Update();
    vtkRectilinearGrid* rectilinearGrid = rectilinearGridReader->GetOutput();
    dd->AddInput("RectilinearGrid");
    dd->GetInputDescriptionByName("RectilinearGrid")->SetGrid(rectilinearGrid);
    dd->GetInputDescriptionByName("RectilinearGrid")->SetWholeExtent(rectilinearGrid->GetExtent());
  }
  else
  {
    vtkGenericWarningMacro("Could not open cth.vtr");
    return 1;
  }

  if (const char* fname =
        vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/bluntfin.vts"))
  {
    vtkNew<vtkXMLStructuredGridReader> structuredGridReader;
    structuredGridReader->SetFileName(fname);
    structuredGridReader->Update();
    vtkStructuredGrid* structuredGrid = structuredGridReader->GetOutput();
    dd->AddInput("StructuredGrid");
    dd->GetInputDescriptionByName("StructuredGrid")->SetGrid(structuredGrid);
    dd->GetInputDescriptionByName("StructuredGrid")->SetWholeExtent(structuredGrid->GetExtent());
  }
  else
  {
    vtkGenericWarningMacro("Could not open bluntfin.vts");
    return 1;
  }

  if (const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/cow.vtp"))
  {
    vtkNew<vtkXMLPolyDataReader> polyDataReader;
    polyDataReader->SetFileName(fname);
    polyDataReader->Update();
    dd->AddInput("PolyData");
    dd->GetInputDescriptionByName("PolyData")->SetGrid(polyDataReader->GetOutput());
  }
  else
  {
    vtkGenericWarningMacro("Could not open cow.vtp");
    return 1;
  }

  vtkNew<vtkCellTypeSource> unstructuredGridSource;
  unstructuredGridSource->Update();
  dd->AddInput("UnstructuredGrid");
  dd->GetInputDescriptionByName("UnstructuredGrid")->SetGrid(unstructuredGridSource->GetOutput());

  vtkNew<vtkMultiBlockDataSet> multiBlock;
  multiBlock->SetNumberOfBlocks(2);
  multiBlock->SetBlock(0, unstructuredGridSource->GetOutput());
  multiBlock->SetBlock(1, imageData);
  dd->AddInput("MultiBlock");
  dd->GetInputDescriptionByName("MultiBlock")->SetGrid(multiBlock);

  char* temp =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!temp)
  {
    cerr << "Could not determine temporary directory." << endl;
    return 1;
  }
  std::string tempDir = temp;
  delete[] temp;

  vtkNew<vtkCPProcessor> processor;
  processor->Initialize();
  vtkNew<vtkCPXMLPWriterPipeline> pipeline;
  pipeline->SetPaddingAmount(3);
  pipeline->SetPath(tempDir);
  processor->AddPipeline(pipeline);
  processor->CoProcess(dd);

  // now check if we actually created the files
  std::string names[6] = { tempDir + "/ImageData_010.pvti", tempDir + "/MultiBlock_010.vtm",
    tempDir + "/PolyData_010.pvts", tempDir + "/RectilinearGrid_010.pvtr",
    tempDir + "/StructuredGrid_010.pvts", tempDir + "/UnstructuredGrid_010.pvtu" };
  for (auto& name : names)
  {
    if (!vtksys::SystemTools::FileExists(name.c_str()))
    {
      vtkGenericWarningMacro("Did not write out " << name);
      return 1;
    }
  }

  return 0;
}
