#include "FEAdaptor.h"
#include <iostream>

#include <vtkAMRBox.h>
#include <vtkAMRInformation.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkCompositeDataIterator.h>
#include <vtkDoubleArray.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkOverlappingAMR.h>
#include <vtkPointData.h>
#include <vtkUniformGrid.h>

namespace
{
vtkCPProcessor* Processor = nullptr;

void BuildVTKGrid(vtkOverlappingAMR* grid)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int myRank = controller->GetLocalProcessId();
  int numRanks = controller->GetNumberOfProcesses();

  int numberOfLevels = 3;
  int blocksPerLevel[3] = { numRanks, numRanks, 2 * numRanks };
  grid->Initialize(numberOfLevels, blocksPerLevel);
  grid->SetGridDescription(VTK_XYZ_GRID);
  double globalOrigin[] = { 0, 0, 0 };
  double level0Spacing[] = { 4, 4, 4 };
  double level1Spacing[] = { 2, 2, 2 };
  double level2Spacing[] = { 1, 1, 1 };
  grid->SetOrigin(globalOrigin);
  for (int rank = 0; rank < numRanks; rank++)
  {
    int level0CellDims[] = { rank * 13, rank * 13 + 12, 0, 12, 0,
      12 }; // first and last cell in each direction
    vtkAMRBox level0Box(level0CellDims);
    int level1CellDims[] = { rank * 26, rank * 26 + 19, 0, 19, 0,
      19 }; // first and last cell in each direction
    vtkAMRBox level1Box(level1CellDims);
    int level2CellDims_0[] = { rank * 52, rank * 52 + 11, 0, 11, 0,
      11 }; // first and last cell in each direction
    vtkAMRBox level2Box_0(level2CellDims_0);
    int level2CellDims_1[] = { rank * 52 + 14, rank * 52 + 21, 0, 11, 0,
      11 }; // first and last cell in each direction
    vtkAMRBox level2Box_1(level2CellDims_1);

    grid->GetAMRInfo()->SetSpacing(0, level0Spacing);
    grid->GetAMRInfo()->SetRefinementRatio(0, 2);
    grid->GetAMRInfo()->SetAMRBox(0, rank, level0Box);

    grid->GetAMRInfo()->SetSpacing(1, level1Spacing);
    grid->GetAMRInfo()->SetRefinementRatio(1, 2);
    grid->GetAMRInfo()->SetAMRBox(1, rank, level1Box);

    grid->GetAMRInfo()->SetSpacing(2, level2Spacing);
    grid->GetAMRInfo()->SetRefinementRatio(2, 2);
    grid->GetAMRInfo()->SetAMRBox(2, 2 * rank, level2Box_0);
    grid->GetAMRInfo()->SetAMRBox(2, 2 * rank + 1, level2Box_1);
  }

  grid->GenerateParentChildInformation();

  int level0CellDims[] = { myRank * 13, myRank * 13 + 12, 0, 12, 0,
    12 }; // first and last cell in each direction
  int level1CellDims[] = { myRank * 26, myRank * 26 + 19, 0, 19, 0,
    19 }; // first and last cell in each direction
  int level2CellDims_0[] = { myRank * 52, myRank * 52 + 11, 0, 11, 0,
    11 }; // first and last cell in each direction
  int level2CellDims_1[] = { myRank * 52 + 14, myRank * 52 + 21, 0, 11, 0,
    11 }; // first and last cell in each direction

  // the highest level grid
  vtkNew<vtkUniformGrid> level0Grid;
  level0Grid->SetSpacing(level0Spacing);
  level0Grid->SetOrigin(globalOrigin);
  // first and last point in each direction which is 1 more than cells in each direction
  level0Grid->SetExtent(level0CellDims[0], level0CellDims[1] + 1, level0CellDims[2],
    level0CellDims[3] + 1, level0CellDims[4], level0CellDims[5] + 1);
  grid->SetDataSet(0, myRank, level0Grid.GetPointer());

  // the mid-level grid
  vtkNew<vtkUniformGrid> level1Grid;
  level1Grid->SetSpacing(level1Spacing);
  level1Grid->SetOrigin(globalOrigin);
  // first and last point in each direction which is 1 more than cells in each direction
  level1Grid->SetExtent(level1CellDims[0], level1CellDims[1] + 1, level1CellDims[2],
    level1CellDims[3] + 1, level1CellDims[4], level1CellDims[5] + 1);
  grid->SetDataSet(1, myRank, level1Grid.GetPointer());

  // the lowest level grids
  vtkNew<vtkUniformGrid> level2Grid_0;
  level2Grid_0->SetSpacing(level2Spacing);
  level2Grid_0->SetOrigin(globalOrigin);
  // first and last point in each direction which is 1 more than cells in each direction
  level2Grid_0->SetExtent(level2CellDims_0[0], level2CellDims_0[1] + 1, level2CellDims_0[2],
    level2CellDims_0[3] + 1, level2CellDims_0[4], level2CellDims_0[5] + 1);
  grid->SetDataSet(2, 2 * myRank, level2Grid_0.GetPointer());
  vtkNew<vtkUniformGrid> level2Grid_1;
  level2Grid_1->SetOrigin(globalOrigin);
  level2Grid_1->SetSpacing(level2Spacing);
  // first and last point in each direction which is 1 more than cells in each direction
  level2Grid_1->SetExtent(level2CellDims_1[0], level2CellDims_1[1] + 1, level2CellDims_1[2],
    level2CellDims_1[3] + 1, level2CellDims_1[4], level2CellDims_1[5] + 1);
  grid->SetDataSet(2, 2 * myRank + 1, level2Grid_1.GetPointer());
}
}

namespace FEAdaptor
{
void Initialize(int numScripts, char* scripts[])
{
  if (Processor == nullptr)
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
    cerr << "adding in script " << scripts[i] << endl;
    Processor->AddPipeline(pipeline.GetPointer());
  }
}

void Finalize()
{
  if (Processor)
  {
    Processor->Delete();
    Processor = nullptr;
  }
}

void BuildFields(vtkOverlappingAMR* grid, vtkCPInputDataDescription* idd)
{
  if (idd->IsFieldNeeded("data", vtkDataObject::POINT) == false)
  {
    return;
  }
  vtkCompositeDataIterator* iter = grid->NewIterator();
  iter->InitTraversal();
  iter->SkipEmptyNodesOn();
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (vtkDataSet* gridDataset = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
    {
      vtkNew<vtkDoubleArray> data;
      data->SetNumberOfTuples(gridDataset->GetNumberOfPoints());
      data->SetName("data");
      double pt[3];
      for (vtkIdType i = 0; i < gridDataset->GetNumberOfPoints(); i++)
      {
        gridDataset->GetPoint(i, pt);
        pt[0] = -pt[0]; // just to make it change inversely proportional to myRank
        data->SetTypedTuple(i, pt);
      }
      gridDataset->GetPointData()->AddArray(data.GetPointer());
    }
  }
  iter->Delete();
  iter = nullptr;
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
    vtkNew<vtkOverlappingAMR> grid;
    BuildVTKGrid(grid.GetPointer());
    vtkCPInputDataDescription* idd = dataDescription->GetInputDescriptionByName("input");
    BuildFields(grid.GetPointer(), idd);
    idd->SetGrid(grid.GetPointer());
    Processor->CoProcess(dataDescription.GetPointer());
  }
}
} // end of Catalyst namespace
