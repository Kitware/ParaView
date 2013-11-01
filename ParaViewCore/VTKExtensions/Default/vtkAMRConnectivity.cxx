/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRConnectivity.cxx

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
#include "vtkAMRConnectivity.h"

#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkAMRDualGridHelper.h"
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"

vtkStandardNewMacro (vtkAMRConnectivity);

vtkAMRConnectivity::vtkAMRConnectivity ()
{
  this->VolumeFractionSurfaceValue = 0.5;
  this->Helper = 0;
}

vtkAMRConnectivity::~vtkAMRConnectivity ()
{
}

void vtkAMRConnectivity::PrintSelf (ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf (os, indent);
  os << indent << "VolumeFractionSurfaceValue: " << this->VolumeFractionSurfaceValue << endl;
}

void vtkAMRConnectivity::AddInputVolumeArrayToProcess (const char *name) 
{
  this->VolumeArrays.push_back (name);
  this->Modified();
}

void vtkAMRConnectivity::ClearInputVolumeArrayToProcess ()
{
  this->VolumeArrays.clear ();
  this->Modified();
}

int vtkAMRConnectivity::FillInputPortInformation (int port, vtkInformation *info)
{
  switch (port) 
    {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkNonOverlappingAMR");
      break;
    default:
      return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRConnectivity::FillOutputPortInformation(int port, vtkInformation *info)
{
  switch (port)
    {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkNonOverlappingAMR");
      break;
    default:
      return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRConnectivity::RequestData (vtkInformation* vtkNotUsed(request),
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkNonOverlappingAMR* amrInput=vtkNonOverlappingAMR::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkNonOverlappingAMR* amrOutput = vtkNonOverlappingAMR::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  amrOutput->ShallowCopy (amrInput);

  this->Helper = vtkAMRDualGridHelper::New ();
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController ();
  this->Helper->SetController(controller);
  this->Helper->Initialize(amrInput);

  unsigned int noOfArrays = static_cast<unsigned int>(this->VolumeArrays.size());
  for(unsigned int i = 0; i < noOfArrays; i++)
    {
    if (this->DoRequestData (amrInput, this->VolumeArrays[i].c_str()) == 0) 
      {
      return 0;
      }
    }
  this->Helper->Delete ();

  return 1;
}


//----------------------------------------------------------------------------
int vtkAMRConnectivity::DoRequestData (vtkNonOverlappingAMR* volume,
                                      const char* volumeName)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController ();
  int myProc = controller->GetLocalProcessId ();
  int numProcs = controller->GetNumberOfProcesses ();

  // initialize with a global unique region id
  this->NextRegionId = myProc+1;

  vtkCompositeDataIterator* iter = volume->NewIterator ();
  for (iter->InitTraversal (); !iter->IsDoneWithTraversal (); iter->GoToNextItem ())
    {
    // Go through each block and create an array "RegionId" set all zero.
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast (iter->GetCurrentDataObject ());
    if (!grid)
      {
      vtkErrorMacro ("NonOverlappingAMR not made up of UniformGrids");
      return 0;
      }
    vtkIdTypeArray* regionId = vtkIdTypeArray::New ();
    regionId->SetName ("RegionId");
    regionId->SetNumberOfComponents (1);
    regionId->SetNumberOfTuples (grid->GetNumberOfCells ());
    for (int i = 0; i < grid->GetNumberOfCells (); i ++) 
      {
      regionId->SetTuple1 (i, 0);
      }
    grid->GetCellData ()->AddArray (regionId);
    regionId->Delete ();

    vtkDataArray* volArray = grid->GetCellData ()->GetArray (volumeName);
    if (!volArray)
      {
      vtkErrorMacro (<< "There is no " << volumeName << " in cell field");
      return 0;
      }
    vtkDataArray* ghostLevels = grid->GetCellData ()->GetArray ("vtkGhostLevels");
    if (!ghostLevels) 
      {
      vtkErrorMacro ("No vtkGhostLevels array attached to the CTH volume data");
      return 0;
      }

    // within each block find all fragments
    int extents[6];
    grid->GetExtent (extents);

    for (int i = extents[0]; i <= extents[1]; i ++) 
      {
      for (int j = extents[2]; j <= extents[3]; j ++) 
        {
        for (int k = extents[4]; k <= extents[5]; k ++) 
          {
          int ijk[3] = { i, j, k };
          vtkIdType cellId = grid->ComputeCellId (ijk);
          if (regionId->GetTuple1 (cellId) == 0 && 
              volArray->GetTuple1 (cellId) > this->VolumeFractionSurfaceValue &&
              ghostLevels->GetTuple1 (cellId) < 0.5)
            {
            // wave propagation sets the region id as it propagates
            this->WavePropagation (cellId, grid, regionId, volArray, ghostLevels);
            // increment this way so the region id remains globally unique
            this->NextRegionId += numProcs;
            }
          }
        }
      }
    }

  

  // Exchange points between blocks for the equivalence set
  // Each axis at a time.  In the positive direction only.
  // Reduce equivalence set.
  // Relabel all RegionIds with lowest equivalence.

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRConnectivity::WavePropagation (vtkIdType cellIdStart, 
                                         vtkUniformGrid* grid, 
                                         vtkIdTypeArray* regionId,
                                         vtkDataArray* volArray,
                                         vtkDataArray* ghostLevels)
{
  vtkSmartPointer<vtkIdList> todoList = vtkIdList::New ();
  todoList->SetNumberOfIds (0);
  todoList->InsertNextId (cellIdStart);

  while (todoList->GetNumberOfIds () > 0)
    {
    vtkIdType cellId = todoList->GetId (0);
    todoList->DeleteId (cellId);
    regionId->SetTuple1 (cellId, this->NextRegionId);
    vtkSmartPointer<vtkIdList> ptIds = vtkIdList::New ();
    vtkSmartPointer<vtkIdList> cellIds = vtkIdList::New ();

    grid->GetCellPoints (cellId, ptIds);
    for (int i = 0; i < ptIds->GetNumberOfIds (); i ++)
      {
      cellIds->SetNumberOfIds (0);
      grid->GetPointCells (ptIds->GetId (i), cellIds);
      for (int j = 0; j < cellIds->GetNumberOfIds (); j ++)
        {
        vtkIdType neighbor = cellIds->GetId (j);
        if (regionId->GetTuple1 (neighbor) == 0 && 
            volArray->GetTuple1 (neighbor) > this->VolumeFractionSurfaceValue &&
            ghostLevels->GetTuple1 (neighbor) < 0.5)
          {
          todoList->InsertNextId (neighbor);
          }
        }
      }
    }
}
