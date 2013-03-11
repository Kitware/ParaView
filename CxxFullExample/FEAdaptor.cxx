#include <iostream>
#include "FEAdaptor.h"
#include "FEDataStructures.h"

namespace
{
  vtkCPProcessor* Processor = NULL;
  vtkUnstructuredGrid* VTKGrid;

  void BuildVTKGrid(Grid& grid)
  {
    // create the points information
    vtkNew<vtkDoubleArray> pointArray = vtkDoubleArray::New();
    pointArray->SetNumberOfComponents(3);
    pointArray->SetArray(Grid.GetPointsArray(), static_cast<vtkIdType>(Grid.GetNumberOfPoints()*3), 1);
    vtkNew<vtkPoints> points;
    points->SetData(pointArray.GetPointer());
    VTKGrid->SetPoints(points);

    // create the cells
    size_t numCells = Grid.GetNumberOfCells();
    VTKGrid->Allocate(static_cast<vtkIdType>(numCells*9));
    for(size_t cell=0;cell<numCells;cell++)
      {
      unsigned int* cellPoints = Grid.GetCellPoints(cell);
      vtkIdType tmp[8] = {cellPoints[0], cellPoints[1], cellPoints[2], cellPoints[3],
                          cellPoints[4], cellPoints[5], cellPoints[6], cellPoints[7]};
      VTKGrid->InsertNextCell(VTK_HEXAHEDRON, 8, tmp);
      }
  }

  void UpdateVTKAttributes(Grid& grid, Attributes& attributes)
  {
    if(VTKGrid->GetPointData()->GetNumberOfArrays() == 0)
      {
      vtkNew<vtkDoubleArray> velocity;
      velocity->SetName("velocity");
      velocity->SetNumberOfComponents(3);
      velocity->SetNumberOfTuples(static_cast<vtkIdType>(grid.GetNumberOfPoints()));
      VTKGrid->GetPointData()->AddArray(velocity.GetPointer());
      }
    vtkDoubleArray* velocity = vtkDoubleArray::SafeDowncast(
      VTKGrid->GetPointData()->GetArray("velocity"));
    // The velocity array is ordered as vx0,vx1,vx2,..,vy0,vy1,vy2,..,vz0,vz1,vz2,..
    // so we need to create a full copy of it with VTK's ordering of
    // vx0,vy0,vz0,vx1,vy1,vz1,..
    double* velocityData = attributes.GetVelocityArray();
    vtkIdType numTuples = velocity->GetNumberOfTuples();
    for(vtkIdType i=0;i<numTuples;i++)
      {
      double values[3] = {velocityData[i], velocityData[i+numTuples],
                          velocityData[i+2*numTuples]};
      velocity->SetTupleValue(i, values);
      }
  }

  void BuildVTKDataStructures(Grid& grid, Attributes& attributes)
  {
    if(VTKGrid == NULL)
      {
      // The grid structure isn't changing so we only build it
      // the first time it's needed. If we needed the memory
      // we could delete it and rebuild as necessary.
      VTKGrid = vtkUnstructuredGrid::New();
      BuildVTKGrid(grid);
      }
    UpdateVTKAttributes(attributes);
  }
}

namespace Catalyst
{

  void Initialize(int numScripts, const char* scripts[])
  {
    if(Processor == NULL)
      {
      Processor = vtkCPProcessor::New();
      }
    else
      {
      Processor->RemoveAllPipelines();
      }
    for(int i=0;i<numScripts;i++)
      {
      vtkNew<vtkCPPythonScriptPipeline> pipeline;
      pipeline->Initialize(scripts[i]);
      processor->AddPipeline(pipeline.Pointer());
      }
  }

  void Finalize()
  {
    if(Processor)
      {
      Processor->Delete();
      Processor = NULL;
      }
    if(VTKGrid)
      {
      VTKGrid->Delete();
      VTKGrid = NULL;
      }
  }

  void CoProcess(Grid& grid, Attributes& attributes, double time, unsigned int timeStep)
  {
    vtkNew<vtkCPDataDescription> dataDescription;
    dataDescription->AddInput("input");
    dataDescription->SetTimeData(time, timeStep);
    if(Processor->RequestDataDescription(dataDescription) != 0)
      {
      BuildVTKDataStructures(grid, attributes);
      dataDescription->GetInputDataDescriptionByName("input")->SetGrid(VTKGrid);
      Processor->CoProcess(dataDescription);
      }
  }
} // end of Catalyst namespace
