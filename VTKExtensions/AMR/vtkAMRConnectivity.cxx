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

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

#include "vtkAMRDualGridHelper.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtksys/SystemTools.hxx"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#endif

#include <list>
#include <map>

vtkStandardNewMacro(vtkAMRConnectivity);

class vtkAMRConnectivityEquivalence
{
public:
  vtkAMRConnectivityEquivalence()
  {
    set_to_min_id = vtkSmartPointer<vtkIntArray>::New();
    set_to_min_id->SetNumberOfComponents(1);
    set_to_min_id->SetNumberOfTuples(0);
  }

  ~vtkAMRConnectivityEquivalence() = default;

  int AddEquivalence(int id1, int id2)
  {
    int min_id;
    if (id1 < id2)
    {
      min_id = id1;
    }
    else
    {
      min_id = id2;
    }

    std::map<int, int>::iterator iter;

    iter = id_to_set.find(id1);
    int set1 = (iter == id_to_set.end() ? -1 : iter->second);
    iter = id_to_set.find(id2);
    int set2 = (iter == id_to_set.end() ? -1 : iter->second);

    if (set1 >= 0 && set_to_min_id->GetValue(set1) < 0)
    {
      set1 = -1;
    }
    if (set2 >= 0 && set_to_min_id->GetValue(set2) < 0)
    {
      set2 = -1;
    }

    if (set1 >= 0 && set2 >= 0 && set1 == set2)
    {
      return 0;
    }
    else if (set1 >= 0 && set2 >= 0)
    {
      // merge sets
      int min_set, max_set;
      if (set1 < set2)
      {
        min_set = set1;
        max_set = set2;
      }
      else
      {
        min_set = set2;
        max_set = set1;
      }
      for (iter = id_to_set.begin(); iter != id_to_set.end(); iter++)
      {
        if (iter->second == max_set)
        {
          id_to_set[iter->first] = min_set;
        }
      }
      int max_set_min = set_to_min_id->GetValue(max_set);
      int min_set_min = set_to_min_id->GetValue(min_set);
      // pick the smallest of the two mins to represent the set.
      if (max_set_min < min_set_min)
      {
        set_to_min_id->SetValue(min_set, max_set_min);
      }
      set_to_min_id->SetValue(max_set, -1);
    }
    else if (set1 >= 0)
    {
      id_to_set[id2] = set1;
      if (id2 < set_to_min_id->GetValue(set1))
      {
        set_to_min_id->SetValue(set1, id2);
      }
    }
    else if (set2 >= 0)
    {
      id_to_set[id1] = set2;
      if (id1 < set_to_min_id->GetValue(set2))
      {
        set_to_min_id->SetValue(set2, id1);
      }
    }
    else
    {
      // find an empty set.
      int first_empty = -1;
      for (int i = 0; i < set_to_min_id->GetNumberOfTuples(); i++)
      {
        if (set_to_min_id->GetValue(i) < 0)
        {
          first_empty = i;
          break;
        }
      }
      if (first_empty < 0)
      {
        first_empty = set_to_min_id->InsertNextValue(-1);
      }

      id_to_set[id1] = first_empty;
      id_to_set[id2] = first_empty;
      set_to_min_id->SetValue(first_empty, min_id);
      // return zero here because its not values we've previously cared about.
    }
    return 1;
  }

  int GetMinimumSetId(int id)
  {
    std::map<int, int>::iterator iter = id_to_set.find(id);
    if (iter == id_to_set.end())
    {
      // vtkErrorWithObjectMacro (id_to_set, << "ID out of range " << id << " (expected 0 <= x < "
      // << id_to_set->GetNumberOfTuples () << ")");
      return -1;
    }
    int set = id_to_set[id];
    return (set >= 0 ? set_to_min_id->GetValue(set) : -1);
  }

private:
  std::map<int, int> id_to_set;
  vtkSmartPointer<vtkIntArray> set_to_min_id;
};

#if VTK_MODULE_ENABLE_VTK_ParallelMPI

