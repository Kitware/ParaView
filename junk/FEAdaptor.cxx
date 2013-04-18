#include <iostream>
#include "FEAdaptor.h"
#include "FEDataStructures.h"

#include <vtkCellData.h>
#include <vtkCellType.h>
#include <vtkCPDataDescription.h>
#include <vtkCPInputDataDescription.h>
#include <vtkCPPythonProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkIdTypeArray.h>

namespace
{
  vtkCPPythonProcessor* Processor = NULL;
  vtkUnstructuredGrid* VTKGrid;

  void BuildVTKGrid(Grid& grid)
  {
    // create the points information
    vtkNew<vtkDoubleArray> pointArray;
    pointArray->SetNumberOfComponents(3);
    double coords[3] = {0,0,0};
    pointArray->InsertNextTupleValue(coords);
    coords[0] = 1;
    pointArray->InsertNextTupleValue(coords);
    coords[1] = 1;
    pointArray->InsertNextTupleValue(coords);
    coords[0] = 0;
    pointArray->InsertNextTupleValue(coords);
    coords[2] = 1;
    coords[1] = 0;
    pointArray->InsertNextTupleValue(coords);
    coords[0] = 1;
    pointArray->InsertNextTupleValue(coords);
    coords[1] = 1;
    pointArray->InsertNextTupleValue(coords);
    coords[0] = 0;
    pointArray->InsertNextTupleValue(coords);


    vtkNew<vtkPoints> points;
    points->SetData(pointArray.GetPointer());
    VTKGrid->SetPoints(points.GetPointer());

    // create the cells
    vtkNew<vtkCellArray> cellArray;
    vtkNew<vtkIdTypeArray> offsets;
    vtkNew<vtkUnsignedCharArray> types;
    vtkIdType ids[8];
    // create a triangle
    ids[0] = 0; ids[1] = 1; ids[2] = 2;
    cellArray->InsertNextCell(3, ids);
    offsets->InsertNextValue(0);
    types->InsertNextValue(VTK_TRIANGLE);
    // create a quad
    ids[0] = 0; ids[1] = 1; ids[2] = 2; ids[3] = 3;
    cellArray->InsertNextCell(4, ids);
    offsets->InsertNextValue(4);
    types->InsertNextValue(VTK_QUAD);
    // create a tet
    ids[0] = 0; ids[1] = 1; ids[2] = 2; ids[3] = 4;
    cellArray->InsertNextCell(4, ids);
    offsets->InsertNextValue(9);
    types->InsertNextValue(VTK_TETRA);
    // create a hex
    ids[0] = 0; ids[1] = 1; ids[2] = 2; ids[3] = 3;
    ids[4] = 4; ids[5] = 5; ids[6] = 6; ids[7] = 7;
    cellArray->InsertNextCell(8, ids);
    offsets->InsertNextValue(14);
    types->InsertNextValue(VTK_HEXAHEDRON);

    VTKGrid->SetCells(types.GetPointer(), offsets.GetPointer(), cellArray.GetPointer());
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
  }
}

namespace FEAdaptor
{

  void Initialize(int numScripts, char* scripts[])
  {
    if(Processor == NULL)
      {
      Processor = vtkCPPythonProcessor::New();
      Processor->Initialize();
      }
    else
      {
      Processor->RemoveAllPipelines();
      }
    for(int i=1;i<numScripts;i++)
      {
      vtkNew<vtkCPPythonScriptPipeline> pipeline;
      pipeline->Initialize(scripts[i]);
      Processor->AddPipeline(pipeline.GetPointer());
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

  void CoProcess(Grid& grid, Attributes& attributes, double time,
                 unsigned int timeStep, bool lastTimeStep)
  {
    vtkNew<vtkCPDataDescription> dataDescription;
    dataDescription->AddInput("input");
    dataDescription->SetTimeData(time, timeStep);
    if(lastTimeStep == true)
      {
      // assume that we want to all the pipelines to execute if it
      // is the last time step.
      dataDescription->ForceOutputOn();
      }
    if(Processor->RequestDataDescription(dataDescription.GetPointer()) != 0)
      {
      BuildVTKDataStructures(grid, attributes);
      dataDescription->GetInputDescriptionByName("input")->SetGrid(VTKGrid);
      Processor->CoProcess(dataDescription.GetPointer());
      }
  }
} // end of Catalyst namespace
