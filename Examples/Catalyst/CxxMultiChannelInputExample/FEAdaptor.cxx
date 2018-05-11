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
#include <vtkPolyData.h>
#include <vtkUnstructuredGrid.h>

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

void FEAdaptor::CoProcess(Grid& grid, Attributes& attributes, Particles& particles, double time,
  unsigned int timeStep, bool lastTimeStep)
{
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->AddInput("volumetric grid");
  dataDescription->AddInput("particles");
  dataDescription->SetTimeData(time, timeStep);
  if (lastTimeStep == true)
  {
    // assume that we want to all the pipelines to execute if it
    // is the last time step.
    dataDescription->ForceOutputOn();
  }
  if (Processor->RequestDataDescription(dataDescription) != 0)
  {
    // We know that Catalyst in situ work needs to be done this time step
    // but we need to query each input channel (i.e. volumetric grid and particles)
    // to see which ones are needed this time step.
    vtkCPInputDataDescription* volumetricGridChannel =
      dataDescription->GetInputDescriptionByName("volumetric grid");
    if (volumetricGridChannel->GetIfGridIsNecessary())
    { // only build the VTK volumetric grid if it's needed this time step
      vtkNew<vtkUnstructuredGrid> volumetricGrid;
      this->BuildVTKVolumetricGridDataStructures(
        grid, attributes, volumetricGridChannel, volumetricGrid);
      volumetricGridChannel->SetGrid(volumetricGrid);
    }
    vtkCPInputDataDescription* particlesChannel =
      dataDescription->GetInputDescriptionByName("particles");
    if (particlesChannel->GetIfGridIsNecessary())
    { // only build the VTK particles if it's needed this time step
      vtkNew<vtkPolyData> vtkparticles;
      // the particles have no field data
      this->BuildVTKParticlesDataStructures(particles, vtkparticles);
      particlesChannel->SetGrid(vtkparticles);
    }
    this->Processor->CoProcess(dataDescription);
  }
}

void FEAdaptor::BuildVTKVolumetricGrid(Grid& grid, vtkUnstructuredGrid* volumetricGrid)
{
  // create the points information
  vtkNew<vtkDoubleArray> pointArray;
  pointArray->SetNumberOfComponents(3);
  pointArray->SetArray(
    grid.GetPointsArray(), static_cast<vtkIdType>(grid.GetNumberOfPoints() * 3), 1);
  vtkNew<vtkPoints> points;
  points->SetData(pointArray);
  volumetricGrid->SetPoints(points);

  // create the cells
  size_t numCells = grid.GetNumberOfCells();
  volumetricGrid->Allocate(static_cast<vtkIdType>(numCells * 9));
  for (size_t cell = 0; cell < numCells; cell++)
  {
    unsigned int* cellPoints = grid.GetCellPoints(cell);
    vtkIdType tmp[8] = { cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3], cellPoints[4],
      cellPoints[5], cellPoints[6], cellPoints[7] };
    volumetricGrid->InsertNextCell(VTK_HEXAHEDRON, 8, tmp);
  }
}

void FEAdaptor::UpdateVTKAttributes(Grid& grid, Attributes& attributes,
  vtkCPInputDataDescription* volumetricGridChannel, vtkUnstructuredGrid* volumetricGrid)
{
  if (volumetricGridChannel->IsFieldNeeded("velocity", 0))
  {
    if (volumetricGrid->GetPointData()->GetNumberOfArrays() == 0)
    {
      // velocity array
      vtkNew<vtkDoubleArray> velocity;
      velocity->SetName("velocity");
      velocity->SetNumberOfComponents(3);
      velocity->SetNumberOfTuples(static_cast<vtkIdType>(grid.GetNumberOfPoints()));
      volumetricGrid->GetPointData()->AddArray(velocity);
    }
    vtkDoubleArray* velocity =
      vtkDoubleArray::SafeDownCast(volumetricGrid->GetPointData()->GetArray("velocity"));
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
  if (volumetricGridChannel->IsFieldNeeded("pressure", 1))
  {
    if (volumetricGrid->GetCellData()->GetNumberOfArrays() == 0)
    {
      // pressure array
      vtkNew<vtkFloatArray> pressure;
      pressure->SetName("pressure");
      pressure->SetNumberOfComponents(1);
      volumetricGrid->GetCellData()->AddArray(pressure);
    }
    vtkFloatArray* pressure =
      vtkFloatArray::SafeDownCast(volumetricGrid->GetCellData()->GetArray("pressure"));
    // The pressure array is a scalar array so we can reuse
    // memory as long as we ordered the points properly.
    float* pressureData = attributes.GetPressureArray();
    pressure->SetArray(pressureData, static_cast<vtkIdType>(grid.GetNumberOfCells()), 1);
  }
}

void FEAdaptor::BuildVTKVolumetricGridDataStructures(Grid& grid, Attributes& attributes,
  vtkCPInputDataDescription* volumetricGridChannel, vtkUnstructuredGrid* volumetricGrid)
{
  this->BuildVTKVolumetricGrid(grid, volumetricGrid);
  this->UpdateVTKAttributes(grid, attributes, volumetricGridChannel, volumetricGrid);
}

void FEAdaptor::BuildVTKParticlesDataStructures(Particles& particles, vtkPolyData* vtkparticles)
{
  // create the points information
  vtkNew<vtkDoubleArray> pointArray;
  pointArray->SetNumberOfComponents(3);
  pointArray->SetArray(
    particles.GetPointsArray(), static_cast<vtkIdType>(particles.GetNumberOfPoints() * 3), 1);
  vtkNew<vtkPoints> points;
  points->SetData(pointArray);
  vtkparticles->SetPoints(points);

  // create the cells
  vtkIdType numCells = static_cast<vtkIdType>(particles.GetNumberOfPoints());
  vtkparticles->Allocate(numCells);
  for (vtkIdType cell = 0; cell < numCells; cell++)
  {
    vtkparticles->InsertNextCell(VTK_VERTEX, 1, &cell);
  }
}