static const int BOUNDARY_TAG = 857089;
static const int EQUIV_SIZE_TAG = 748957;
static const int EQUIV_TAG = 357345;

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
class vtkAMRConnectivityCommRequestList : public std::list<vtkAMRConnectivityCommRequest>
{
public:
  // Description:
  // Waits for all of the communication to complete.
  void WaitAll()
  {
    for (iterator i = this->begin(); i != this->end(); i++)
    {
      i->Request.Wait();
      // this->erase (i);
    }
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
#endif

//-----------------------------------------------------------------------------
vtkAMRConnectivity::vtkAMRConnectivity()
{
  this->VolumeFractionSurfaceValue = 0.5;
  this->Helper = nullptr;
  this->Equivalence = nullptr;
  this->ResolveBlocks = 1;
  this->PropagateGhosts = 0;
}

vtkAMRConnectivity::~vtkAMRConnectivity() = default;

void vtkAMRConnectivity::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VolumeFractionSurfaceValue: " << this->VolumeFractionSurfaceValue << endl;
}

void vtkAMRConnectivity::AddInputVolumeArrayToProcess(const char* name)
{
  this->VolumeArrays.push_back(name);
  this->Modified();
}

void vtkAMRConnectivity::ClearInputVolumeArrayToProcess()
{
  this->VolumeArrays.clear();
  this->Modified();
}

int vtkAMRConnectivity::FillInputPortInformation(int port, vtkInformation* info)
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
int vtkAMRConnectivity::FillOutputPortInformation(int port, vtkInformation* info)
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
int vtkAMRConnectivity::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkNonOverlappingAMR* amrInput =
    vtkNonOverlappingAMR::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkNonOverlappingAMR* amrOutput =
    vtkNonOverlappingAMR::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  amrOutput->ShallowCopy(amrInput);

  this->Helper = vtkAMRDualGridHelper::New();
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  this->Helper->SetController(controller);
  this->Helper->Initialize(amrInput);

  unsigned int noOfArrays = static_cast<unsigned int>(this->VolumeArrays.size());
  for (unsigned int i = 0; i < noOfArrays; i++)
  {
    if (this->DoRequestData(amrOutput, this->VolumeArrays[i].c_str()) == 0)
    {
      return 0;
    }
  }
  this->Helper->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRConnectivity::DoRequestData(vtkNonOverlappingAMR* volume, const char* volumeName)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkMPIController* mpiController = vtkMPIController::SafeDownCast(controller);
#endif
  int myProc = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  this->RegionName = std::string("RegionId-");
  this->RegionName += volumeName;

  // initialize with a global unique region id
  this->NextRegionId = myProc + 1;

  vtkTimerLog::MarkStartEvent("Initial fragment seeding");

  // Find the block local fragments
  vtkCompositeDataIterator* iter = volume->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    // Go through each block and create an array RegionId set all zero.
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    if (!grid)
    {
      vtkErrorMacro("NonOverlappingAMR not made up of UniformGrids");
      return 0;
    }
    vtkSmartPointer<vtkIdTypeArray> regionId = vtkSmartPointer<vtkIdTypeArray>::New();
    regionId->SetName(this->RegionName.c_str());
    regionId->SetNumberOfComponents(1);
    regionId->SetNumberOfTuples(grid->GetNumberOfCells());
    for (int i = 0; i < grid->GetNumberOfCells(); i++)
    {
      regionId->SetTuple1(i, 0);
    }
    grid->GetCellData()->AddArray(regionId);

    vtkDataArray* volArray = grid->GetCellData()->GetArray(volumeName);
    if (!volArray)
    {
      vtkErrorMacro(<< "There is no " << volumeName << " in cell field");
      return 0;
    }

    vtkUnsignedCharArray* ghostArray = grid->GetCellGhostArray();
    if (!ghostArray)
    {
      vtkErrorMacro("No ghost array attached to the CTH volume data");
      return 0;
    }

    // within each block find all fragments
    int extents[6];
    grid->GetExtent(extents);

