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
#include "vtkPEquivalenceSet.h"
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtksys/SystemTools.hxx"

#include <list>

vtkStandardNewMacro (vtkAMRConnectivity);


static const int BOUNDARY_TAG = 39857089;

//-----------------------------------------------------------------------------
// Simple containers for managing asynchronous communication.
struct vtkAMRConnectivityCommRequest
{
  vtkMPICommunicator::Request Request;
  vtkSmartPointer<vtkIntArray> Buffer;
  int SendProcess;
  int ReceiveProcess;
};

// This class is a STL list of vtkAMRConnectivityCommRequest structs with some
// helper methods added.
class vtkAMRConnectivityCommRequestList
  : public std::list<vtkAMRConnectivityCommRequest>
{
public:
  // Description:
  // Waits for all of the communication to complete.
  void WaitAll()
  {
    for (iterator i = this->begin(); i != this->end(); i++) i->Request.Wait();
  }
  // Description:
  // Waits for one of the communications to complete, removes it from the list,
  // and returns it.
  value_type WaitAny()
  {
    while (!this->empty())
      {
      for (iterator i = this->begin(); i != this->end(); i++)
        {
        if (i->Request.Test())
          {
          value_type retval = *i;
          this->erase(i);
          return retval;
          }
        }
      vtksys::SystemTools::Delay(1);
      }
    vtkGenericWarningMacro(<< "Nothing to wait for.");
    return value_type();
  }
};



