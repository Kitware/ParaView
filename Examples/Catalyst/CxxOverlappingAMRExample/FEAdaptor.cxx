#include "FEAdaptor.h"
#include <iostream>

#include <vtkAMRBox.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkCompositeDataIterator.h>
#include <vtkNew.h>
#include <vtkOverlappingAMR.h>
#include <vtkUniformGrid.h>

namespace
{
vtkCPProcessor* Processor = NULL;
vtkOverlappingAMR* VTKGrid;

void BuildVTKGrid()
{
  if (VTKGrid == NULL)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    VTKGrid = vtkOverlappingAMR::New();
    BuildVTKGrid();
  }

  int numberOfLevels = 3;
  int blocksPerLevel[3] = { 1, 1, 1 };
  VTKGrid->Initialize(numberOfLevels, blocksPerLevel);
  VTKGrid->SetGridDescription(VTK_XYZ_GRID);
  double origin[] = { 0, 0, 0 };
  double level0Spacing[] = { 4, 4, 4 };
  double level1Spacing[] = { 2, 2, 2 };
  double level2Spacing[] = { 1, 1, 1 };
  VTKGrid->SetOrigin(origin);
  int level0Dims[] = { 25, 25, 25 };
  vtkAMRBox level0Box(origin, level0Dims, level0Spacing, origin, VTK_XYZ_GRID);
  int level1Dims[] = { 20, 20, 20 };
  vtkAMRBox level1Box(origin, level1Dims, level1Spacing, origin, VTK_XYZ_GRID);
  int level2Dims[] = { 10, 10, 10 };
  vtkAMRBox level2Box(origin, level2Dims, level2Spacing, origin, VTK_XYZ_GRID);
  VTKGrid->SetSpacing(0, level0Spacing);
  VTKGrid->SetAMRBox(0, 0, level0Box);

  VTKGrid->SetSpacing(1, level1Spacing);
  VTKGrid->SetAMRBox(1, 0, level1Box);

  VTKGrid->SetSpacing(2, level2Spacing);
  VTKGrid->SetAMRBox(2, 0, level2Box);

  VTKGrid->GenerateParentChildInformation();

  // the highest level grid
  vtkNew<vtkUniformGrid> level0Grid;
  level0Grid->SetSpacing(level0Spacing);
  level0Grid->SetOrigin(0, 0, 0);
  level0Grid->SetExtent(0, 25, 0, 25, 0, 25);
  VTKGrid->SetDataSet(0, 0, level0Grid.GetPointer());

  // the mid-level grid
  vtkNew<vtkUniformGrid> level1Grid0;
  level1Grid0->SetSpacing(level1Spacing);
  level1Grid0->SetExtent(0, 20, 0, 20, 0, 20);
  VTKGrid->SetDataSet(1, 0, level1Grid0.GetPointer());

  // the lowest level grid
  vtkNew<vtkUniformGrid> level2Grid;
  level2Grid->SetSpacing(level2Spacing);
  level2Grid->SetExtent(0, 10, 0, 10, 0, 10);
  VTKGrid->SetDataSet(2, 0, level2Grid.GetPointer());
}
}

namespace FEAdaptor
{
void Initialize(int numScripts, char* scripts[])
{
  if (Processor == NULL)
  {
    Processor = vtkCPProcessor::New();
    Processor->Initialize();
  }
  else
  {
    Processor->RemoveAllPipelines();
  }
  for (int i = 1; i < numScripts; i++)
  {
    vtkNew<vtkCPPythonScriptPipeline> pipeline;
    pipeline->Initialize(scripts[i]);
    Processor->AddPipeline(pipeline.GetPointer());
  }
}

void Finalize()
{
  if (Processor)
  {
    Processor->Delete();
    Processor = NULL;
  }
  if (VTKGrid)
  {
    VTKGrid->Delete();
    VTKGrid = NULL;
  }
}

void CoProcess(double time, unsigned int timeStep, bool lastTimeStep)
{
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("input");
  dataDescription->SetTimeData(time, timeStep);
  if (lastTimeStep == true)
  {
    // assume that we want to all the pipelines to execute if it
    // is the last time step.
    dataDescription->ForceOutputOn();
  }
  if (Processor->RequestDataDescription(dataDescription.GetPointer()) != 0)
  {
    BuildVTKGrid();
    dataDescription->GetInputDescriptionByName("input")->SetGrid(VTKGrid);
    Processor->CoProcess(dataDescription.GetPointer());
  }
}
} // end of Catalyst namespace
