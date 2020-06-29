/*=========================================================================

  Program:   ParaView
  Module:    ParticleAdaptor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ParticleAdaptor.h"

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkParticlePipeline.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

namespace
{
vtkMPIController* controller = 0;
vtkParticlePipeline* pipeline = 0;
vtkCPProcessor* coProcessor = 0;
vtkCPDataDescription* coProcessorData = 0;
}

void coprocessorinitialize(void* handle)
{
  if (!controller)
  {
    controller = vtkMPIController::New();
    controller->Initialize(0, 0, 1);
    vtkMultiProcessController::SetGlobalController(controller);
  }

  if (handle != NULL)
  {
    vtkMPICommunicatorOpaqueComm mpicomm((MPI_Comm*)handle);
    vtkMPICommunicator* comm = vtkMPICommunicator::New();
    comm->InitializeExternal(&mpicomm);
    controller->SetCommunicator(comm);
    comm->Delete();
  }

  if (!pipeline)
  {
    pipeline = vtkParticlePipeline::New();
  }

  if (!coProcessor)
  {
    coProcessor = vtkCPProcessor::New();
    coProcessor->Initialize();
    coProcessor->AddPipeline(pipeline);
  }

  if (!coProcessorData)
  {
    coProcessorData = vtkCPDataDescription::New();
    coProcessorData->AddInput("input");
  }
}

void coprocessorcreateimage(int timestep, double time, char* filename, int n, double* xyz,
  double* bounds, double r, double* attr, double min, double max, double theta, double phi,
  double z)
{
  if (!coProcessor || !coProcessorData)
  {
    cerr << "CoProcessor has not been properly initialized" << endl;
    return;
  }

  coProcessorData->SetTimeData(time, timestep);
  if (coProcessor->RequestDataDescription(coProcessorData))
  {
    vtkUnstructuredGrid* grid = vtkUnstructuredGrid::New();
    coProcessorData->GetInputDescriptionByName("input")->SetGrid(grid);
    grid->Delete();

    vtkPoints* points = vtkPoints::New();
    points->SetNumberOfPoints(n);
    grid->SetPoints(points);
    points->Delete();

    vtkDoubleArray* coords = vtkDoubleArray::New();
    coords->SetNumberOfComponents(3);
    coords->SetNumberOfTuples(n);
    coords->SetArray(xyz, n * 3, 1);
    points->SetData(coords);
    coords->Delete();

    for (vtkIdType i = 0; i < n; i++)
    {
      grid->InsertNextCell(VTK_VERTEX, (vtkIdType)1, &i);
    }

    vtkDoubleArray* attribute = vtkDoubleArray::New();
    attribute->SetName("Attribute");
    attribute->SetNumberOfComponents(1);
    attribute->SetNumberOfTuples(n);
    attribute->SetArray(attr, n, 1);
    grid->GetPointData()->AddArray(attribute);
    attribute->Delete();

    pipeline->SetFilename(filename);
    pipeline->SetParticleRadius(r);
    pipeline->SetCameraThetaAngle(theta);
    pipeline->SetCameraPhiAngle(phi);
    pipeline->SetCameraDistance(z);
    pipeline->SetBounds(bounds);
    pipeline->SetAttributeMinimum(min);
    pipeline->SetAttributeMaximum(max);

    coProcessor->CoProcess(coProcessorData);
  }
}

void coprocessorfinalize()
{
  if (coProcessorData)
  {
    coProcessorData->Delete();
    coProcessorData = 0;
  }
  if (coProcessor)
  {
    coProcessor->Delete();
    coProcessor = 0;
  }
  if (pipeline)
  {
    pipeline->Delete();
    pipeline = 0;
  }
  if (controller)
  {
    controller->Finalize(1);
    controller->Delete();
    controller = 0;
    vtkMultiProcessController::SetGlobalController(0);
  }
}
