#include "FEAdaptor.h"
#include <iostream>
#include <stdlib.h>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonPipeline.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkHyperTree.h>
#include <vtkHyperTreeCursor.h>
#include <vtkHyperTreeGrid.h>
#include <vtkIntArray.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>

FEAdaptor::FEAdaptor(int numScripts, char* scripts[])
{
  this->Processor = vtkCPProcessor::New();
  this->Processor->Initialize();
  for (int i = 0; i < numScripts; i++)
  {
    if (auto pipeline = vtkCPPythonPipeline::CreateAndInitializePipeline(scripts[i]))
    {
      Processor->AddPipeline(pipeline);
    }
    else
    {
      vtkLogF(ERROR, "failed to setup pipeline for '%s'", scripts[i]);
    }
  }
}

FEAdaptor::~FEAdaptor()
{
  this->Finalize();
}

void FEAdaptor::Finalize()
{
  if (this->Processor)
  {
    this->Processor->Delete();
    this->Processor = nullptr;
  }
}

void FEAdaptor::CoProcess(double time, unsigned int timeStep, bool lastTimeStep)
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
  if (this->Processor->RequestDataDescription(dataDescription) != 0)
  {
    vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
    int numberOfProcesses = controller->GetNumberOfProcesses();
    int processId = controller->GetLocalProcessId();
    vtkNew<vtkHyperTreeGrid> hyperTreeGrid;
    hyperTreeGrid->Initialize();
    int extent[6] = { processId, processId + 1, 0, 1, 0, 1 };
    hyperTreeGrid->SetGridExtent(extent);
    hyperTreeGrid->SetDimension(3);
    // hyperTreeGrid->SetOrientation(0);
    hyperTreeGrid->SetBranchFactor(2);
    // hyperTreeGrid->SetMaterialMaskIndex(nullptr);

    vtkNew<vtkDoubleArray> xCoords;
    xCoords->SetNumberOfValues(2);
    xCoords->SetValue(0, processId);
    xCoords->SetValue(1, processId + 1.);
    vtkNew<vtkDoubleArray> yCoords;
    yCoords->SetNumberOfValues(2);
    yCoords->SetValue(0, 0);
    yCoords->SetValue(1, 1);
    vtkNew<vtkDoubleArray> zCoords;
    zCoords->SetNumberOfValues(2);
    zCoords->SetValue(0, 0);
    zCoords->SetValue(1, 1);

    hyperTreeGrid->SetXCoordinates(xCoords);
    hyperTreeGrid->SetYCoordinates(yCoords);
    hyperTreeGrid->SetZCoordinates(zCoords);

    hyperTreeGrid->GenerateTrees();

    this->FillHTG(hyperTreeGrid);

    vtkCPInputDataDescription* inputDataDescription =
      dataDescription->GetInputDescriptionByName("input");
    inputDataDescription->SetGrid(hyperTreeGrid);

    // Set whole extent
    int wholeExtent[6] = { 0, numberOfProcesses, 0, 1, 0, 1 };
    inputDataDescription->SetWholeExtent(wholeExtent);

    this->Processor->CoProcess(dataDescription);
  }
}

bool FEAdaptor::ShouldRefine(unsigned int level)
{
  return level < 1 || (level < 5 && rand() % 100 < 80);
}

void FEAdaptor::AddData(vtkHyperTreeGrid* htg, vtkHyperTreeCursor* cursor)
{
  vtkDataArray* levels = htg->GetPointData()->GetArray("levels");
  vtkDataArray* ids = htg->GetPointData()->GetArray("ids");

  // std::cout << "add levels: " << levels << std::endl;
  // std::cout << "add ids: " << ids << std::endl;

  unsigned int level = cursor->GetLevel();
  vtkIdType idx = cursor->GetTree()->GetGlobalIndexFromLocal(cursor->GetVertexId());
  levels->InsertTuple1(idx, level);
  ids->InsertTuple1(idx, idx);
  // std::cout << "add data at " << idx << " with level " << level << std::endl;
}

void FEAdaptor::SubdivideLeaves(vtkHyperTreeGrid* htg, vtkHyperTreeCursor* cursor, long long treeId)
{
  this->AddData(htg, cursor);
  if (cursor->IsLeaf())
  {
    if (this->ShouldRefine(cursor->GetLevel()))
    {
      htg->SubdivideLeaf(cursor, treeId);
      this->SubdivideLeaves(htg, cursor, treeId);
    }
  }
  else
  {
    long long nbChildren = cursor->GetNumberOfChildren();
    for (long long childIdx = 0; childIdx < nbChildren; childIdx++)
    {
      cursor->ToChild(childIdx);
      this->SubdivideLeaves(htg, cursor, treeId);
      cursor->ToParent();
    }
  }
}

void FEAdaptor::FillHTG(vtkHyperTreeGrid* htg)
{
  vtkNew<vtkIntArray> levels;
  levels->SetName("levels");
  htg->GetPointData()->AddArray(levels);
  // std::cout << "levels: " << htg->GetPointData()->GetArray("levels") << std::endl;

  vtkNew<vtkIntArray> ids;
  ids->SetName("ids");
  htg->GetPointData()->AddArray(ids);
  // std::cout << "ids: " << htg->GetPointData()->GetArray("ids") << std::endl;

  std::cout << "FillHTG" << std::endl;
  long long treeOffset = 0;
  long long nbTree = htg->GetNumberOfTrees();
  for (long long treeId = 0; treeId < nbTree; treeId++)
  {
    vtkHyperTreeCursor* cursor = htg->NewCursor(treeId);
    cursor->ToRoot();
    cursor->GetTree()->SetGlobalIndexStart(treeOffset);
    this->SubdivideLeaves(htg, cursor, treeId);
    treeOffset += cursor->GetTree()->GetNumberOfVertices();
    cursor->Delete();
  }
  std::cout << "Data size: " << treeOffset << std::endl;
  std::cout << "Ids size: " << ids->GetNumberOfTuples() << std::endl;
  std::cout << "Levels size: " << levels->GetNumberOfTuples() << std::endl;
}