    for (int i = extents[0]; i < extents[1]; i++)
    {
      for (int j = extents[2]; j < extents[3]; j++)
      {
        for (int k = extents[4]; k < extents[5]; k++)
        {
          int ijk[3] = { i, j, k };
          vtkIdType cellId = grid->ComputeCellId(ijk);
          if (regionId->GetTuple1(cellId) == 0 &&
            volArray->GetTuple1(cellId) > this->VolumeFractionSurfaceValue &&
            (ghostArray->GetValue(cellId) & vtkDataSetAttributes::DUPLICATECELL) == 0)
          {
            // wave propagation sets the region id as it propagates
            this->WavePropagation(cellId, grid, regionId, volArray, ghostArray);
            // increment this way so the region id remains globally unique
            this->NextRegionId += numProcs;
          }
        }
      }
    }
  }

  vtkTimerLog::MarkEndEvent("Initial fragment seeding");

  if (this->ResolveBlocks)
  {
    vtkTimerLog::MarkStartEvent("Computing boundary regions");

    // Determine boundaries at the block that need to be sent to neighbors
    this->BoundaryArrays.resize(numProcs);
    this->ReceiveList.resize(numProcs);
    this->ValidNeighbor.resize(numProcs);
    this->NeighborList.resize(this->Helper->GetNumberOfLevels());
    for (int level = 0; level < this->Helper->GetNumberOfLevels(); level++)
    {
      int maxId = 0;
      for (int blockId = 0; blockId < this->Helper->GetNumberOfBlocksInLevel(level); blockId++)
      {
        vtkAMRDualGridHelperBlock* block = this->Helper->GetBlock(level, blockId);
        if (block->BlockId > maxId)
        {
          maxId = block->BlockId;
        }
      }
      this->NeighborList[level].resize(maxId + 1);
      for (int blockId = 0; blockId < maxId; blockId++)
      {
        this->NeighborList[level][blockId].resize(0);
      }
    }

    for (int level = 0; level < this->Helper->GetNumberOfLevels(); level++)
    {
      for (int blockId = 0; blockId < this->Helper->GetNumberOfBlocksInLevel(level); blockId++)
      {
        vtkAMRDualGridHelperBlock* block = this->Helper->GetBlock(level, blockId);
        for (int dir = 0; dir < 3; dir++)
        {
          vtkAMRDualGridHelperBlock* neighbor = this->GetBlockNeighbor(block, dir);
          if (neighbor != nullptr)
          {
            this->ProcessBoundaryAtBlock(volume, block, neighbor, dir);
            if (neighbor && neighbor->Level > block->Level)
            {
              int n_level = neighbor->Level;
              int index[3] = { neighbor->GridIndex[0], neighbor->GridIndex[1],
                neighbor->GridIndex[2] };

              index[(dir + 1) % 3]++;
              neighbor = this->Helper->GetBlock(n_level, index[0], index[1], index[2]);
              if (neighbor != nullptr)
              {
                this->ProcessBoundaryAtBlock(volume, block, neighbor, dir);
              }

              index[(dir + 2) % 3]++;
              neighbor = this->Helper->GetBlock(n_level, index[0], index[1], index[2]);
              if (neighbor != nullptr)
              {
                this->ProcessBoundaryAtBlock(volume, block, neighbor, dir);
              }

              index[(dir + 1) % 3]--;
              neighbor = this->Helper->GetBlock(n_level, index[0], index[1], index[2]);
              if (neighbor != nullptr)
              {
                this->ProcessBoundaryAtBlock(volume, block, neighbor, dir);
              }
            }
          }
        }
      }
    }

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
    // Exchange all boundaries between processes where block and neighbor are different procs
    if (numProcs > 1 && !this->ExchangeBoundaries(mpiController))
    {
      return 0;
    }
#endif
    // Process all boundaries at the neighbors to find the equivalence pairs at the boundaries
    this->Equivalence = new vtkAMRConnectivityEquivalence;

    // Initialize equivalence with all regions independent
    for (size_t i = 0; i < this->BoundaryArrays.size(); i++)
    {
      for (size_t j = 0; j < this->BoundaryArrays[i].size(); j++)
      {
        if (i == static_cast<unsigned int>(myProc))
        {
          this->ProcessBoundaryAtNeighbor(volume, this->BoundaryArrays[myProc][j]);
        }
      }
      this->BoundaryArrays[i].clear();
    }
    vtkTimerLog::MarkEndEvent("Computing boundary regions");

    vtkTimerLog::MarkStartEvent("Transferring equivalence");
    this->EquivPairs.resize(numProcs);
    while (true)
    {
      int sets_changed = 0;
      // Relabel all fragment IDs with the equivalence set number
      // (set numbers start with 1 and 0 is considered "no set" or "no fragment")
      for (int level = 0; level < this->Helper->GetNumberOfLevels(); level++)
      {
        for (int blockId = 0; blockId < this->Helper->GetNumberOfBlocksInLevel(level); blockId++)
        {
          vtkAMRDualGridHelperBlock* block = this->Helper->GetBlock(level, blockId);
          if (block->ProcessId != myProc)
          {
            continue;
          }
          vtkUniformGrid* grid = volume->GetDataSet(block->Level, block->BlockId);
          vtkIdTypeArray* regionIdArray =
            vtkIdTypeArray::SafeDownCast(grid->GetCellData()->GetArray(this->RegionName.c_str()));
          if (regionIdArray == nullptr)
          {
            vtkErrorMacro("block Image doesn't not contain the regionId just added");
            return 0;
          }
          for (int i = 0; i < regionIdArray->GetNumberOfTuples(); i++)
          {
            vtkIdType regionId = regionIdArray->GetTuple1(i);
            if (regionId > 0)
            {
              int setId = this->Equivalence->GetMinimumSetId(regionId);
              if (setId > 0 && setId != regionId)
              {
                regionIdArray->SetTuple1(i, setId);
                sets_changed = 1;
                for (size_t p = 0; p < this->NeighborList[block->Level][block->BlockId].size(); p++)
                {
                  int n = this->NeighborList[block->Level][block->BlockId][p];
                  if (this->EquivPairs[n] == nullptr)
                  {
                    this->EquivPairs[n] = vtkSmartPointer<vtkIntArray>::New();
                    this->EquivPairs[n]->SetNumberOfComponents(1);
                    this->EquivPairs[n]->SetNumberOfTuples(0);
                  }
                  int contained = 0;
                  for (int e = 0; e < this->EquivPairs[n]->GetNumberOfTuples(); e += 2)
                  {
                    int v1 = this->EquivPairs[n]->GetValue(e);
                    int v2 = this->EquivPairs[n]->GetValue(e + 1);
                    if ((v1 == regionId && v2 == setId) || (v2 == regionId && v1 == setId))
                    {
                      contained = 1;
                    }
                  }
                  if (contained == 0)
                  {
                    this->EquivPairs[n]->InsertNextValue(regionId);
                    this->EquivPairs[n]->InsertNextValue(setId);
                  }
                }
              }
            }
            else
            {
              regionIdArray->SetTuple1(i, 0);
            }
          }
        }
      }
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
      if (numProcs > 1)
      {
        int out;
        controller->AllReduce(&sets_changed, &out, 1, vtkCommunicator::MAX_OP);
        sets_changed = out;
      }
#endif
      if (sets_changed == 0)
      {
        break;
      }
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
      if (numProcs > 1 && !this->ExchangeEquivPairs(mpiController))
      {
        return 0;
      }
#endif
      // clear out the pairs after sending them.
      for (int i = 0; i < numProcs; i++)
      {
        if (this->EquivPairs[i] != nullptr)
        {
          this->EquivPairs[i]->SetNumberOfTuples(0);
        }
      }
    }

    // clean up EquivPairs for the last time this update
    this->EquivPairs.clear();
    ValidNeighbor.clear();
    NeighborList.clear();

    vtkTimerLog::MarkEndEvent("Transferring equivalence");

    delete this->Equivalence;
    this->Equivalence = nullptr;
  }

  if (PropagateGhosts)
  {
    vtkTimerLog::MarkStartEvent("Propagating ghosts");
    // Propagate the region IDs out to the ghosts
    vtkSmartPointer<vtkIdList> ptIds = vtkIdList::New();
    vtkSmartPointer<vtkIdList> cellIds = vtkIdList::New();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
      vtkIdTypeArray* regionIdArray =
        vtkIdTypeArray::SafeDownCast(grid->GetCellData()->GetArray(this->RegionName.c_str()));
      vtkDataArray* volArray = grid->GetCellData()->GetArray(volumeName);
      vtkUnsignedCharArray* ghostArray = grid->GetCellGhostArray();
      for (int cellId = 0; cellId < regionIdArray->GetNumberOfTuples(); cellId++)
      {
        if ((ghostArray->GetValue(cellId) & vtkDataSetAttributes::DUPLICATECELL) == 0 ||
          volArray->GetTuple1(cellId) < this->VolumeFractionSurfaceValue)
        {
          continue;
        }
        grid->GetCellPoints(cellId, ptIds);
        bool isSet = false;
        for (int i = 0; i < ptIds->GetNumberOfIds(); i++)
        {
          grid->GetPointCells(ptIds->GetId(i), cellIds);
          for (int j = 0; j < cellIds->GetNumberOfIds(); j++)
          {
            vtkIdType neighbor = cellIds->GetId(j);
            if (neighbor != cellId &&
              volArray->GetTuple1(neighbor) > this->VolumeFractionSurfaceValue &&
              (ghostArray->GetValue(neighbor) & vtkDataSetAttributes::DUPLICATECELL) == 0)
            {
              regionIdArray->SetTuple1(cellId, regionIdArray->GetTuple1(neighbor));
              isSet = true;
              break;
            }
          }
          if (isSet)
          {
            break;
          }
        }
      }
    }

    vtkTimerLog::MarkEndEvent("Propagating ghosts");
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRConnectivity::WavePropagation(vtkIdType cellIdStart, vtkUniformGrid* grid,
  vtkIdTypeArray* regionId, vtkDataArray* volArray, vtkUnsignedCharArray* ghostArray)
{
  vtkSmartPointer<vtkIdList> todoList = vtkIdList::New();
  todoList->SetNumberOfIds(0);
  todoList->InsertNextId(cellIdStart);

  vtkSmartPointer<vtkIdList> ptIds = vtkIdList::New();
  vtkSmartPointer<vtkIdList> cellIds = vtkIdList::New();

  while (todoList->GetNumberOfIds() > 0)
  {
    vtkIdType cellId = todoList->GetId(0);
    todoList->DeleteId(cellId);
    regionId->SetTuple1(cellId, this->NextRegionId);

    grid->GetCellPoints(cellId, ptIds);
    for (int i = 0; i < ptIds->GetNumberOfIds(); i++)
    {
      cellIds->SetNumberOfIds(0);
      grid->GetPointCells(ptIds->GetId(i), cellIds);
      for (int j = 0; j < cellIds->GetNumberOfIds(); j++)
      {
        vtkIdType neighbor = cellIds->GetId(j);
        if (neighbor == cellId || todoList->IsId(neighbor) >= 0)
        {
          continue;
        }
        if (regionId->GetTuple1(neighbor) == 0 &&
          volArray->GetTuple1(neighbor) > this->VolumeFractionSurfaceValue &&
          (ghostArray->GetValue(neighbor) & vtkDataSetAttributes::DUPLICATECELL) == 0)
        {
          todoList->InsertNextId(neighbor);
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
vtkAMRDualGridHelperBlock* vtkAMRConnectivity::GetBlockNeighbor(
  vtkAMRDualGridHelperBlock* block, int dir)
{
  int gridIndex[3] = { block->GridIndex[0], block->GridIndex[1], block->GridIndex[2] };
  gridIndex[dir]++;

  vtkAMRDualGridHelperBlock* neighbor = nullptr;
  neighbor = this->Helper->GetBlock(block->Level, gridIndex[0], gridIndex[1], gridIndex[2]);
  if (neighbor != nullptr)
  {
    return neighbor;
  }
  neighbor = this->Helper->GetBlock(
    block->Level - 1, gridIndex[0] >> 1, gridIndex[1] >> 1, gridIndex[2] >> 1);
  if (neighbor != nullptr)
  {
    return neighbor;
  }
  // It's only the 0,0,0 corner block of the set.
  // We'll need 3 more +1 blocks depending on direction
  // e.g., 0, 0, 1  and 0, 1, 0 and 0, 1, 1  If it's in the x direction.
  neighbor = this->Helper->GetBlock(
    block->Level + 1, gridIndex[0] << 1, gridIndex[1] << 1, gridIndex[2] << 1);
  if (neighbor != nullptr)
  {
    return neighbor;
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void vtkAMRConnectivity::ProcessBoundaryAtBlock(vtkNonOverlappingAMR* volume,
  vtkAMRDualGridHelperBlock* block, vtkAMRDualGridHelperBlock* neighbor, int dir)
{
  if (block == nullptr || neighbor == nullptr)
  {
    vtkErrorMacro("Cannot process undefined blocks");
    return;
  }

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int myProc = controller->GetLocalProcessId();
  // int numProcs = controller->GetNumberOfProcesses ();

  if (block->ProcessId != myProc && neighbor->ProcessId != myProc)
  {
    return;
  }

  if (block->ProcessId != neighbor->ProcessId)
  {
    int levelId, blockId, processId;
    if (neighbor->ProcessId != myProc)
    {
      levelId = block->Level;
      blockId = block->BlockId;
      processId = neighbor->ProcessId;
    }
    else /* if (block->ProcessId != myProc) */
    {
      levelId = neighbor->Level;
      blockId = neighbor->BlockId;
      processId = block->ProcessId;
    }

    // Add this neighbor to this block's neighbor list.
    bool contained = false;
    for (size_t i = 0; i < this->NeighborList[levelId][blockId].size(); i++)
    {
      if (this->NeighborList[levelId][blockId][i] == processId)
      {
        contained = true;
        break;
      }
    }
    if (!contained)
    {
      this->NeighborList[levelId][blockId].push_back(processId);
      this->ValidNeighbor[processId] = true;
    }
  }

  if (block->ProcessId == myProc)
  {
    vtkUniformGrid* grid = volume->GetDataSet(block->Level, block->BlockId);
    vtkIdTypeArray* regionId =
      vtkIdTypeArray::SafeDownCast(grid->GetCellData()->GetArray(this->RegionName.c_str()));

    int extent[6];
    grid->GetExtent(extent);

    // move inside the overlap areas.
    extent[dir * 2 + 1]--;

    // we only want the plane at the edge in the direction
    extent[dir * 2] = extent[dir * 2 + 1] - 1;

    if (block->Level < neighbor->Level)
    {
      // we only want the quarter of the boundary that applies to this neighbor
      int del[3];
      del[0] = (extent[1] - extent[0]) >> 1;
      del[1] = (extent[3] - extent[2]) >> 1;
      del[2] = (extent[5] - extent[4]) >> 1;

      for (int i = 0; i < 3; i++)
      {
        if (i == dir)
        {
          continue;
        }
        // shrink the orthogonal directions by half depending on which offset the neighbor is
        if ((neighbor->GridIndex[i] % 2) == 0)
        {
          extent[i * 2 + 1] -= del[i];
        }
        else
        {
          extent[i * 2 + 0] += del[i];
        }
      }
    }

    vtkSmartPointer<vtkIdTypeArray> array = vtkSmartPointer<vtkIdTypeArray>::New();
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(
      9 + (extent[1] - extent[0]) * (extent[3] - extent[2]) * (extent[5] - extent[4]));

    int index = 0;
    array->SetTuple1(index, block->Level);
    index++;
    array->SetTuple1(index, block->GridIndex[0]);
    index++;
    array->SetTuple1(index, block->GridIndex[1]);
    index++;
    array->SetTuple1(index, block->GridIndex[2]);
    index++;
    array->SetTuple1(index, dir);
    index++;
    array->SetTuple1(index, neighbor->Level);
    index++;
    array->SetTuple1(index, neighbor->GridIndex[0]);
    index++;
    array->SetTuple1(index, neighbor->GridIndex[1]);
    index++;
    array->SetTuple1(index, neighbor->GridIndex[2]);
    index++;

    if (block->Level > neighbor->Level)
    {
      int ijk[3];
      for (ijk[0] = extent[0]; ijk[0] < extent[1]; ijk[0] += 2)
      {
        for (ijk[1] = extent[2]; ijk[1] < extent[3]; ijk[1] += 2)
        {
          for (ijk[2] = extent[4]; ijk[2] < extent[5]; ijk[2] += 2)
          {
            vtkIdType cellId = grid->ComputeCellId(ijk);
            int region = regionId->GetTuple1(cellId);
            int setting = 1;
            // if this cell doesn't have a region
            // check one of the other 3 that align with the neighbor cell
            while (region == 0 && setting <= 8)
            {
              int ijk_off[3];
              ijk_off[0] = ijk[0];
              if ((setting & 1) == 1 && extent[1] != extent[0])
              {
                ijk_off[0]++;
              }
              ijk_off[1] = ijk[1];
              if ((setting & 2) == 2 && extent[3] != extent[2])
              {
                ijk_off[1]++;
              }
              ijk_off[2] = ijk[2];
              if ((setting & 4) == 4 && extent[5] != extent[4])
              {
                ijk_off[2]++;
              }
              cellId = grid->ComputeCellId(ijk_off);
              region = regionId->GetTuple1(cellId);
              setting++;
            }
            array->SetTuple1(index, region);
            index++;
          }
        }
      }
    }
    else
    {
      int ijk[3];
      for (ijk[0] = extent[0]; ijk[0] < extent[1]; ijk[0]++)
      {
        for (ijk[1] = extent[2]; ijk[1] < extent[3]; ijk[1]++)
        {
          for (ijk[2] = extent[4]; ijk[2] < extent[5]; ijk[2]++)
          {
            vtkIdType cellId = grid->ComputeCellId(ijk);
            array->SetTuple1(index, regionId->GetTuple1(cellId));
            index++;
          }
        }
      }
    }

    this->BoundaryArrays[neighbor->ProcessId].push_back(array);
  }
  else if (neighbor->ProcessId == myProc)
  {
    vtkUniformGrid* grid = volume->GetDataSet(neighbor->Level, neighbor->BlockId);
    // estimate the size of the receive by the extent of this neighbor
    int extent[6];
    grid->GetExtent(extent);
    extent[dir * 2 + 1] = extent[dir * 2] + 2;

    this->ReceiveList[block->ProcessId].push_back(
      9 + (extent[1] - extent[0]) * (extent[3] - extent[2]) * (extent[5] - extent[4]));
  }
}
//----------------------------------------------------------------------------
int vtkAMRConnectivity::ExchangeBoundaries(vtkMPIController* controller)
{
  if (controller == nullptr)
  {
    vtkErrorMacro("vtkAMRConnectivity only works parallel in MPI environment");
    return 0;
  }

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  int myProc = controller->GetLocalProcessId();

  vtkAMRConnectivityCommRequestList receiveList;
  for (size_t i = 0; i < this->ReceiveList.size(); i++)
  {
    if (i == static_cast<unsigned int>(myProc))
    {
      continue;
    }

    int messageLength = 0;
    for (size_t j = 0; j < this->ReceiveList[i].size(); j++)
    {
      messageLength += 1 + this->ReceiveList[i][j];
    }
    if (messageLength == 0)
    {
      continue;
    }
    messageLength++; // end of message marker

    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(messageLength);

    vtkAMRConnectivityCommRequest request;
    request.SendProcess = static_cast<int>(i);
    request.ReceiveProcess = myProc;
    request.Buffer = array;

    controller->NoBlockReceive(
      array->GetPointer(0), messageLength, static_cast<int>(i), BOUNDARY_TAG, request.Request);

    this->ReceiveList[i].clear();
    receiveList.push_back(request);
  }

  vtkAMRConnectivityCommRequestList sendList;
  for (size_t i = 0; i < this->BoundaryArrays.size(); i++)
  {
    if (i == static_cast<unsigned int>(myProc))
    {
      continue;
    }
    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(0);
    for (size_t j = 0; j < this->BoundaryArrays[i].size(); j++)
    {
      int tuples = this->BoundaryArrays[i][j]->GetNumberOfTuples();
      array->InsertNextTuple1(tuples);
      for (int k = 0; k < tuples; k++)
      {
        array->InsertNextTuple1(this->BoundaryArrays[i][j]->GetTuple1(k));
      }
    }

    if (array->GetNumberOfTuples() == 0)
    {
      continue;
    }

    array->InsertNextTuple1(-1);

    vtkAMRConnectivityCommRequest request;
    request.SendProcess = myProc;
    request.ReceiveProcess = static_cast<int>(i);
    request.Buffer = array;

    controller->NoBlockSend(array->GetPointer(0), array->GetNumberOfTuples(), static_cast<int>(i),
      BOUNDARY_TAG, request.Request);

    sendList.push_back(request);
  }

  while (!receiveList.empty())
  {
    vtkAMRConnectivityCommRequest request = receiveList.WaitAny();
    vtkSmartPointer<vtkIntArray> array = request.Buffer;
    int total = array->GetNumberOfTuples();
    int index = 0;
    while (index < total)
    {
      int tuples = array->GetTuple1(index);
      if (tuples < 0)
      {
        break;
      }
      index++;
      vtkIdTypeArray* boundary = vtkIdTypeArray::New();
      boundary->SetNumberOfComponents(1);
      boundary->SetNumberOfTuples(tuples);
      for (int i = 0; i < tuples; i++)
      {
        boundary->SetTuple1(i, array->GetTuple1(index));
        index++;
      }
      this->BoundaryArrays[myProc].push_back(boundary);
    }
  }

  sendList.WaitAll();
  sendList.clear();
#endif
  return 1;
}

int vtkAMRConnectivity::ExchangeEquivPairs(vtkMPIController* controller)
{
  if (controller == nullptr)
  {
    vtkErrorMacro("vtkAMRConnectivity only works parallel in MPI environment");
    return 0;
  }
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  int myProc = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  vtkAMRConnectivityCommRequestList receiveList;
  for (size_t i = 0; i < static_cast<unsigned int>(numProcs); i++)
  {
    if (i == static_cast<unsigned int>(myProc) || !this->ValidNeighbor[i])
    {
      continue;
    }

    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(1);

    vtkAMRConnectivityCommRequest request;
    request.SendProcess = static_cast<int>(i);
    request.ReceiveProcess = myProc;
    request.Buffer = array;

    controller->NoBlockReceive(
      array->GetPointer(0), 1, static_cast<int>(i), EQUIV_SIZE_TAG, request.Request);

    this->ReceiveList[i].clear();
    receiveList.push_back(request);
  }

  vtkAMRConnectivityCommRequestList sendList;
  for (size_t i = 0; i < static_cast<unsigned int>(numProcs); i++)
  {
    if (i == static_cast<unsigned int>(myProc) || !this->ValidNeighbor[i])
    {
      continue;
    }

    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(1);
    int size = this->EquivPairs[i] == nullptr ? 0 : this->EquivPairs[i]->GetNumberOfTuples();
    array->SetTuple1(0, size);

    vtkAMRConnectivityCommRequest request;
    request.SendProcess = myProc;
    request.ReceiveProcess = static_cast<int>(i);
    request.Buffer = array;

    controller->NoBlockSend(
      array->GetPointer(0), 1, static_cast<int>(i), EQUIV_SIZE_TAG, request.Request);

    sendList.push_back(request);
  }

  std::vector<int> receive_sizes(numProcs);
  for (int i = 0; i < numProcs; i++)
  {
    receive_sizes[i] = 0;
  }

  while (!receiveList.empty())
  {
    vtkAMRConnectivityCommRequest request = receiveList.WaitAny();
    vtkSmartPointer<vtkIntArray> array = request.Buffer;
    receive_sizes[request.SendProcess] = array->GetTuple1(0);
  }

  receiveList.clear();
  sendList.WaitAll();
  sendList.clear();

  for (size_t i = 0; i < static_cast<unsigned int>(numProcs); i++)
  {
    if (i == static_cast<unsigned int>(myProc) || receive_sizes[i] == 0)
    {
      continue;
    }

    vtkSmartPointer<vtkIntArray> array = vtkSmartPointer<vtkIntArray>::New();
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(receive_sizes[i]);

    vtkAMRConnectivityCommRequest request;
    request.SendProcess = static_cast<int>(i);
    request.ReceiveProcess = myProc;
    request.Buffer = array;

    controller->NoBlockReceive(
      array->GetPointer(0), receive_sizes[i], static_cast<int>(i), EQUIV_TAG, request.Request);

    this->ReceiveList[i].clear();
    receiveList.push_back(request);
  }

  for (size_t i = 0; i < static_cast<unsigned int>(numProcs); i++)
  {
    if (i == static_cast<unsigned int>(myProc) || this->EquivPairs[i] == nullptr ||
      this->EquivPairs[i]->GetNumberOfTuples() == 0)
    {
      continue;
    }

    vtkAMRConnectivityCommRequest request;
    request.SendProcess = myProc;
    request.ReceiveProcess = static_cast<int>(i);
    request.Buffer = this->EquivPairs[i];

    controller->NoBlockSend(this->EquivPairs[i]->GetPointer(0),
      this->EquivPairs[i]->GetNumberOfTuples(), static_cast<int>(i), EQUIV_TAG, request.Request);

    sendList.push_back(request);
  }

  while (!receiveList.empty())
  {
    vtkAMRConnectivityCommRequest request = receiveList.WaitAny();
    vtkSmartPointer<vtkIntArray> array = request.Buffer;
    for (int i = 0; i < array->GetNumberOfTuples(); i += 2)
    {
      int v1 = array->GetValue(i);
      int v2 = array->GetValue(i + 1);
      this->Equivalence->AddEquivalence(v1, v2);
    }
  }

  receiveList.clear();
  sendList.WaitAll();
  sendList.clear();
#endif
  return 1;
}

//----------------------------------------------------------------------------
void vtkAMRConnectivity::ProcessBoundaryAtNeighbor(
  vtkNonOverlappingAMR* volume, vtkIdTypeArray* array)
{

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int myProc = controller->GetLocalProcessId();
  // int numProcs = controller->GetNumberOfProcesses ();

  int index = 0;
  int blockLevel = array->GetTuple1(index);
  index++;
  int blockGrid[3];
  blockGrid[0] = array->GetTuple1(index);
  index++;
  blockGrid[1] = array->GetTuple1(index);
  index++;
  blockGrid[2] = array->GetTuple1(index);
  index++;
  int dir = array->GetTuple1(index);
  index++;
  int neighborLevel = array->GetTuple1(index);
  index++;
  int gridX = array->GetTuple1(index);
  index++;
  int gridY = array->GetTuple1(index);
  index++;
  int gridZ = array->GetTuple1(index);
  index++;

  vtkAMRDualGridHelperBlock* neighbor = this->Helper->GetBlock(neighborLevel, gridX, gridY, gridZ);

  if (neighbor->ProcessId != myProc)
  {
    vtkErrorMacro("Shouldn't get here.  Cannot process neighbor on remote process");
    return;
  }

  vtkUniformGrid* grid = volume->GetDataSet(neighbor->Level, neighbor->BlockId);
  vtkIdTypeArray* regionId =
    vtkIdTypeArray::SafeDownCast(grid->GetCellData()->GetArray(this->RegionName.c_str()));

  int extent[6];
  grid->GetExtent(extent);

  // move inside the overlap areas.
  extent[dir * 2]++;

  // we only want the plane at the edge in dir
  extent[dir * 2 + 1] = extent[dir * 2] + 1;

  if (blockLevel > neighborLevel)
  {
    // we only want the quarter of the boundary that applies to this neighbor
    int del[3];
    del[0] = (extent[1] - extent[0]) >> 1;
    del[1] = (extent[3] - extent[2]) >> 1;
    del[2] = (extent[5] - extent[4]) >> 1;

    for (int i = 0; i < 3; i++)
    {
      if (i == dir)
      {
        continue;
      }
      // shrink the orthogonal directions by half depending on which offset the neighbor is
      // TODO need the blocks offset within the octet
      if ((blockGrid[i] % 2) == 0)
      {
        extent[i * 2 + 1] -= del[i];
      }
      else
      {
        extent[i * 2 + 0] += del[i];
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
          vtkIdType cellId = grid->ComputeCellId(ijk);
          int neighborRegion = regionId->GetTuple1(cellId);
          int setting = 1;
          // if this cell doesn't have a region
          // check one of the other 3 that align with the neighbor cell
          while (neighborRegion == 0 && setting <= 8)
          {
            int ijk_off[3];
            ijk_off[0] = ijk[0];
            if ((setting & 1) == 1 && extent[1] != extent[0])
            {
              ijk_off[0]++;
            }
            ijk_off[1] = ijk[1];
            if ((setting & 2) == 2 && extent[3] != extent[2])
            {
              ijk_off[1]++;
            }
            ijk_off[2] = ijk[2];
            if ((setting & 4) == 4 && extent[5] != extent[4])
            {
              ijk_off[2]++;
            }
            cellId = grid->ComputeCellId(ijk_off);
            neighborRegion = regionId->GetTuple1(cellId);
            setting++;
          }
          int blockRegion = array->GetTuple1(index);
          if (neighborRegion != 0 && blockRegion != 0)
          {
            this->Equivalence->AddEquivalence(neighborRegion, blockRegion);
          }
        }
        index++;
      }
    }
  }
  else /* blockLevel >= neighborLevel */
  {
    int ijk[3];
    for (ijk[0] = extent[0]; ijk[0] < extent[1]; ijk[0]++)
    {
      for (ijk[1] = extent[2]; ijk[1] < extent[3]; ijk[1]++)
      {
        for (ijk[2] = extent[4]; ijk[2] < extent[5]; ijk[2]++)
        {
          vtkIdType cellId = grid->ComputeCellId(ijk);
          int neighborRegion = regionId->GetTuple1(cellId);
          int blockRegion = array->GetTuple1(index);
          if (neighborRegion != 0 && blockRegion != 0)
          {
            this->Equivalence->AddEquivalence(neighborRegion, blockRegion);
          }
          index++;
        }
      }
    }
  }
}
