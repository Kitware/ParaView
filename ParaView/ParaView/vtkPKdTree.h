// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPKdTree.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

// .NAME vtkPKdTree - Build a k-d tree decomposition of a list of points.
//
// .SECTION Description
//      Build, in parallel, a k-d tree decomposition of one or more
//      vtkDataSets distributed across processors.
//      When done, each processor has access to the k-d tree structure, and
//      can obtain information about which processor contains the original
//      data for which spatial regions.
//
// .SECTION See Also
//      vtkKdTree

#ifndef __vtkPKdTree_h
#define __vtkPKdTree_h

//#include "vtksnlParallelWin32Header.h"

#include <vtkKdTree.h>
#include <vtkMultiProcessController.h>
#include <vtkCommunicator.h>
#include <vtkVersion.h>

class vtkSubGroup;
class vtkIdList;

class VTK_EXPORT vtkPKdTree : public vtkKdTree
{
    vtkTypeRevisionMacro(vtkPKdTree, vtkKdTree);

public:

    void PrintSelf(ostream& os, vtkIndent indent);
    void PrintTiming(ostream& os, vtkIndent indent);
    void PrintTables(ostream& os, vtkIndent indent);

    static vtkPKdTree *New();

    // Description:
    //   Build the spatial decomposition.  Call this explicitly
    //   after changing any parameters affecting the build of the
    //   tree.  It must be called by all processes in the parallel
    //   application, or it will hang.  This is the only public method
    //   in this class that must be called by all processes.

    void BuildLocator();

    // Description:
    //   Get the total number of cells distributed across the data
    //   files read by all processes.

    int GetTotalNumberOfCells(){return this->TotalNumCells;}


    // Description:
    //   Set/Get the number of spatial regions you want to get close
    //   to without going over.  (The number of spatial regions is normally
    //   a power of two.)

    vtkSetMacro(NumRegionsOrLess, int);
    vtkGetMacro(NumRegionsOrLess, int);

    // Description:
    //   Set/Get the number of spatial regions you want to get close
    //   to while having at least this many regions.  (The number of 
    //   spatial regions is normally a power of two.)  Performance
    //   hint:  Request just enough regions so you have at least one
    //   spatial region per processor.  Further subdividing the
    //   space takes time.

    vtkSetMacro(NumRegionsOrMore, int);
    vtkGetMacro(NumRegionsOrMore, int);

    // Description:
    //   Set/Get the level of ghost cells included in cell count tables
    //   The feature is not implemented yet.  GhostLevel is 0.

    vtkSetMacro(GhostLevel, int);
    vtkGetMacro(GhostLevel, int);

    // Description:
    //    Set/Get whether any cell counts, including block level, 
    //    are created.  If this is "On", we create tables indicating
    //    how many cells each processor has in each spatial region.
    //    Default is "Off".

    vtkBooleanMacro(ProcessCellCountData, int);
    vtkSetMacro(ProcessCellCountData, int);
    vtkGetMacro(ProcessCellCountData, int);

    // Description:
    //    Set/Get whether block level cell counts are created.  If
    //    this is "On", the process cell counts will be broken down
    //    per Exodus file block.  Default is "Off".  Ignored if
    //    ProcessCellCountData is "Off".

    vtkBooleanMacro(BlockData, int);
    vtkSetMacro(BlockData, int);
    vtkGetMacro(BlockData, int);

    // Description:
    //   Set/Get the communicator object

    void SetController(vtkMultiProcessController *c);
    vtkGetObjectMacro(Controller, vtkMultiProcessController);

    // Description:
    //   The PKdTree class can assign spatial regions to processors after
    //   building the k-d tree, using one of several partitioning criteria.
    //   These functions Set/Get whether this assignment is computed. 
    //   The default is "Off", no assignment is computed.   If "On", and
    //   no assignment scheme is specified, contiguous assignment will be
    //   computed.  Specifying an assignment scheme (with AssignRegions*())
    //   automatically turns on RegionAssignment.

