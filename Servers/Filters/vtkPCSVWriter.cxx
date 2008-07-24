/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCSVWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPCSVWriter.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCharArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkPolyLineToRectilinearGridFilter.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPCSVWriter);
vtkCxxRevisionMacro(vtkPCSVWriter, "1.5");

vtkCxxSetObjectMacro(vtkPCSVWriter, Controller, vtkMultiProcessController);

//----------------------------------------------------------------------------
vtkPCSVWriter::vtkPCSVWriter()
{
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkPCSVWriter::~vtkPCSVWriter()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
int vtkPCSVWriter::RequestInformation(vtkInformation *, // request
                                        vtkInformationVector **, // inputVector,
                                        vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkPCSVWriter::AppendCSVDataSet(vtkRectilinearGrid *remoteCSVOutput,
                                     vtkRectilinearGrid* rectilinearGrid)
{
  vtkPointData *remotePointData;
  vtkCellData *remoteCellData;
  vtkIdType k;
  vtkPointData * node0PointData = rectilinearGrid->GetPointData();
  vtkCellData * node0CellData = rectilinearGrid->GetCellData();

  remotePointData = remoteCSVOutput->GetPointData();
  remoteCellData = remoteCSVOutput->GetCellData();

  // Iterate over all point data in the output gathered from the remote
  // and copy array values 

  int dims[3];
  rectilinearGrid->GetDimensions(dims);
  vtkDataArray * XCoords = rectilinearGrid->GetXCoordinates();
  int numXCoords = XCoords->GetNumberOfTuples();
  vtkDataArray * remoteXCoords = remoteCSVOutput->GetXCoordinates();
  int numRemoteXCoords = remoteXCoords->GetNumberOfTuples();
  for (k = 0; k < numRemoteXCoords; k++)
    {
    double remoteX = remoteXCoords->GetTuple1(k);
    XCoords->InsertTuple1(numXCoords++, remoteX);
    }

  rectilinearGrid->SetDimensions(numXCoords, dims[1], dims[2]);
  rectilinearGrid->SetXCoordinates(XCoords);

  int numNode0PointArrays = node0PointData->GetNumberOfArrays();

  // point data
  for (k = 0; k < numNode0PointArrays; k++)
    {
    vtkAbstractArray *node0Array = node0PointData->GetArray(k);
    vtkAbstractArray *remoteArray = remotePointData->GetArray(
      node0Array->GetName());

    int numRemoteArrayDataValues = remoteArray->GetNumberOfTuples();
    if (remoteArray != NULL)
      {
      int j;
      for (j = 0; j < numRemoteArrayDataValues; j++)
        {
        node0Array->InsertNextTuple(j, remoteArray);
        }
      }
    }

  // cell data
  for (k = 0; k < numNode0PointArrays; k++)
    {
    vtkAbstractArray *node0Array = node0CellData->GetArray(k);
    vtkAbstractArray *remoteArray = remoteCellData->GetArray(
      node0Array->GetName());

    int numRemoteArrayDataValues = remoteArray->GetNumberOfTuples();
    if (remoteArray != NULL)
      {
      int j;
      for (j = 0; j < numRemoteArrayDataValues; j++)
        {
        node0Array->InsertNextTuple(j, remoteArray);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPCSVWriter::WriteRectilinearDataInParallel(
  vtkRectilinearGrid* rectilinearGrid)
{
  int procid = 0;
  int numProcs = 1;

  if ( this->Controller )
    {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }

  if (procid == 0)
    {
    WriteRectilinearGridData(rectilinearGrid);
    }


#if 0
  vtkIdType numPoints = rectilinearGrid->GetNumberOfPoints();

  if ( procid )
    {
    // Satellite node
    this->Controller->Send(&numPoints, 1, 0, PCSV_COMMUNICATION_TAG);
    if ( numPoints > 0 )
      {
      this->Controller->Send(rectilinearGrid, 0, PCSV_COMMUNICATION_TAG);
      }
    rectilinearGrid->ReleaseData();
    }
  else if ( numProcs > 1 )
    {
    vtkRectilinearGrid * rectilinearGridCopy = vtkRectilinearGrid::New();
    rectilinearGridCopy->DeepCopy(rectilinearGrid);


    vtkIdType numRemoteValidPoints = 0;

    /* 2/6/2008: JG: it seems like this code is not needed as the
       rectilinear grid has already been gathered from remote processes.
       We'll leave this in place for now, in case it's needed for a situation
       that arises later.
    */

    vtkDataSet *remoteCSVOutput = rectilinearGrid->NewInstance();
    vtkIdType i;
    for (i = 1; i < numProcs; i++)
      {
      this->Controller->Receive(&numRemoteValidPoints, 1, i,
        PCSV_COMMUNICATION_TAG);
      if (numRemoteValidPoints > 0)
        {
        this->Controller->Receive(remoteCSVOutput, i, PCSV_COMMUNICATION_TAG);

        vtkRectilinearGrid * remoteRectilinearGrid =
          vtkRectilinearGrid::SafeDownCast(remoteCSVOutput);
        if (remoteCSVOutput == NULL)
          {
          return;
          }

        AppendCSVDataSet(remoteRectilinearGrid, rectilinearGridCopy);
      
        }
      }
    remoteCSVOutput->Delete();

    if (procid == 0)
      {
      WriteRectilinearGridData(rectilinearGridCopy);
      }

    rectilinearGridCopy->Delete();
    }
  else 
    {
    WriteRectilinearGridData(rectilinearGrid);
    }
#endif
}

//-----------------------------------------------------------------------------
void vtkPCSVWriter::WriteData()
{
  vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(this->GetInput());
  if (rg)
    {
    this->WriteRectilinearDataInParallel(rg);
    }
  else if (vtkPolyData* pd = vtkPolyData::SafeDownCast(this->GetInput()))
    {
    vtkPolyData* clone = vtkPolyData::New();
    clone->ShallowCopy(pd);

    const char* const infoStr =
      "input data type is a vtkPolyData."
      " Converting via vtkPolyLineToRectilinearGridFilter";
    vtkDebugMacro(<< infoStr);
    vtkPolyLineToRectilinearGridFilter* p2rgf =
      vtkPolyLineToRectilinearGridFilter::New();
    p2rgf->SetInput(clone);

    p2rgf->Update();
    this->WriteRectilinearDataInParallel(p2rgf->GetOutput());

    p2rgf->Delete();
    clone->Delete();
    }
  else
    {
    vtkErrorMacro("input data type needs to be of type vtkPolyData or "
        "a vtkRectilinearGrid");
    }
}

//----------------------------------------------------------------------------
void vtkPCSVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "Controller " << this->Controller << endl;
  os << indent << "NumberOfPieces: " << this->NumberOfPieces<< endl;
  os << indent << "Piece: " << this->Piece<< endl;
}
