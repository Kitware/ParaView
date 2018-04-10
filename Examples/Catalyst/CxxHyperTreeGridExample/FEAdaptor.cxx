#include "FEAdaptor.h"
#include <iostream>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkDoubleArray.h>
#include <vtkHyperTreeGrid.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkPoints.h>

FEAdaptor::FEAdaptor(int numScripts, char* scripts[])
{
  this->Processor = vtkCPProcessor::New();
  this->Processor->Initialize();
  for (int i = 0; i < numScripts; i++)
  {
    vtkNew<vtkCPPythonScriptPipeline> pipeline;
    pipeline->Initialize(scripts[i]);
    this->Processor->AddPipeline(pipeline);
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
    hyperTreeGrid->SetOrientation(0);
    hyperTreeGrid->SetBranchFactor(2);
    hyperTreeGrid->SetMaterialMaskIndex(nullptr);

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

    vtkCPInputDataDescription* inputDataDescription =
      dataDescription->GetInputDescriptionByName("input");
    inputDataDescription->SetGrid(hyperTreeGrid);

    this->Processor->CoProcess(dataDescription);
  }
}