    vtkBooleanMacro(RegionAssignment, int);
    vtkGetMacro(RegionAssignment, int);

    static const int NoRegionAssignment;
    static const int ContiguousAssignment;
    static const int UserDefinedAssignment;
    static const int RoundRobinAssignment;
    static const int MinimizeDataMovementAssignment;

    // Description:
    //   Assign spatial regions to processes via a user defined map.
    //   The user-supplied map is indexed by region ID, and provides a
    //   process ID for each region. 

    int AssignRegions(int *map, int numRegions);

    // Description:
    //   Let the PKdTree class assign a process to each region in a
    //   round robin fashion.  If the k-d tree has not yet been
    //   built, the regions will be assigned after BuildLocator executes.

    int AssignRegionsRoundRobin();

    // Description:
    //    Let the PKdTree class assign a process to each region 
    //    by assigning contiguous sets of spatial regions to each
    //    process.  If the k-d tree has not yet been
    //   built, the regions will be assigned after BuildLocator executes.

    int AssignRegionsContiguous();

    // Description:
    //    Let the PKdTree class assign a process to each region 
    //    by considering which data each process already has, so
    //    as to minimize data movement.  If the k-d tree has not yet been
    //    built, the regions will be assigned after BuildLocator executes.
    //    Not yet implemented.

    int AssignRegionsToMinimizeDataMovement();

    // Description:
    //    Writes the list of region IDs assigned to the specified
    //    process.  Returns the number of regions in the list. 

    int GetRegionAssignmentList(int procId, vtkIdList *list);

    // Description:
    //   Returns 1 if the process has data for the given region,
    //   0 otherwise.  If ghost cells have been requested, a 1
    //   is returned even if the process only has data which are
    //   ghost cells for the region, and not actually within the
    //   region.

    int HasData(int processId, int regionId);

    // Description:
    //   Returns the number of cells the specified process has in the
    //   specified region.  If ghost cells have been requested,
    //   these are counted, even if they are not in the region.

    int GetProcessCellCountForRegion(int processId, int regionId);

    // Description:
    //   Returns the total number of processes that have data
    //   falling within this spatial region.  If ghost cells were
    //   requested, processes holding data that are ghost cells
    //   for the region are also counted.

    int GetTotalProcessesInRegion(int regionId);

    // Description:
    //   Adds the list of processes having data for the given
    //   region to the supplied list, returns the number of
    //   processes added.

    int GetProcessListForRegion(int regionId, vtkIdList *processes);

    // Description:
    //   Writes the number of cells each process has for the region
    //   to the supplied list of length len.  Returns the number of
    //   cell counts written.  The order of the cell counts corresponds
    //   to the order of process IDs in the process list returned by
    //   GetProcessListForRegion.

    int GetProcessesCellCountForRegion(int regionId, int *count, int len);

    // Description:
    //   Returns the total number of spatial regions that a given
    //   process has data for.  If ghost cells were requested,
    //   regions for which the process has only ghost cells are
    //   counted.

    int GetTotalRegionsForProcess(int processId);

    // Description:
    //   Adds the region IDs for which this process has data to
    //   the supplied vtkIdList.  Retruns the number of regions.

    int GetRegionListForProcess(int processId, vtkIdList *regions);

    // Description:
    //   Writes to the supplied list the number of cells this
    //   process has for each region.  Returns the number of
    //   cell counts written.  The order of the cell counts corresponds
    //   to the order of region IDs in the region list returned by
    //   GetRegionListForProcess.

    int GetRegionsCellCountForProcess(int ProcessId, int *count, int len);

    // Description:
    //    Returns the number of blocks in the data set.

    int GetNumberOfBlockIds();

    // Description:
    //    Adds the IDs of the blocks in the data set to the provided list. 
    //    Returns the number of block IDs added.

    int GetListOfBlockIds(vtkIdList *blockIds);

    // Description:
    //   Some data sets organize cells into blocks, each block having
    //   and assigned unique non-negative block ID.  This function
    //   adds to the supplied list the block Ids of all data found in
    //   the given region.  Returns the number of block Ids added.

