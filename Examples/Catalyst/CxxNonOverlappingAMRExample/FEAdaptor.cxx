#include "FEAdaptor.h"
#include <iostream>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkCompositeDataIterator.h>
#include <vtkNew.h>
#include <vtkNonOverlappingAMR.h>
#include <vtkUniformGrid.h>

namespace
{
vtkCPProcessor* Processor = NULL;
vtkNonOverlappingAMR* VTKGrid;

void BuildVTKGrid()
{
  if (VTKGrid == NULL)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    VTKGrid = vtkNonOverlappingAMR::New();
  }

  // Note that all of the vtkUniformGrids in the vtkNonOverlappingAMR
  // grid can use independent spacing, origin and extents. This is
  // shown in the mid-level grids in that they don't share an
  // origin. The highest level grid and lowest level grid do share
  // the same origin of (0,0,0) and thus need to have appropriate
  // extents and spacings as well.
  int numberOfLevels = 3;
  int blocksPerLevel[3] = { 1, 2, 1 };
  VTKGrid->Initialize(numberOfLevels, blocksPerLevel);

  // the highest level grid
  vtkNew<vtkUniformGrid> level0Grid;
  level0Grid->SetSpacing(4, 4, 4);
  level0Grid->SetOrigin(0, 0, 0);
  level0Grid->SetExtent(0, 10, 0, 20, 0, 20);
  VTKGrid->SetDataSet(0, 0, level0Grid);

  // the first mid-level grid
  vtkNew<vtkUniformGrid> level1Grid0;
  level1Grid0->SetSpacing(2, 2, 2);
  level1Grid0->SetOrigin(40, 0, 0);
  level1Grid0->SetExtent(0, 8, 0, 20, 0, 40);
  VTKGrid->SetDataSet(1, 0, level1Grid0);

  // the second mid-level grid
  vtkNew<vtkUniformGrid> level1Grid1;
  level1Grid1->SetSpacing(2, 2, 2);
  level1Grid1->SetOrigin(40, 40, 0);
  level1Grid1->SetExtent(0, 40, 0, 20, 0, 40);
  VTKGrid->SetDataSet(1, 1, level1Grid1);

  // the lowest level grid
  vtkNew<vtkUniformGrid> level2Grid;
  level2Grid->SetSpacing(1, 1, 2);
  level2Grid->SetOrigin(0, 0, 0);
  level2Grid->SetExtent(56, 120, 0, 40, 0, 40);
  VTKGrid->SetDataSet(2, 0, level2Grid);
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
  for (int i = 0; i < numScripts; i++)
  {
    vtkNew<vtkCPPythonScriptPipeline> pipeline;
    pipeline->Initialize(scripts[i]);
    Processor->AddPipeline(pipeline);
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
  if (Processor->RequestDataDescription(dataDescription) != 0)
  {
    BuildVTKGrid();
    dataDescription->GetInputDescriptionByName("input")->SetGrid(VTKGrid);
    Processor->CoProcess(dataDescription);
  }
}
} // end of Catalyst namespace
