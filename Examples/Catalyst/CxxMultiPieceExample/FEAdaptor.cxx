#include "FEAdaptor.h"
#include "FEDataStructures.h"
#include <iostream>

#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPProcessor.h>
#include <vtkCPPythonPipeline.h>
#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkCommunicator.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkLogger.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiPieceDataSet.h>
#include <vtkMultiProcessController.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>

#include <vtk_mpi.h>

namespace
{
vtkCPProcessor* Processor = NULL;
vtkMultiBlockDataSet* VTKGrid;

void BuildVTKGrid(Grid& grid)
{
  vtkNew<vtkImageData> imageData;
  imageData->SetSpacing(grid.GetSpacing());
  imageData->SetOrigin(0, 0, 0); // Not necessary for (0,0,0)
  unsigned int* extents = grid.GetExtents();
  int extents2[6];
  for (int i = 0; i < 6; i++)
  {
    extents2[i] = static_cast<int>(extents[i]);
  }
  imageData->SetExtent(extents2);
  int mpiSize = 1;
  int mpiRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  vtkNew<vtkMultiPieceDataSet> multiPiece;
  multiPiece->SetNumberOfPieces(mpiSize);
  multiPiece->SetPiece(mpiRank, imageData);
  VTKGrid->SetNumberOfBlocks(1);
  VTKGrid->SetBlock(0, multiPiece);
}

void UpdateVTKAttributes(Grid& grid, Attributes& attributes, vtkCPInputDataDescription* idd)
{
  int mpiRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  vtkMultiPieceDataSet* multiPiece = vtkMultiPieceDataSet::SafeDownCast(VTKGrid->GetBlock(0));
  if (idd->IsFieldNeeded("velocity", vtkDataObject::POINT))
  {
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(multiPiece->GetPiece(mpiRank));
    if (dataSet->GetPointData()->GetNumberOfArrays() == 0)
    {
      // velocity array
      vtkNew<vtkDoubleArray> velocity;
      velocity->SetName("velocity");
      velocity->SetNumberOfComponents(3);
      velocity->SetNumberOfTuples(static_cast<vtkIdType>(grid.GetNumberOfLocalPoints()));
      dataSet->GetPointData()->AddArray(velocity);
    }
    vtkDoubleArray* velocity =
      vtkDoubleArray::SafeDownCast(dataSet->GetPointData()->GetArray("velocity"));
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
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(multiPiece->GetPiece(mpiRank));
    if (dataSet->GetCellData()->GetNumberOfArrays() == 0)
    {
      // pressure array
      vtkNew<vtkFloatArray> pressure;
      pressure->SetName("pressure");
      pressure->SetNumberOfComponents(1);
      dataSet->GetCellData()->AddArray(pressure);
    }
    vtkFloatArray* pressure =
      vtkFloatArray::SafeDownCast(dataSet->GetCellData()->GetArray("pressure"));
    // The pressure array is a scalar array so we can reuse
    // memory as long as we ordered the points properly.
    float* pressureData = attributes.GetPressureArray();
    pressure->SetArray(pressureData, static_cast<vtkIdType>(grid.GetNumberOfLocalCells()), 1);
  }
}

void BuildVTKDataStructures(Grid& grid, Attributes& attributes, vtkCPInputDataDescription* idd)
{
  if (VTKGrid == NULL)
  {
    // The grid structure isn't changing so we only build it
    // the first time it's needed. If we needed the memory
    // we could delete it and rebuild as necessary.
    VTKGrid = vtkMultiBlockDataSet::New();
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