    int GetBlockIdsInRegion(int regionId, vtkIdList *blockIds);

    // Description:
    //   Adds to the supplied list the regions containing the
    //   given block ID.  Returns the number of region IDs added.

    int GetRegionIdsContainingBlock(int blockID, vtkIdList *regionIds);

    // Description:
    //   Writes to the supplied array the number of cells found in
    //   each block found in the given spatial region (up to a maximum
    //   of len blocks).  The order of the block data written is the
    //   order of the block Ids returned in GetBlockIdsInRegion.
    //   Returns the number of cell counts written.

    int GetBlockCellCountsInRegion(int regionId, int *cellCounts, int len);

    // Description:
    //   Returns the number of cells held by the given process for the
    //   given region and block Id.

    int GetProcessBlockCellCountInRegion(int processId, int regionId, int blockId);

    // Description:
    //    The internal process/region/block tables may require a 
    //    large amount of memory.  This call frees this memory.

    void ReleaseTables();

    // Description:
    //   This is a useful function that probably belongs in some
    //   other class.  It takes a list and creates a new sorted 
    //   list of unique IDs.

    static int MakeSortedUnique(int *list, int len, int **newList);
    
protected:

    vtkPKdTree();
    ~vtkPKdTree();

private:

  int NumRegionsOrLess;
  int NumRegionsOrMore;
  int GhostLevel;
  int BlockData;
  int ProcessCellCountData;
  int RegionAssignment;

  vtkMultiProcessController *Controller;

  vtkSubGroup *SubGroup;

  int NumProcesses;
  int MyId;

  // basic tables - each region is the responbility of one process, but
  //                one process may be assigned many regions

  int *RegionAssignmentMap;        // indexed by region ID
  int RegionAssignmentMapLength;
  int *NumRegionsAssigned;         // indexed by process ID

  vtkSetMacro(RegionAssignment, int);
  int UpdateRegionAssignment();

  // basic tables reflecting the data that was read from disk
  // by each process, includes ghost cells if GhostLevel > 0

  char *DataLocationMap;              // by process, by region

  int *NumProcessesInRegion;          // indexed by region ID
  int **ProcessList;                  // indexed by region ID

  int *NumRegionsInProcess;           // indexed by process ID
  int **RegionList;                   // indexed by process ID

  int **CellCountList;                // indexed by region ID

  // optional block data

  int NumBlockIds;
  int *BlockIdList;
  int FirstBlockId;
  int LastBlockId;
  int BlockIdsContiguous;

  int *NumBlocksInRegion;    // indexed by region ID
  int *CumulativeNumBlocks;  // indexed by region ID
  int **BlockList;           // indexed by region ID

  int *TempBlockCounts;          // by region by block ID
  int *TotalBlockCounts;         // by region by block ID

  int **ProcessRegionBlockCounts;  // by process, by region, by block ID

  // distribution of indices for select operation

  int BuildGlobalIndexLists(int ncells);

  int *StartVal;
  int *EndVal;
  int *NumCells;
  int TotalNumCells;

  // local share of points to be partitioned, and local cache

  int WhoHas(int pos);
  int _whoHas(int L, int R, int pos);
  float *GetLocalVal(int pos);
  float *GetLocalValNext(int pos);
  void SetLocalVal(int pos, float *val);
  void ExchangeVals(int pos1, int pos2);
  void ExchangeLocalVals(int pos1, int pos2);

  float *PtArray;
  float *PtArray2;
  float *CurrentPtArray;
  float *NextPtArray;
  int PtArraySize;

  int *SelectBuffer;

  // Parallel build of k-d tree

  int AllCheckForFailure(int rc, char *where, char *how);
  void AllCheckParameters();
  float *VolumeBounds();
  int DivideTest(int L, int R, int level);
  int DivideRegion(vtkKdNode *kd, int L, int level, int tag);
  int BreadthFirstDivide(float *bounds);
  void enQueueNode(vtkKdNode *kd, int L, int level, int tag);