//-----------------------------------------------------------------------------
vtkAMRConnectivity::vtkAMRConnectivity ()
{
  this->VolumeFractionSurfaceValue = 0.5;
  this->Helper = 0;
  this->Equivalence = 0;
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
    if (this->DoRequestData (amrOutput, this->VolumeArrays[i].c_str()) == 0) 
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
  vtkMPIController *mpiController = vtkMPIController::SafeDownCast(controller);
  int myProc = controller->GetLocalProcessId ();
  int numProcs = controller->GetNumberOfProcesses ();

  this->RegionName = std::string ("RegionId-");
  this->RegionName += volumeName;

  // initialize with a global unique region id
  this->NextRegionId = myProc+1;

  // Find the block local fragments
  vtkCompositeDataIterator* iter = volume->NewIterator ();
  for (iter->InitTraversal (); !iter->IsDoneWithTraversal (); iter->GoToNextItem ())
    {
    // Go through each block and create an array RegionId set all zero.
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast (iter->GetCurrentDataObject ());
    if (!grid)
      {
      vtkErrorMacro ("NonOverlappingAMR not made up of UniformGrids");
      return 0;
      }
    vtkIdTypeArray* regionId = vtkIdTypeArray::New ();
    regionId->SetName (this->RegionName.c_str());
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

    for (int i = extents[0]; i < extents[1]; i ++) 
      {
      for (int j = extents[2]; j < extents[3]; j ++) 
        {
        for (int k = extents[4]; k < extents[5]; k ++) 
          {
          int ijk[3] = { i, j, k };
          vtkIdType cellId = grid->ComputeCellId (ijk);
          if (regionId->GetTuple1 (cellId) == 0 
              && volArray->GetTuple1 (cellId) > this->VolumeFractionSurfaceValue
              && ghostLevels->GetTuple1 (cellId) < 0.5)
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

  if (this->ResolveBlocks)
    {
    // Determine boundaries at the block that need to be sent to neighbors
    this->BoundaryArrays.resize (numProcs);
    this->ReceiveList.resize (numProcs);
    for (int level = 0; level < this->Helper->GetNumberOfLevels (); level ++)
      {
      for (int blockId = 0; blockId < this->Helper->GetNumberOfBlocksInLevel (level); blockId ++) 
        {
        vtkAMRDualGridHelperBlock* block = this->Helper->GetBlock (level, blockId);
        for (int dir = 0; dir < 3; dir ++)
          {
          vtkAMRDualGridHelperBlock* neighbor = this->GetBlockNeighbor (block, dir);
          if (neighbor != 0)
            {
            this->ProcessBoundaryAtBlock (volume, block, neighbor, dir);
            if (neighbor->Level > block->Level) 
              {
              int index[3] = { neighbor->GridIndex[0], neighbor->GridIndex[1], neighbor->GridIndex[2] };
  
              index[(dir+1) % 3] ++;
              neighbor = this->Helper->GetBlock (neighbor->Level, index[0], index[1], index[2]);
              this->ProcessBoundaryAtBlock (volume, block, neighbor, dir);
  
              index[(dir+2) % 3] ++;
              neighbor = this->Helper->GetBlock (neighbor->Level, index[0], index[1], index[2]);
              this->ProcessBoundaryAtBlock (volume, block, neighbor, dir);
  
              index[(dir+1) % 3] --;
              neighbor = this->Helper->GetBlock (neighbor->Level, index[0], index[1], index[2]);
              this->ProcessBoundaryAtBlock (volume, block, neighbor, dir);
              }
            }
          }
        }
      }
  
    // Exchange all boundaries between processes where block and neighbor are different procs
    if (numProcs > 1  && !this->ExchangeBoundaries (mpiController))
      {
      return 0;
      }

    // Process all boundaries at the neighbors to find the equivalence pairs at the boundaries
    this->Equivalence = vtkPEquivalenceSet::New ();
    for (int i = 0; i < this->BoundaryArrays.size (); i ++) 
      {
      for (int j = 0; j < this->BoundaryArrays[i].size (); j ++)
        {
        if (i == myProc)
          {
          this->ProcessBoundaryAtNeighbor (volume, this->BoundaryArrays[myProc][j]);
          }
        this->BoundaryArrays[i][j]->Delete ();
        }
        this->BoundaryArrays[i].clear ();
      }
  
    // Reduce all equivalence pairs into equivalence sets
    this->Equivalence->ResolveEquivalences ();
  
    // Relabel all fragment IDs with the equivalence set number 
    // (set numbers start with 1 and 0 is considered "no set" or "no fragment")
    int maxSet = 0;
    for (iter->InitTraversal (); !iter->IsDoneWithTraversal (); iter->GoToNextItem ())
      {
      vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast (iter->GetCurrentDataObject ());
      vtkIdTypeArray* regionIdArray = vtkIdTypeArray::SafeDownCast (
                                        grid->GetCellData ()->GetArray (this->RegionName.c_str()));
      for (int i = 0; i < regionIdArray->GetNumberOfTuples (); i ++) 
        {
        vtkIdType regionId = regionIdArray->GetTuple1 (i);
        if (regionId > 0) 
          {
          int setId = this->Equivalence->GetEquivalentSetId (regionId);
          regionIdArray->SetTuple1 (i, setId + 1);
          if ((setId + 1) > maxSet) 
            {
            maxSet = setId + 1;
            }
          }
        else
          {
          regionIdArray->SetTuple1 (i, 0);
          }
        }
      }
    this->Equivalence->Delete ();
    }


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

  vtkSmartPointer<vtkIdList> ptIds = vtkIdList::New ();
  vtkSmartPointer<vtkIdList> cellIds = vtkIdList::New ();

  while (todoList->GetNumberOfIds () > 0)
    {
    vtkIdType cellId = todoList->GetId (0);
    todoList->DeleteId (cellId);
    regionId->SetTuple1 (cellId, this->NextRegionId);

    grid->GetCellPoints (cellId, ptIds);
    for (int i = 0; i < ptIds->GetNumberOfIds (); i ++)
      {
      cellIds->SetNumberOfIds (0);
      grid->GetPointCells (ptIds->GetId (i), cellIds);
      for (int j = 0; j < cellIds->GetNumberOfIds (); j ++)
        {
        vtkIdType neighbor = cellIds->GetId (j);
        if (neighbor == cellId || todoList->IsId (neighbor) >= 0) { continue; }
        if (regionId->GetTuple1 (neighbor) == 0 
            && volArray->GetTuple1 (neighbor) > this->VolumeFractionSurfaceValue
            && ghostLevels->GetTuple1 (neighbor) < 0.5)
          {
          todoList->InsertNextId (neighbor);
          }
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkAMRDualGridHelperBlock* vtkAMRConnectivity::GetBlockNeighbor (
                       vtkAMRDualGridHelperBlock* block, 
                       int dir)
{
  int gridIndex[3] = { 
          block->GridIndex[0], 
          block->GridIndex[1],
          block->GridIndex[2] };
  gridIndex[dir] ++;

  vtkAMRDualGridHelperBlock* neighbor = 0;
  neighbor = this->Helper->GetBlock (block->Level, gridIndex[0], gridIndex[1], gridIndex[2]);
  if (neighbor != 0)
    {
    return neighbor;
    }
  neighbor = this->Helper->GetBlock (block->Level-1, gridIndex[0] >> 1, gridIndex[1] >> 1, gridIndex[2] >> 1);
  if (neighbor != 0)
    {
    return neighbor;
    }
  // It's only the 0,0,0 corner block of the set.  
  // We'll need 3 more +1 blocks depending on direction
  // e.g., 0, 0, 1  and 0, 1, 0 and 0, 1, 1  If it's in the x direction.
  neighbor = this->Helper->GetBlock (block->Level+1, gridIndex[0] << 1, gridIndex[1] << 1, gridIndex[2] << 1);
  if (neighbor != 0)
    {
    return neighbor;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkAMRConnectivity::ProcessBoundaryAtBlock (
                       vtkNonOverlappingAMR* volume,
                       vtkAMRDualGridHelperBlock* block, 
                       vtkAMRDualGridHelperBlock* neighbor, 
                       int dir)
{
  if (block == 0 || neighbor == 0)
    {
    vtkErrorMacro ("Cannot process undefined blocks");
    return;
    }

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController ();
  int myProc = controller->GetLocalProcessId ();
  int numProcs = controller->GetNumberOfProcesses ();

  if (block->ProcessId != myProc && neighbor->ProcessId != myProc)
    {
    return;
    }

  if (block->ProcessId == myProc) 
    {
    vtkUniformGrid* grid = volume->GetDataSet (block->Level, block->BlockId);
    vtkIdTypeArray* regionId = vtkIdTypeArray::SafeDownCast (
                                  grid->GetCellData ()->GetArray (this->RegionName.c_str()));

    int extent[6];
    grid->GetExtent (extent);

    // move inside the overlap areas.
    extent[dir*2+1] --; 

    // we only want the plane at the edge in the direction
    extent[dir*2] = extent[dir*2+1] - 1;

    if (block->Level < neighbor->Level) 
      {
      // we only want the quarter of the boundary that applies to this neighbor
      int del[3];
      del[0] = (extent[1] - extent[0]) >> 1;
      del[1] = (extent[3] - extent[2]) >> 1;
      del[2] = (extent[5] - extent[4]) >> 1;

      for (int i = 0; i < 3; i ++)
        {
        if (i == dir) 
          { 
          continue; 
          }
        // shrink the orthogonal directions by half depending on which offset the neighbor is
        if ((neighbor->GridIndex[i] % 2) == 0) 
          {
            extent[i*2+1] -= del[i];
          } else {
            extent[i*2+0] += del[i];
          }
        }
      }

    vtkIdTypeArray* array = vtkIdTypeArray::New ();
    array->SetNumberOfComponents (1);
    array->SetNumberOfTuples (9 + 
                              (extent[1] - extent[0]) *
                              (extent[3] - extent[2]) *
                              (extent[5] - extent[4]));

    int index = 0;
    array->SetTuple1 (index, block->Level);
    index ++;
    array->SetTuple1 (index, block->GridIndex[0]);
    index ++;
    array->SetTuple1 (index, block->GridIndex[1]);
    index ++;
    array->SetTuple1 (index, block->GridIndex[2]);
    index ++;
    array->SetTuple1 (index, dir);
    index ++;
    array->SetTuple1 (index, neighbor->Level);
    index ++;
    array->SetTuple1 (index, neighbor->GridIndex[0]);
    index ++;
    array->SetTuple1 (index, neighbor->GridIndex[1]);
    index ++;
    array->SetTuple1 (index, neighbor->GridIndex[2]);
    index ++;

    if (block->Level > neighbor->Level)
      { 
      int ijk[3];
      for (ijk[0] = extent[0]; ijk[0] < extent[1]; ijk[0] += 2)
        {
        for (ijk[1] = extent[2]; ijk[1] < extent[3]; ijk[1] += 2)
          {
          for (ijk[2] = extent[4]; ijk[2] < extent[5]; ijk[2] += 2)
            {
            vtkIdType cellId = grid->ComputeCellId (ijk); 
            int region = regionId->GetTuple1 (cellId);
            int setting = 1;
            // if this cell doesn't have a region
            // check one of the other 3 that align with the neighbor cell
            while (region == 0 && setting <= 8) 
              {
              int ijk_off[3];
              ijk_off[0] = ijk[0];
              if (setting & 1 == 1 && extent[1] != extent[0]) 
                {
                ijk_off[0] ++;
                } 
              ijk_off[1] = ijk[1];
              if (setting & 2 == 1 && extent[3] != extent[2]) 
                {
                ijk_off[1] ++;
                } 
              ijk_off[2] = ijk[2];
              if (setting & 4 == 1 && extent[5] != extent[4]) 
                {
                ijk_off[2] ++;
                } 
              cellId = grid->ComputeCellId (ijk_off); 
              region = regionId->GetTuple1 (cellId);
              setting ++;
              }
            array->SetTuple1 (index, region);
            index ++;
            }
          }
      }
      }
    else
      {
      int ijk[3];
      for (ijk[0] = extent[0]; ijk[0] < extent[1]; ijk[0] ++)
        {
        for (ijk[1] = extent[2]; ijk[1] < extent[3]; ijk[1] ++)
          {
          for (ijk[2] = extent[4]; ijk[2] < extent[5]; ijk[2] ++)
            {
            vtkIdType cellId = grid->ComputeCellId (ijk); 
            array->SetTuple1 (index, regionId->GetTuple1 (cellId));
            index ++;
            }
          }
        }
      }

    this->BoundaryArrays[neighbor->ProcessId].push_back (array);
    }
  else if (neighbor->ProcessId == myProc)
    {
    vtkUniformGrid* grid = volume->GetDataSet (neighbor->Level, neighbor->BlockId);
    // estimate the size of the receive by the extent of this neighbor
    int extent[6];
    grid->GetExtent (extent);
    extent[dir*2 + 1] = extent[dir*2] + 2;

    this->ReceiveList[block->ProcessId].push_back (9 + 
                                                   (extent[1] - extent[0]) *
                                                   (extent[3] - extent[2]) *
                                                   (extent[5] - extent[4]));
    }
}
//----------------------------------------------------------------------------
int vtkAMRConnectivity::ExchangeBoundaries (vtkMPIController *controller)
{
  if (controller == 0)
    {
    vtkErrorMacro ("vtkAMRConnectivity only works parallel in MPI environment");
    return 0;
    }

  int myProc = controller->GetLocalProcessId ();
  int numProcs = controller->GetNumberOfProcesses ();

  vtkAMRConnectivityCommRequestList receiveList;
  for (int i = 0; i < this->ReceiveList.size (); i ++) 
    {
    if (i == myProc) { continue; }

    int messageLength = 0; 
    for (int j = 0; j < this->ReceiveList[i].size (); j ++)
      {
      messageLength += 1 + this->ReceiveList[i][j];
      }
    if (messageLength == 0) { continue; }
    messageLength ++; // end of message marker

    vtkIntArray* array = vtkIntArray::New ();
    array->SetNumberOfComponents (1);
    array->SetNumberOfTuples (messageLength);

    vtkAMRConnectivityCommRequest request;
    request.SendProcess = i;
    request.ReceiveProcess = myProc;
    request.Buffer = array;

    controller->NoBlockReceive(array->GetPointer (0),
                               messageLength,
                               i, BOUNDARY_TAG,
                               request.Request);

    this->ReceiveList[i].clear ();
    receiveList.push_back(request);
    array->Delete ();
    } 

  vtkAMRConnectivityCommRequestList sendList;
  for (int i = 0; i < this->BoundaryArrays.size (); i ++)
    {
    if (i == myProc) { continue; }
    vtkIntArray* array = vtkIntArray::New ();
    array->SetNumberOfComponents (1);
    array->SetNumberOfTuples (0);
    for (int j = 0; j < this->BoundaryArrays[i].size (); j ++)
      {
      int tuples = this->BoundaryArrays[i][j]->GetNumberOfTuples ();
      array->InsertNextTuple1 (tuples);
      for (int k = 0; k < tuples; k ++) 
        {
        array->InsertNextTuple1 (this->BoundaryArrays[i][j]->GetTuple1 (k));
        }
      }

    if (array->GetNumberOfTuples () == 0) { continue; }
    
    array->InsertNextTuple1 (-1);

    vtkAMRConnectivityCommRequest request;
    request.SendProcess = myProc;
    request.ReceiveProcess = i;
    request.Buffer = array;

    controller->NoBlockSend(array->GetPointer(0),
                            array->GetNumberOfTuples (),
                            i, BOUNDARY_TAG,
                            request.Request);

    sendList.push_back(request);
    array->Delete ();
    }

  while (!receiveList.empty())
    {
    vtkAMRConnectivityCommRequest request = receiveList.WaitAny();
    vtkIntArray* array = request.Buffer;
    int total = array->GetNumberOfTuples ();
    int index = 0;
    while (index < total)
      {
      int tuples = array->GetTuple1 (index);
      if (tuples < 0) 
        {
        break;
        }
      index ++;
      vtkIdTypeArray* boundary = vtkIdTypeArray::New ();
      boundary->SetNumberOfComponents (1);
      boundary->SetNumberOfTuples (tuples);
      for (int i = 0; i < tuples; i ++) 
        {
        boundary->SetTuple1 (i, array->GetTuple1 (index));
        index ++;
        }
      this->BoundaryArrays[myProc].push_back (boundary);
      }
    }

  sendList.WaitAll();
  return 1;
}

//----------------------------------------------------------------------------
void vtkAMRConnectivity::ProcessBoundaryAtNeighbor (
                vtkNonOverlappingAMR* volume, vtkIdTypeArray *array)
{

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController ();
  int myProc = controller->GetLocalProcessId ();
  int numProcs = controller->GetNumberOfProcesses ();

  int index = 0;
  int blockLevel = array->GetTuple1 (index);
  index ++;
  int blockGrid[3];
  blockGrid[0] = array->GetTuple1 (index);
  index ++;
  blockGrid[1] = array->GetTuple1 (index);
  index ++;
  blockGrid[2] = array->GetTuple1 (index);
  index ++;
  int dir = array->GetTuple1 (index);
  index ++;
  int neighborLevel = array->GetTuple1 (index);
  index ++;
  int gridX = array->GetTuple1 (index);
  index ++;
  int gridY = array->GetTuple1 (index);
  index ++;
  int gridZ = array->GetTuple1 (index);
  index ++;

  vtkAMRDualGridHelperBlock* neighbor = 
          this->Helper->GetBlock (neighborLevel, gridX, gridY, gridZ);

  if (neighbor->ProcessId != myProc) 
    {
    vtkErrorMacro ("Shouldn't get here.  Cannot process neighbor on remote process");
    return;
    }

  vtkUniformGrid* grid = volume->GetDataSet (neighbor->Level, neighbor->BlockId);
  vtkIdTypeArray* regionId = vtkIdTypeArray::SafeDownCast (
                                grid->GetCellData ()->GetArray (this->RegionName.c_str()));

  int extent[6];
  grid->GetExtent (extent);

  // move inside the overlap areas.
  extent[dir*2] ++; 

  // we only want the plane at the edge in dir
  extent[dir*2+1] = extent[dir*2] + 1;

  if (blockLevel > neighborLevel) 
    {
    // we only want the quarter of the boundary that applies to this neighbor
    int del[3];
    del[0] = (extent[1] - extent[0]) >> 1;
    del[1] = (extent[3] - extent[2]) >> 1;
    del[2] = (extent[5] - extent[4]) >> 1;

    for (int i = 0; i < 3; i ++)
      {
      if (i == dir) 
        { 
        continue; 
        }
      // shrink the orthogonal directions by half depending on which offset the neighbor is
      // TODO need the blocks offset within the octet
      if ((blockGrid[i] % 2) == 0) 
        {
          extent[i*2+1] -= del[i];
        } else {
          extent[i*2+0] += del[i];
        }
      }
    }

  if (blockLevel < neighborLevel)
    {
    int ijk[3];
    for (ijk[0] = extent[0]; ijk[0] < extent[1]; ijk[0] += 2)
      {
      for (ijk[1] = extent[2]; ijk[1] < extent[3]; ijk[1] += 2)
        {
        for (ijk[2] = extent[4]; ijk[2] < extent[5]; ijk[2] += 2)
          {
          vtkIdType cellId = grid->ComputeCellId (ijk); 
          int neighborRegion = regionId->GetTuple1 (cellId);
          int setting = 1;
          // if this cell doesn't have a region
          // check one of the other 3 that align with the neighbor cell
          while (neighborRegion == 0 && setting <= 8) 
            {
            int ijk_off[3];
            ijk_off[0] = ijk[0];
            if (setting & 1 == 1 && extent[1] != extent[0]) 
              {
              ijk_off[0] ++;
              } 
            ijk_off[1] = ijk[1];
            if (setting & 2 == 1 && extent[3] != extent[2]) 
              {
              ijk_off[1] ++;
              } 
            ijk_off[2] = ijk[2];
            if (setting & 4 == 1 && extent[5] != extent[4]) 
              {
              ijk_off[2] ++;
              } 
            cellId = grid->ComputeCellId (ijk_off); 
            neighborRegion = regionId->GetTuple1 (cellId);
            setting ++;
            }
          int blockRegion = array->GetTuple1 (index);
          if (neighborRegion != 0 && blockRegion != 0) 
            {
	    int neighborRef = this->Equivalence->GetReference (neighborRegion);
            int blockRef = this->Equivalence->GetReference (blockRegion);
            this->Equivalence->AddEquivalence (neighborRegion, blockRegion);
            }
          }
          index ++;
        }
      }
    }
  else /* blockLevel >= neighborLevel */
    {
    int ijk[3];
    for (ijk[0] = extent[0]; ijk[0] < extent[1]; ijk[0] ++)
      {
      for (ijk[1] = extent[2]; ijk[1] < extent[3]; ijk[1] ++)
        {
        for (ijk[2] = extent[4]; ijk[2] < extent[5]; ijk[2] ++)
          {
          vtkIdType cellId = grid->ComputeCellId (ijk); 
          int neighborRegion = regionId->GetTuple1 (cellId);
          int blockRegion = array->GetTuple1 (index);
          if (neighborRegion != 0 && blockRegion != 0) 
            {
	    int neighborRef = this->Equivalence->GetReference (neighborRegion);
            int blockRef = this->Equivalence->GetReference (blockRegion);
            this->Equivalence->AddEquivalence (neighborRegion, blockRegion);
            }
          index ++;
          }
        }
      }
    }
}
