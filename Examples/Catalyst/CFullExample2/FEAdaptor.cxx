#include "FEAdaptor.h"
#include <iostream>

#include <vtkCPAdaptorAPI.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

namespace
{
vtkSmartPointer<vtkUnstructuredGrid> VTKGrid;

void BuildVTKGrid(unsigned int numberOfPoints, double* pointsData, unsigned int numberOfCells,
  unsigned int* cellsData)
{
  // create the points information
  vtkNew<vtkDoubleArray> pointArray;
  pointArray->SetNumberOfComponents(3);
  pointArray->SetArray(pointsData, static_cast<vtkIdType>(numberOfPoints * 3), 1);
  vtkNew<vtkPoints> points;
  points->SetData(pointArray);
  VTKGrid->SetPoints(points);

  // create the cells
  VTKGrid->Allocate(static_cast<vtkIdType>(numberOfCells * 9));
  for (unsigned int cell = 0; cell < numberOfCells; cell++)
  {
    unsigned int* cellPoints = cellsData + 8 * cell;
    vtkIdType tmp[8] = { cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3], cellPoints[4],
      cellPoints[5], cellPoints[6], cellPoints[7] };
    VTKGrid->InsertNextCell(VTK_HEXAHEDRON, 8, tmp);
  }
}

void UpdateVTKAttributes(vtkCPInputDataDescription* idd, unsigned int numberOfPoints,
  double* velocityData, unsigned int numberOfCells, float* pressureData)
{
  if (idd->IsFieldNeeded("velocity", vtkDataObject::POINT) == true)
  {
    if (VTKGrid->GetPointData()->GetNumberOfArrays() == 0)
    {
      // velocity array
      vtkNew<vtkDoubleArray> velocity;
      velocity->SetName("velocity");
      velocity->SetNumberOfComponents(3);
      velocity->SetNumberOfTuples(static_cast<vtkIdType>(numberOfPoints));
      VTKGrid->GetPointData()->AddArray(velocity);
    }
    vtkDoubleArray* velocity =
      vtkDoubleArray::SafeDownCast(VTKGrid->GetPointData()->GetArray("velocity"));
    // The velocity array is ordered as vx0,vx1,vx2,..,vy0,vy1,vy2,..,vz0,vz1,vz2,..
    // so we need to create a full copy of it with VTK's ordering of
    // vx0,vy0,vz0,vx1,vy1,vz1,..
    for (unsigned int i = 0; i < numberOfPoints; i++)
    {
      double values[3] = { velocityData[i], velocityData[i + numberOfPoints],
        velocityData[i + 2 * numberOfPoints] };
      velocity->SetTypedTuple(i, values);
    }
  }
  if (idd->IsFieldNeeded("pressure", vtkDataObject::CELL) == true)
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
    pressure->SetArray(pressureData, static_cast<vtkIdType>(numberOfCells), 1);
  }
}

void BuildVTKDataStructures(vtkCPInputDataDescription* idd, unsigned int numberOfPoints,
  double* points, double* velocity, unsigned int numberOfCells, unsigned int* cells,
  float* pressure)
{
  if (VTKGrid == nullptr)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    VTKGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    BuildVTKGrid(numberOfPoints, points, numberOfCells, cells);
  }
  UpdateVTKAttributes(idd, numberOfPoints, velocity, numberOfCells, pressure);
}
}

void CatalystCoProcess(unsigned int numberOfPoints, double* pointsData, unsigned int numberOfCells,
  unsigned int* cellsData, double* velocityData, float* pressureData, double time,
  unsigned int timeStep, int lastTimeStep)
{
  vtkCPProcessor* processor = vtkCPAdaptorAPI::GetCoProcessor();
  vtkCPDataDescription* dataDescription = vtkCPAdaptorAPI::GetCoProcessorData();
  if (processor == nullptr || dataDescription == nullptr)
  {
    cerr << "ERROR: Catalyst not properly initialized.\n";
    return;
  }

  dataDescription->AddInput("input");
  dataDescription->SetTimeData(time, timeStep);
  if (lastTimeStep == true)
  {
    // assume that we want to all the pipelines to execute if it
    // is the last time step.
    dataDescription->ForceOutputOn();
  }
  if (processor->RequestDataDescription(dataDescription) != 0)
  {
    vtkCPInputDataDescription* idd = dataDescription->GetInputDescriptionByName("input");
    BuildVTKDataStructures(
      idd, numberOfPoints, pointsData, velocityData, numberOfCells, cellsData, pressureData);
    idd->SetGrid(VTKGrid);
    processor->CoProcess(dataDescription);
  }
}

void CatalystFinalize()
{ // Used to free grid if it was used
  if (VTKGrid)
  {
    VTKGrid = nullptr;
  }
}