  int Select(int dim, int L, int R);
  void _select(int L, int R, int K, int dim);
  void DoTransfer(int from, int to, int fromIndex, int toIndex, int count);
  int PartitionAboutMyValue(int L, int R, int K, int dim);
  int PartitionAboutOtherValue(int L, int R, float T, int dim);
  int PartitionSubArray(int L, int R, int K, int dim, int p1, int p2);

  int CompleteTree();
  void RetrieveData(vtkKdNode *kd, int *buf);

  float *DataBounds(int L, int K, int R);
  void GetLocalMinMax(int L, int R, int me, float *min, float *max);

  static int FillOutTree(vtkKdNode *kd, int level);
  static int ComputeDepth(vtkKdNode *kd);
  static void PackData(vtkKdNode *kd, float *data);
  static void UnpackData(vtkKdNode *kd, float *data);

  // list management

  int AllocateDoubleBuffer();
  void FreeDoubleBuffer();
  void SwitchDoubleBuffer();
  int AllocateSelectBuffer();
  void FreeSelectBuffer();

  void InitializeGlobalIndexLists();
  int AllocateAndZeroGlobalIndexLists();
  void FreeGlobalIndexLists();
  void InitializeRegionAssignmentLists();
  int AllocateAndZeroRegionAssignmentLists();
  void FreeRegionAssignmentLists();
  void InitializeProcessDataLists();
  int AllocateAndZeroProcessDataLists();
  void FreeBlockDataLists();
  void FreeProcessDataLists();

  // Assigning regions to processors

  void AddProcessRegions(int procId, vtkKdNode *kd);

  // Gather process/region data totals

  int *CollectLocalRegionProcessData();
  int BuildRegionProcessTables();
  void AddEntry(int *list, int len, int id);
  int FindBlockIdIndex(int block);
  int FindBlockId(int index);
  int FindRegionBlockIndex(int regionId, int blockIndex);
  int BuildGlobalBlockIdList();
  int SameBlockIdList(vtkDataArray *blockIds);
  void BuildBlockCountTables();
  static int BinarySearch(int *list, int len, int which);

};

//BTX
class VTK_EXPORT vtkSubGroup{
public:

  enum {MINOP = 1, MAXOP = 2, SUMOP = 3};

  vtkSubGroup(int p0, int p1, int me, int tag, vtkCommunicator *c);
  ~vtkSubGroup();

  int Gather(int *data, int *to, int length, int root);
  int Gather(char *data, char *to, int length, int root);
  int Gather(float *data, float *to, int length, int root);
  int Broadcast(float *data, int length, int root);
  int Broadcast(int *data, int length, int root);
  int Broadcast(char *data, int length, int root);
  int ReduceSum(int *data, int *to, int length, int root);
  int ReduceMax(float *data, float *to, int length, int root);
  int ReduceMax(int *data, int *to, int length, int root);
  int ReduceMin(float *data, float *to, int length, int root);
  int ReduceMin(int *data, int *to, int length, int root);

  int AllReduceUniqueList(int *list, int len, int **newList);
  int MergeSortedUnique(int *list1, int len1, int *list2, int len2, int **newList);

  void setGatherPattern(int root, int length);
  int getLocalRank(int processID);

  int Barrier();

  void PrintSubGroup() const;

  int tag;

private:
  static void mergeUnique(int *list, int len, vtkIdList *ids);
  int computeFanInTargets();
  void restoreRoot(int rootLoc);
  void moveRoot(int rootLoc);
  void setUpRoot(int root);

  int fanInFrom[20];         // reduce, broadcast
  int fanInTo;
  int nFrom, nTo;

  int sendId;                // gather
  int sendOffset;
  int sendLength;
  int recvId[20];
  int recvOffset[20];
  int recvLength[20];
  int nSend, nRecv;
  int gatherRoot, gatherLength;

  int *members;
  int nmembers;
  int myLocalRank;

  vtkCommunicator *comm;
};
//ETX
#endif
