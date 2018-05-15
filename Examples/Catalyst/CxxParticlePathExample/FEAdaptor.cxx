#include "FEAdaptor.h"
#include "FEDataStructures.h"
#include <iostream>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

namespace
{
vtkCPProcessor* Processor = NULL;

void BuildVTKGrid(Grid& grid, vtkUnstructuredGrid* vtkgrid)
{
  // create the points information
  vtkNew<vtkDoubleArray> pointArray;
  pointArray->SetNumberOfComponents(3);
  pointArray->SetArray(
    grid.GetPointsArray(), static_cast<vtkIdType>(grid.GetNumberOfPoints() * 3), 1);
  vtkNew<vtkPoints> points;
  points->SetData(pointArray.GetPointer());
  vtkgrid->SetPoints(points.GetPointer());

  // create the cells
  size_t numCells = grid.GetNumberOfCells();
  vtkgrid->Allocate(static_cast<vtkIdType>(numCells * 9));
  for (size_t cell = 0; cell < numCells; cell++)
  {
    unsigned int* cellPoints = grid.GetCellPoints(cell);
    vtkIdType tmp[8] = { cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3], cellPoints[4],
      cellPoints[5], cellPoints[6], cellPoints[7] };
    vtkgrid->InsertNextCell(VTK_HEXAHEDRON, 8, tmp);
  }
}

void UpdateVTKAttributes(
  Grid& grid, Attributes& attributes, vtkUnstructuredGrid* vtkgrid, vtkCPInputDataDescription* idd)
{
  if (idd->IsFieldNeeded("velocity", vtkDataObject::POINT))
  {
    if (vtkgrid->GetPointData()->GetNumberOfArrays() == 0)
    {
      // velocity array
      vtkNew<vtkDoubleArray> velocity;
      velocity->SetName("velocity");
      velocity->SetNumberOfComponents(3);
      velocity->SetNumberOfTuples(static_cast<vtkIdType>(grid.GetNumberOfPoints()));
      vtkgrid->GetPointData()->AddArray(velocity.GetPointer());
    }
    vtkDoubleArray* velocity =
      vtkDoubleArray::SafeDownCast(vtkgrid->GetPointData()->GetArray("velocity"));
    // The velocity array is ordered as vx0,vx1,vx2,..,vy0,vy1,vy2,..,vz0,vz1,vz2,..
    // so we need to create a full copy of it with VTK's ordering of
    // vx0,vy0,vz0,vx1,vy1,vz1,..
    double* velocityData = attributes.GetVelocityArray();
    vtkIdType numTuples = velocity->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numTuples; i++)
    {
      double values[3] = { velocityData[i], velocityData[i + numTuples],
        velocityData[i + 2 * numTuples] };
      velocity->SetTypedTuple(i, values);
    }
  }
  if (idd->IsFieldNeeded("pressure", vtkDataObject::CELL))
  {
    if (vtkgrid->GetCellData()->GetNumberOfArrays() == 0)
    {
      // pressure array
      vtkNew<vtkFloatArray> pressure;
      pressure->SetName("pressure");
      pressure->SetNumberOfComponents(1);
      vtkgrid->GetCellData()->AddArray(pressure.GetPointer());
    }
    vtkFloatArray* pressure =
      vtkFloatArray::SafeDownCast(vtkgrid->GetCellData()->GetArray("pressure"));
    // The pressure array is a scalar array so we can reuse
    // memory as long as we ordered the points properly.
    float* pressureData = attributes.GetPressureArray();
    pressure->SetArray(pressureData, static_cast<vtkIdType>(grid.GetNumberOfCells()), 1);
  }
}

void BuildVTKDataStructures(
  Grid& grid, Attributes& attributes, vtkUnstructuredGrid* vtkgrid, vtkCPInputDataDescription* idd)
{
  BuildVTKGrid(grid, vtkgrid);
  UpdateVTKAttributes(grid, attributes, vtkgrid, idd);
}
}

namespace FEAdaptor
{
void Initialize(std::vector<std::string>& scripts)
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
  for (std::vector<std::string>::iterator it = scripts.begin(); it != scripts.end(); it++)
  {
    vtkNew<vtkCPPythonScriptPipeline> pipeline;
    pipeline->Initialize(it->c_str());
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
  if (Processor->RequestDataDescription(dataDescription.GetPointer()) != 0)
  {
    vtkCPInputDataDescription* idd = dataDescription->GetInputDescriptionByName("input");
    vtkNew<vtkUnstructuredGrid> vtkgrid;
    BuildVTKDataStructures(grid, attributes, vtkgrid.GetPointer(), idd);
    idd->SetGrid(vtkgrid.GetPointer());
    Processor->CoProcess(dataDescription.GetPointer());
  }
}
} // end of Catalyst namespace
