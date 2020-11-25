#include "FEAdaptor.h"
#include "FEDataStructures.h"
#include <iostream>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonPipeline.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

#include "vtkCPMappedVectorArrayTemplate.h"

namespace
{
vtkCPProcessor* Processor = NULL;
vtkUnstructuredGrid* VTKGrid;

void BuildVTKGrid(Grid& grid)
{
  // create the points information
  vtkCPMappedVectorArrayTemplate<double>* pointArray =
    vtkCPMappedVectorArrayTemplate<double>::New();

  pointArray->SetVectorArray(
    grid.GetPointsArray(), static_cast<vtkIdType>(grid.GetNumberOfPoints()));
  vtkNew<vtkPoints> points;
  points->SetData(pointArray);
  pointArray->Delete();
  VTKGrid->SetPoints(points);

  // create the cells
  size_t numCells = grid.GetNumberOfCells();
  VTKGrid->Allocate(static_cast<vtkIdType>(numCells * 9));
  for (size_t cell = 0; cell < numCells; cell++)
  {
    unsigned int* cellPoints = grid.GetCellPoints(cell);
    vtkIdType tmp[8] = { cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3], cellPoints[4],
      cellPoints[5], cellPoints[6], cellPoints[7] };
    VTKGrid->InsertNextCell(VTK_HEXAHEDRON, 8, tmp);
  }
}

void UpdateVTKAttributes(Grid& grid, Attributes& attributes, vtkCPInputDataDescription* idd)
{
  if (idd->IsFieldNeeded("velocity", vtkDataObject::POINT))
  {
    if (VTKGrid->GetPointData()->GetNumberOfArrays() == 0)
    {
      // velocity array
      vtkCPMappedVectorArrayTemplate<double>* velocity =
        vtkCPMappedVectorArrayTemplate<double>::New();
      velocity->SetName("velocity");
      VTKGrid->GetPointData()->AddArray(velocity);
      velocity->Delete();
    }
    vtkCPMappedVectorArrayTemplate<double>* velocity =
      vtkCPMappedVectorArrayTemplate<double>::SafeDownCast(
        VTKGrid->GetPointData()->GetArray("velocity"));
    velocity->SetVectorArray(attributes.GetVelocityArray(), VTKGrid->GetNumberOfPoints());
  }
  if (idd->IsFieldNeeded("pressure", vtkDataObject::CELL))
  {
    if (VTKGrid->GetCellData()->GetNumberOfArrays() == 0)
    {
      // pressure array
      vtkNew<vtkFloatArray> pressure;
      pressure->SetName("pressure");
      pressure->SetNumberOfComponents(1);
      VTKGrid->GetCellData()->AddArray(pressure);
    }
    vtkFloatArray* pressure =
      vtkFloatArray::SafeDownCast(VTKGrid->GetCellData()->GetArray("pressure"));
    // The pressure array is a scalar array so we can reuse
    // memory as long as we ordered the points properly.
    float* pressureData = attributes.GetPressureArray();
    pressure->SetArray(pressureData, static_cast<vtkIdType>(grid.GetNumberOfCells()), 1);
  }
}

void BuildVTKDataStructures(Grid& grid, Attributes& attributes, vtkCPInputDataDescription* idd)
{
  if (VTKGrid == NULL)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    VTKGrid = vtkUnstructuredGrid::New();
    BuildVTKGrid(grid);
  }
  UpdateVTKAttributes(grid, attributes, idd);
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

void CoProcess(
  Grid& grid, Attributes& attributes, double time, unsigned int timeStep, bool lastTimeStep)
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
    vtkCPInputDataDescription* idd = dataDescription->GetInputDescriptionByName("input");
    BuildVTKDataStructures(grid, attributes, idd);
    idd->SetGrid(VTKGrid);
    Processor->CoProcess(dataDescription);
  }
}
} // end of Catalyst namespace
