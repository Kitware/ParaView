/*=========================================================================

  Program:   ParaView
  Module:    vtkDistributedDataFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkDistributedDataFilter
//
// .SECTION Description
//
// .SECTION See Also

#include "vtkDistributedDataFilter.h"
#include "vtkExtractCells.h"
#include "vtkMergeCells.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtractUserDefinedPiece.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkShortArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMultiProcessController.h"
#include "vtkSocketController.h"
#include "vtkDataSetWriter.h"
#include "vtkDataSetReader.h"
#include "vtkCharArray.h"
#include "vtkBoxClipDataSet.h"
#include "vtkClipDataSet.h"
#include "vtkBox.h"
#include "vtkPlanes.h"
#include "vtkIdList.h"
#include "vtkPlane.h"

#ifdef VTK_USE_MPI
#include <vtkMPIController.h>
#endif

// Timing data ---------------------------------------------

#include <vtkTimerLog.h>

#define MSGSIZE 60

static char dots[MSGSIZE] = "...........................................................";
static char msg[MSGSIZE];

static char * makeEntry(char *s)
{
  memcpy(msg, dots, MSGSIZE);
  int len = strlen(s);
  len = (len >= MSGSIZE) ? MSGSIZE-1 : len;

  memcpy(msg, s, len);

  return msg;
}

#define TIMER(s)                    \
  if (this->Timing)                 \
    {                               \
    char *s2 = makeEntry(s);        \
    if (this->TimerLog == NULL)            \
      {                                    \
      this->TimerLog = vtkTimerLog::New(); \
      }                                    \
    this->TimerLog->MarkStartEvent(s2); \
    }

#define TIMERDONE(s) \
  if (this->Timing){ char *s2 = makeEntry(s); this->TimerLog->MarkEndEvent(s2); }

// Timing data ---------------------------------------------

vtkCxxRevisionMacro(vtkDistributedDataFilter, "1.9");

vtkStandardNewMacro(vtkDistributedDataFilter);

const char *vtkDistributedDataFilter::TemporaryGlobalCellIds="___D3___GlobalCellIds";
const char *vtkDistributedDataFilter::TemporaryInsideBoxFlag="___D3___WHERE";
const char *vtkDistributedDataFilter::TemporaryGlobalNodeIds="___D3___GlobalNodeIds";
const unsigned char vtkDistributedDataFilter::UnsetGhostLevel = 99;

vtkDistributedDataFilter::vtkDistributedDataFilter()
{
  this->Kdtree = NULL;

  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->NumConvexSubRegions = 0;
  this->ConvexSubRegionBounds = NULL;

  this->GhostLevel = 0;

  this->GlobalIdArrayName = NULL;

  this->RetainKdtree = 0;
  this->IncludeAllIntersectingCells = 0;
  this->ClipCells = 0;

  this->Timing = 0;
  this->TimerLog = NULL;
}

vtkDistributedDataFilter::~vtkDistributedDataFilter()
{ 
  if (this->Kdtree)
    {
    this->Kdtree->Delete();
    this->Kdtree = NULL;
    }
  
  this->SetController(NULL);

  if (this->ConvexSubRegionBounds)
    {
    delete [] this->ConvexSubRegionBounds;
    this->ConvexSubRegionBounds = NULL;
    } 
  
  if (this->GlobalIdArrayName) 
    {
    delete [] this->GlobalIdArrayName;
    }
  
  if (this->TimerLog)
    {
    this->TimerLog->Delete();
    this->TimerLog = 0;
    }
}

//-------------------------------------------------------------------------
// Help with the complex business of the global node ID array.  It may
// have been given to us by the user, we may have found it by looking 
// up common array names, or we may have created it ourselves.  We don't
// necessarily know the data type unless we created it ourselves.
//-------------------------------------------------------------------------

const char *vtkDistributedDataFilter::GetGlobalNodeIdArray(vtkDataSet *set)
{
  //------------------------------------------------
  // list common names for global node id arrays here
  //
  int nnames = 1;
  const char *arrayNames[1] = {
     "GlobalNodeId"  // vtkExodusReader name
     };
  //------------------------------------------------

  const char *gidArrayName = NULL;

  if (this->GlobalIdArrayName)
    {
    vtkDataArray *da = set->GetPointData()->GetArray(this->GlobalIdArrayName);

    if (da)
      {
      // The user gave us the name of the field containing global
      // node ids.

      gidArrayName = this->GlobalIdArrayName;
      }
    }

  if (!gidArrayName)
    {
    vtkDataArray *da = set->GetPointData()->GetArray(
       vtkDistributedDataFilter::TemporaryGlobalNodeIds);

    if (da)
      {
      // We created in parallel a field of global node ids.
      //

      gidArrayName = vtkDistributedDataFilter::TemporaryGlobalNodeIds;
      }
    }

  if (!gidArrayName)
    {
    // Maybe we can find a global node ID array

    for (int nameId=0; nameId < nnames; nameId++)
      {
      vtkDataArray *da = set->GetPointData()->GetArray(arrayNames[nameId]);

      if (da)
        {
        this->SetGlobalIdArrayName(arrayNames[nameId]);
        gidArrayName = arrayNames[nameId];
        break;
        }
      }
    }

  return gidArrayName;
}

int vtkDistributedDataFilter::GlobalNodeIdAccessGetId(int idx)
{
  if (this->GlobalIdArrayIdType)
    return (int)this->GlobalIdArrayIdType[idx];
  else if (this->GlobalIdArrayLong)
    return (int)this->GlobalIdArrayLong[idx];
  else if (this->GlobalIdArrayInt)
    return (int)this->GlobalIdArrayInt[idx];
  else if (this->GlobalIdArrayShort)
    return (int)this->GlobalIdArrayShort[idx];
  else if (this->GlobalIdArrayChar)
    return (int)this->GlobalIdArrayChar[idx];
  else
    return 0;
}
int vtkDistributedDataFilter::GlobalNodeIdAccessStart(vtkDataSet *set)
{
  this->GlobalIdArrayChar = NULL;
  this->GlobalIdArrayShort = NULL;
  this->GlobalIdArrayInt = NULL;
  this->GlobalIdArrayLong = NULL;
  this->GlobalIdArrayIdType = NULL;

  const char *arrayName = this->GetGlobalNodeIdArray(set);

  if (!arrayName)
    {
    return 0;
    }

  vtkDataArray *da = set->GetPointData()->GetArray(arrayName);

  int type = da->GetDataType();

  switch (type)
  {
    case VTK_ID_TYPE:

      this->GlobalIdArrayIdType = ((vtkIdTypeArray *)da)->GetPointer(0);
      break;

    case VTK_CHAR:
    case VTK_UNSIGNED_CHAR:

      this->GlobalIdArrayChar = ((vtkCharArray *)da)->GetPointer(0);
      break;

    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT: 

      this->GlobalIdArrayShort = ((vtkShortArray *)da)->GetPointer(0);
      break;

    case VTK_INT: 
    case VTK_UNSIGNED_INT:

      this->GlobalIdArrayInt = ((vtkIntArray *)da)->GetPointer(0);
      break;

    case VTK_LONG:
    case VTK_UNSIGNED_LONG:

      this->GlobalIdArrayLong = ((vtkLongArray *)da)->GetPointer(0);
      break;

    default:
      return 0;
  }

  return 1;
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
vtkPKdTree *vtkDistributedDataFilter::GetKdtree()
{
  if (this->Kdtree == NULL)
    {
    this->Kdtree = vtkPKdTree::New();
    this->Kdtree->SetController(this->Controller);
    }

  return this->Kdtree;
}
unsigned long vtkDistributedDataFilter::GetMTime()
{
  unsigned long t1, t2;

  t1 = this->Superclass::GetMTime();
  if (this->Kdtree == NULL)
    {
    return t1;
    }
  t2 = this->Kdtree->GetMTime();
  if (t1 > t2)
    {
    return t1;
    }
  return t2;
}

void vtkDistributedDataFilter::SetController(vtkMultiProcessController *c)
{
  if (this->Kdtree)
    {
    this->Kdtree->SetController(c);
    }

  if ((c == NULL) || (c->GetNumberOfProcesses() == 0))
    {
    this->NumProcesses = 1;    
    this->MyId = 0;
    }

  if (this->Controller == c)
    {
    return;
    }

  this->Modified();

  if (this->Controller != NULL)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }

  if (c == NULL)
    {
    return;
    }

  vtkSocketController *sc = vtkSocketController::SafeDownCast(c);

  if (sc)
    {
    vtkErrorMacro(<<
      "vtkDistributedDataFilter communication will fail with a socket controller");

    return;
    }

  this->Controller = c;

  c->Register(this);
  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyId    = c->GetLocalProcessId();
}
//-------------------------------------------------------------------------
// Execute
//-------------------------------------------------------------------------

void vtkDistributedDataFilter::ExecuteInformation()
{
  vtkDataSet* input = this->GetInput();
  vtkUnstructuredGrid* output = this->GetOutput();

  if (input && output)
    {
    output->CopyInformation(input);
    output->SetMaximumNumberOfPieces(-1);
    }
}

void vtkDistributedDataFilter::Execute()
{
  vtkDataSet *input  = this->GetInput();
  vtkDataSet *output  = this->GetOutput();
  vtkDataSet *inputPlus = NULL;

  vtkDebugMacro(<< "vtkDistributedDataFilter::Execute()");

  if (input->GetNumberOfCells() < 1)
    {
    vtkErrorMacro("Empty input");
    return;
    }

  this->GhostLevel = output->GetUpdateGhostLevel();

  if ( (this->NumProcesses == 1) && !this->RetainKdtree)
    {
    // Output is a new grid.  It is the input grid, with
    // duplicate points removed.  Duplicate points arise
    // when the input was read from a data set distributed
    // across more than one file.  

    this->SingleProcessExecute();

    return;
    }

  int HasIdTypeArrays = this->CheckFieldArrayTypes(input);

  if (HasIdTypeArrays)
    {
    // We use vtkDataWriter to marshall portions of data sets to send to other
    // processes.  Unfortunately, this writer writes out vtkIdType arrays as
    // integer arrays.  Trying to combine that on receipt with your vtkIdType
    // array won't work.  We need to exit with an error while we think of the
    // best way to handle this situation.  In practice I would think data sets
    // read in from disk files would never have such a field, only data sets created
    // in VTK at run time would have such a field.

    vtkErrorMacro(<<
      "vtkDistributedDataFilter::Execute - Can't distribute vtkIdTypeArrays, sorry");
    return;
    }

  if (this->Kdtree == NULL)
    {
    this->Kdtree = vtkPKdTree::New();
    this->Kdtree->SetController(this->Controller);
    }

  // Stage (1) - use vtkPKdTree to...
  //   Create a load balanced spatial decomposition in parallel.
  //   Create tables telling us how many cells each process has for
  //    each spatial region.
  //   Create a table assigning regions to processes.
  //
  // Note k-d tree will only be re-built if input or parameters
  // have changed on any of the processing nodes.

  int regionAssignmentScheme = this->Kdtree->GetRegionAssignment();

  if (regionAssignmentScheme == vtkPKdTree::NoRegionAssignment)
    {
    this->Kdtree->AssignRegionsContiguous();
    }

  this->Kdtree->SetDataSet(input);

  this->Kdtree->ProcessCellCountDataOn();

  TIMER("Build K-d tree in parallel");

  this->Kdtree->BuildLocator();

  TIMERDONE("Build K-d tree in parallel");

  if (this->Kdtree->GetNumberOfRegions() == 0)
    {
    vtkErrorMacro("Unable to build k-d tree structure");
    return;
    }

  if (this->NumProcesses == 1)
    {
    this->SingleProcessExecute();
    return;
    }

  // For aquiring ghost cells and for clipping, we'll need to
  // know whether our assigned spatial region is a single
  // convex region.  (It will be, assuming regions were
  // assigned contiguously.)

  if (this->ClipCells || (this->GhostLevel > 0))
    {
    vtkIntArray *myRegions = vtkIntArray::New();

    this->Kdtree->GetRegionAssignmentList(this->MyId, myRegions);

    this->NumConvexSubRegions = 
      this->Kdtree->MinimalNumberOfConvexSubRegions(
        myRegions, &this->ConvexSubRegionBounds);

    myRegions->Delete();
    }

  // Stage (2) - Redistribute data, so that each process gets a ugrid
  //   containing the cells in it's assigned spatial regions.  (Note
  //   that a side effect of merging the grids received from different
  //   processes is that the final grid has no duplicate points.)
  //
  //   If we plan to acquire ghost cells:
  //
  //     We will add point and cell arrays, that indicate ghost level, 
  //     before redistribution.  If IncludeAllIntersectingCells is ON, 
  //     all the cells and points have ghost level of 0.
  //
  //     If IncludeAllIntersectingCells is OFF, and we will be aquiring
  //     ghost cells, we instead create a ugrid on each process that DOES
  //     have all intersecting cells.  We mark the ghost level 0 cells
  //     for that process (the cells they normally would have received).  
  //     Including all intersecting cells in the redistribution 
  //     simplifies the creation of ghost cells later on.

  if (this->GhostLevel > 0)
    {
    // We'll need some temporary global cell IDs later on 
    // when we add ghost cells to our data set.  vtkPKdTree 
    // knows how many cells each process read in, so we'll 
    // use that to create cell IDs.

    TIMER("Create temporary global cell IDs");

    vtkIntArray *ids = this->AssignGlobalCellIds(); 

    inputPlus = input->NewInstance();
    inputPlus->ShallowCopy(input);

    inputPlus->GetCellData()->AddArray(ids);

    ids->Delete();

    TIMERDONE("Create temporary global cell IDs");
    }
  else
    {
    inputPlus = input;
    }

  TIMER("Redistribute data among processors");

  vtkUnstructuredGrid *finalGrid = NULL;

#ifdef VTK_USE_MPI

  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  if (mpiContr)
    {
    finalGrid = this->MPIRedistribute(mpiContr, inputPlus);   // faster
    }
  else
    {
    finalGrid = this->GenericRedistribute(inputPlus);
    }
#else

  // No MPI controller.  This is currently never used.  A socket
  // controller would fail because comm routines are not written for
  // them.  A threaded controller would fail because D3 is not yet
  // threadsafe.

  finalGrid = this->GenericRedistribute(inputPlus);  
#endif

  if (inputPlus != input)
    {
    inputPlus->Delete();
    inputPlus = NULL;
    }

  TIMERDONE("Redistribute data among processors");

  if (finalGrid == NULL)
    {
    vtkErrorMacro("Unable to redistribute data");
    return;
    }

  const char *nodeIdArrayName = this->GetGlobalNodeIdArray(finalGrid);

  if (!nodeIdArrayName &&            // we don't have global point IDs
      ((this->GhostLevel > 0) ||     // we need ids for ghost cell computation
        this->GlobalIdArrayName))    // user requested that we compute ids
    {
    // Create unique global point IDs across processes

    TIMER("Create global node ID array");
    this->AssignGlobalNodeIds(finalGrid);
    TIMERDONE("Create global node ID array");
    }

  if (this->GhostLevel > 0)
    {
    // If ghost cells were requested, then obtain enough cells from
    // neighbors so we will have the required levels of ghost cells.
    // AddGhostCells deletes finalGrid and creates expandedGrid.

    TIMER("Add ghost cells");

    vtkUnstructuredGrid *expandedGrid= this->AddGhostCells(finalGrid);

    TIMERDONE("Add ghost cells");

    expandedGrid->GetCellData()->RemoveArray(
      vtkDistributedDataFilter::TemporaryGlobalCellIds);

    if (this->GlobalIdArrayName == NULL)
      {
      expandedGrid->GetPointData()->RemoveArray(
        vtkDistributedDataFilter::TemporaryGlobalNodeIds);
      }

    finalGrid = expandedGrid;
    }

  // Possible Stage (3) - Clip cells to the spatial region boundaries

  if (this->ClipCells)
    {
    // The clipping process introduces new points, and interpolates
    // the point arrays.  Interpolating the global point id is
    // meaningless, so we remove that array.

    if (this->GlobalIdArrayName) 
      {
      finalGrid->GetPointData()->RemoveArray(this->GlobalIdArrayName);
      this->GlobalIdArrayName = NULL;
      }

    TIMER("Clip boundary cells to region");
    
    this->ClipCellsToSpatialRegion(finalGrid);

    TIMERDONE("Clip boundary cells to region");
    }

  this->GetOutput()->ShallowCopy(finalGrid);
  
  finalGrid->Delete();

  if (!this->RetainKdtree)
    {
    this->Kdtree->Delete();
    this->Kdtree = NULL;
    }
}
void vtkDistributedDataFilter::SingleProcessExecute()
{
  vtkDataSet *input               = this->GetInput();
  vtkUnstructuredGrid *output     = this->GetOutput();

  vtkDebugMacro(<< "vtkDistributedDataFilter::SingleProcessExecute()");

  // we run the input through vtkMergeCells which will remove
  // duplicate points

  vtkMergeCells *merged = vtkMergeCells::New();

  merged->SetTotalNumberOfCells(input->GetNumberOfCells());
  merged->SetTotalNumberOfPoints(input->GetNumberOfPoints());
  merged->SetTotalNumberOfDataSets(1);

  merged->SetUnstructuredGrid(output);

  const char *arrayName = this->GetGlobalNodeIdArray(input);

  if (arrayName)
    {
    merged->SetGlobalIdArrayName(arrayName); // faster
    }
  else
    {
    merged->SetPointMergeTolerance(0.0);
    }

  vtkDataSet* tmp = input->NewInstance();
  tmp->ShallowCopy(input);

  merged->MergeDataSet(tmp);
  tmp->Delete();

  merged->Finish();
  merged->Delete();

  if (this->GhostLevel > 0)
    {
    // Add the vtkGhostLevels arrays.  We have the whole
    // data set, so all cells are level 0.

    this->AddConstantUnsignedCharPointArray(output, "vtkGhostLevels", 0);
    this->AddConstantUnsignedCharCellArray(output, "vtkGhostLevels", 0);
    }
}
int vtkDistributedDataFilter::CheckFieldArrayTypes(vtkDataSet *set)
{
  int i;

  // problem - vtkIdType arrays are written out as int arrays
  // when marshalled with vtkDataWriter.  This is a problem
  // when receive the array and try to merge it with our own,
  // which is a vtkIdType

  vtkPointData *pd = set->GetPointData();
  vtkCellData *cd = set->GetCellData();

  int npointArrays = pd->GetNumberOfArrays();

  for (i=0; i<npointArrays; i++)
    {
    int arrayType = pd->GetArray(i)->GetDataType();

    if (arrayType == VTK_ID_TYPE)
      {
      return 1;
      }
    }

  int ncellArrays = cd->GetNumberOfArrays();

  for (i=0; i<ncellArrays; i++)
    {
    int arrayType = cd->GetArray(i)->GetDataType();

    if (arrayType == VTK_ID_TYPE)
      {
      return 1;
      }
    }

  return 0;
}
//-------------------------------------------------------------------------
// Communication routines
//-------------------------------------------------------------------------

// A generic routine for pairwise exchanges of information, which D3
// does quite a lot of.
//
// Provide an array with an entry for each process.  The array value
// is the size of the data object I want to send to each process.
// Also provide array with a pointer for each process.  The array value
// points to the data to send to that process.
//
// This routine will overwrite these arrays.  The first array will
// indicate the size of the data object sent to you by each process.
// The second array will contain pointers to the data objects you
// received.

int vtkDistributedDataFilter::PairWiseDataExchange(int *yourSize, 
                                                   char **yourData, int tag)
{
#ifdef VTK_USE_MPI
  int i;

  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  if (mpiContr == NULL)
    {
    // Right now this would never happen.  The only non-MPI jobs that
    // would use D3 would be serial, and would never make it to here.
  
    vtkErrorMacro(<< "PairWiseDataExchange does not work without MPI");
    return 1;
    }
  
  int iam    = this->MyId;
  int nprocs = this->NumProcesses;

  // First, do pairwise exchanges of the sizes of the transfers

  int *mySize = new int [nprocs];
  mySize[iam] = 0;

  int offset, source, target;

  vtkMPICommunicator::Request req;

  for (offset = 1; offset < nprocs; offset++)
    {
    target = (iam + offset) % nprocs;
    source = (iam + nprocs - offset) % nprocs;

    // Post to get count from source

    mpiContr->NoBlockReceive(mySize + source, 1, source, tag, req);

    // Send count to target

    mpiContr->Send(yourSize + target, 1, target, tag);

    // Wait for source

    req.Wait();
    }

  // Allocate storage for data I am to receive

  int status = 0;

  char **myData = new char * [nprocs];

  if (myData)
    {
    memset(myData, 0, sizeof(char *) * nprocs);
  
    for (i=0; i<nprocs; i++)
      {
      if (mySize[i] > 0)
        {
        myData[i] = new char [mySize[i]];

        if (myData[i] == NULL)
          {
          status = 1;
          break;
          }
        }
      }
    }
  else
    {
    status = 1;
    }

  if (status)
    {
    if (myData)
      {
      for (i=0; i<nprocs; i++)
        {
        if (myData[i]) delete [] myData[i];
        }
      delete [] myData;
      }

    delete [] mySize;
    vtkErrorMacro(<< 
      "vtkDistributedDataFilter::PairWiseDataExchange - memory allocation");
    return 1;   // error
    }

  // Now do pairwise exchanges of the data

  for (offset = 1; offset < nprocs; offset++)
    {
    target = (iam + offset) % nprocs;
    source = (iam + nprocs - offset) % nprocs;
  
    // Post to get data from source

    if (mySize[source] > 0)
      {
      mpiContr->NoBlockReceive(myData[source], mySize[source], source, tag, req);
      }

    // Send data to target
  
    if (yourSize[target] > 0)
      {
      mpiContr->Send(yourData[target], yourSize[target], target, tag);
      }
  
    // Wait for source 
  
    if (mySize[source] > 0)
      {
      req.Wait();
      }
    }

  for (i=0; i<nprocs; i++)
    {
    yourSize[i] = mySize[i];
    yourData[i] = myData[i];
    }

  delete [] myData;
  delete [] mySize;

  return 0;
#endif
}

#ifdef VTK_USE_MPI
vtkUnstructuredGrid 
  *vtkDistributedDataFilter::MPIRedistribute(vtkMPIController *mpiContr,
                                             vtkDataSet *in)
{
  int proc;
  int me = this->MyId;
  int nnodes = this->NumProcesses;

  // Every process has been assigned a spatial region.  Create a
  // ugrid for every process, containing the cells I have read in
  // that are in their spatial region.

  TIMER("Create cell lists");
  
  if (this->IncludeAllIntersectingCells || (this->GhostLevel > 0))
    {
    this->Kdtree->IncludeRegionBoundaryCellsOn();
    }
  
  this->Kdtree->CreateCellLists();  // req'd by ExtractCellsForProcess
    
  TIMERDONE("Create cell lists");
    
  TIMER("Extract sub grids for others");

  vtkUnstructuredGrid *mySubGrid = NULL;

  char **yourPackedGrids = new char * [nnodes];
  int *yourSizeData  = new int [nnodes];

  if (!yourPackedGrids || !yourSizeData)
    {
    if (yourSizeData) delete [] yourSizeData;
    if (yourPackedGrids) delete [] yourPackedGrids;
    vtkErrorMacro(<< 
      "vtkDistributedDataFilter::MPIRedistribute - memory allocation");
    return NULL;
    }

  for (proc=0; proc < nnodes; proc++)
    {
    yourSizeData[proc] = 0;
    yourPackedGrids[proc] = NULL;

    vtkUnstructuredGrid *extractedGrid = this->ExtractCellsForProcess(proc, in);

    if (!extractedGrid )
      {
      continue; // This process has no assigned regions
      }

    if (extractedGrid->GetNumberOfCells()==0)
      {
      if (proc == me)
        {
        // Must start the merging process with field arrays read from disk,
        // not field arrays received from another process that have been
        // marshalled/unmarshalled.  This is so field array order is
        // the same on every process.  So we hold on to this grid of
        // zero cells and will give it to vtkMergeCells as first dataset.

        mySubGrid = extractedGrid;
        }
      else
        {
        extractedGrid->UnRegister(this);
        }

      continue;
      } 

    if (proc != me)
      {
      yourPackedGrids[proc] = this->MarshallDataSet(extractedGrid, yourSizeData[proc]);
      extractedGrid->UnRegister(this);
      }
    else
      {
      mySubGrid = extractedGrid;
      }
    }
  this->Kdtree->DeleteCellLists();

  TIMERDONE("Extract sub grids for others");

  // Exchange sub grids with other processes

  TIMER("Exchange sub grids");

  int *mySizeData  = new int [nnodes];
  char **myPackedGrids = new char * [nnodes];

  if (!myPackedGrids || !mySizeData)
    {
    if (mySizeData) delete [] mySizeData;
    if (myPackedGrids) delete [] myPackedGrids;
    for (proc=0; proc < nnodes; proc++) 
      {
      if (yourPackedGrids[proc]) delete [] yourPackedGrids[proc]; 
      }
    delete [] yourSizeData;
    delete [] yourPackedGrids;
    vtkErrorMacro(<< 
      "vtkDistributedDataFilter::MPIRedistribute - memory allocation");
    return NULL;
    }

  memcpy(mySizeData, yourSizeData, sizeof(int) * nnodes);
  memcpy(myPackedGrids, yourPackedGrids, sizeof(char *) * nnodes);

  int tag = 0x0011;

  int rc = this->PairWiseDataExchange(mySizeData, myPackedGrids, tag);

  for (proc=0; proc<nnodes; proc++)
    {
    if (yourPackedGrids[proc]) delete [] yourPackedGrids[proc];
    }
  delete [] yourPackedGrids;
  delete [] yourSizeData;

  TIMERDONE("Exchange sub grids");

  if (rc || (mySubGrid == NULL))
    {
    delete [] mySizeData;
    delete [] myPackedGrids;

    if (rc)
      {
      vtkErrorMacro(<< 
        "vtkDistributedDataFilter::MPIRedistribute: memory allocation");
      }
    return NULL;
    }

  TIMER("Unmarshall received sub grids");

  // Unpack the received grids

  vtkUnstructuredGrid **subGrid = new vtkUnstructuredGrid * [nnodes];
  memset(subGrid, 0, sizeof(vtkUnstructuredGrid *) * nnodes);

  vtkIdType TotalCells = mySubGrid->GetNumberOfCells();
  vtkIdType TotalPoints = mySubGrid->GetNumberOfPoints();
  int TotalSets = 1;

  for (proc=0; proc<nnodes; proc++)
    {
    if (proc == me) continue;

    if (mySizeData[proc] > 0)
      {
      subGrid[proc] = this->UnMarshallDataSet(myPackedGrids[proc], mySizeData[proc]);

      delete [] myPackedGrids[proc];

      TotalCells += subGrid[proc]->GetNumberOfCells();
      TotalPoints += subGrid[proc]->GetNumberOfPoints();
      TotalSets += 1;
      }
    }

  delete [] mySizeData;
  delete [] myPackedGrids;

  TIMERDONE("Unmarshall received sub grids");

  // initialize my new ugrid - use vtkMergeCells object which can merge
  //   in ugrids with same field arrays, filtering out duplicate points
  //   as it goes.

  TIMER("Merge into new grid");

  vtkUnstructuredGrid *newGrid = vtkUnstructuredGrid::New();

  vtkMergeCells *merged = vtkMergeCells::New();

  merged->SetTotalNumberOfCells(TotalCells);
  merged->SetTotalNumberOfPoints(TotalPoints);
  merged->SetTotalNumberOfDataSets(TotalSets);

  merged->SetUnstructuredGrid(newGrid);

  const char *nodeIdArrayName  = this->GetGlobalNodeIdArray(mySubGrid);

  if (nodeIdArrayName)
    {
    merged->SetGlobalIdArrayName(nodeIdArrayName);
    }
  else
    {
    merged->SetPointMergeTolerance(this->Kdtree->GetFudgeFactor());
    }

  merged->MergeDataSet(mySubGrid);
  mySubGrid->UnRegister(this);

  for (proc=0; proc < nnodes; proc++)
    {
    if (subGrid[proc])
      {
      merged->MergeDataSet(subGrid[proc]);

      subGrid[proc]->Delete();
      }
    }

  delete [] subGrid;

  TIMERDONE("Merge into new grid");

  TIMER("Finish merge");

  merged->Finish();
  merged->Delete();

  TIMERDONE("Finish merge");

  return newGrid;
}
#endif

char *vtkDistributedDataFilter::MarshallDataSet(vtkUnstructuredGrid *extractedGrid, int &len)
{
  // taken from vtkCommunicator::WriteDataSet

  vtkUnstructuredGrid *copy;
  vtkDataSetWriter *writer = vtkDataSetWriter::New();

  copy = extractedGrid->NewInstance();
  copy->ShallowCopy(extractedGrid);

  // There is a problem with binary files with no data.
  if (copy->GetNumberOfCells() > 0)
    {
    writer->SetFileTypeToBinary();
    }
  writer->WriteToOutputStringOn();
  writer->SetInput(copy);

  writer->Write();

  len = writer->GetOutputStringLength();

  char *packedFormat = writer->RegisterAndGetOutputString();

  writer->Delete();

  copy->Delete();

  return packedFormat;
}
vtkUnstructuredGrid *vtkDistributedDataFilter::UnMarshallDataSet(char *buf, int size)
{
  // taken from vtkCommunicator::ReadDataSet

  vtkDataSetReader *reader = vtkDataSetReader::New();

  reader->ReadFromInputStringOn();

  vtkCharArray* mystring = vtkCharArray::New();
  
  mystring->SetArray(buf, size, 1);

  reader->SetInputArray(mystring);
  mystring->Delete();

  vtkDataSet *output = reader->GetOutput();
  output->Update();

  vtkUnstructuredGrid *newGrid = vtkUnstructuredGrid::New();

  newGrid->ShallowCopy(output);

  reader->Delete();

  return newGrid;
}

// This parallel redistribution does not require MPI.  It will never
// be used unless this class is made threadsafe.

vtkUnstructuredGrid *vtkDistributedDataFilter::GenericRedistribute(vtkDataSet *in)
{
  vtkUnstructuredGrid *myGrid = NULL; 

  if (this->IncludeAllIntersectingCells)
    {
    this->Kdtree->IncludeRegionBoundaryCellsOn();
    }
  
  this->Kdtree->CreateCellLists();  // req'd by ExtractCellsForProcess

  for (int proc = 0; proc < this->NumProcesses; proc++)
    {
    vtkUnstructuredGrid *ugrid = this->ExtractCellsForProcess(proc, in);

    if (ugrid == NULL) continue;   // process is assigned no regions

    // Fan in and merge ugrids *************************************
    // If I am "proc", my grid is returned *************************
    // This call also deletes ugrid at the earliest opportunity ****

    vtkUnstructuredGrid *someGrid = this->ReduceUgridMerge(ugrid, proc);

    if (this->MyId == proc)
      {
      myGrid = someGrid;
      }
    }
  this->Kdtree->DeleteCellLists();

  return myGrid;
}
vtkUnstructuredGrid 
  *vtkDistributedDataFilter::ExtractCellsForProcess(int proc, vtkDataSet *in)
{
  vtkUnstructuredGrid *ugrid = NULL;

  vtkIntArray *regions = vtkIntArray::New();

  int nregions = this->Kdtree->GetRegionAssignmentList(proc, regions);

  if (nregions == 0)
    {
    return ugrid;
    }

  if ((this->GhostLevel > 0) && (this->IncludeAllIntersectingCells == 0))
    {
    // Ghost cell generation later on will go faster if we actually
    // *do* include all intersecting cells, and if we mark the cells
    // the process normally would have received as ghost level 0, and
    // mark the other cells as unknown ghost level.  

    ugrid = this->ExtractTwoGhostLevels(proc, in);

    }
  else 
    {
    // No ghost cells, or yes ghost cells but we are including all
    // intersecting cells in region sub-grids

    ugrid = this->ExtractOneGhostLevel(regions, in);
    }

  regions->Delete();

  return ugrid;
}
vtkUnstructuredGrid 
  *vtkDistributedDataFilter::ExtractTwoGhostLevels(int proc, vtkDataSet *in)
{
  vtkDataSet* tmpInput = in->NewInstance();
  tmpInput->ShallowCopy(in);

  vtkUnstructuredGrid *ugrid = NULL;
  vtkUnstructuredGrid *level0Grid = NULL;
  vtkUnstructuredGrid *ghostCellGrid = NULL;

  const char *nodeIdArrayName = this->GetGlobalNodeIdArray(in);

  // We need two lists of cell IDs. 
  //   1. All cells having their centroid in the process' regions.
  //   2. All cells which intersect the process' regions, but do
  //      not have their centroid in those regions.

  vtkIdList *insideCells = vtkIdList::New();
  vtkIdList *boundaryCells = vtkIdList::New();
  vtkExtractCells *extCells;

  this->Kdtree->GetCellListsForProcessRegions(proc, insideCells, boundaryCells);

  if (insideCells->GetNumberOfIds() > 0)
    {
    extCells = vtkExtractCells::New();

    extCells->SetInput(tmpInput);

    extCells->AddCellList(insideCells);
    insideCells->Delete();

    extCells->Update();

    level0Grid = extCells->GetOutput();
    level0Grid->Register(this);
    extCells->Delete();

    // these are the recipient's level 0 ghost cells

    this->AddConstantUnsignedCharCellArray(level0Grid, "vtkGhostLevels", 0);
    }

  if (boundaryCells->GetNumberOfIds() > 0)
    {
    extCells = vtkExtractCells::New();

    extCells->SetInput(tmpInput);

    extCells->AddCellList(boundaryCells);
    boundaryCells->Delete();

    extCells->Update();

    ghostCellGrid = extCells->GetOutput();
    ghostCellGrid->Register(this);
    extCells->Delete();

    // ghost level to be determined later by recipient of these cells

    this->AddConstantUnsignedCharCellArray(ghostCellGrid, "vtkGhostLevels", 
                                vtkDistributedDataFilter::UnsetGhostLevel);
    }

  if (ghostCellGrid && level0Grid)
    {
    vtkMergeCells *merged = vtkMergeCells::New();

    merged->SetTotalNumberOfCells(ghostCellGrid->GetNumberOfCells() +
                             level0Grid->GetNumberOfCells());
    merged->SetTotalNumberOfPoints(ghostCellGrid->GetNumberOfPoints() +
                             level0Grid->GetNumberOfPoints());
    merged->SetTotalNumberOfDataSets(2);

    if (nodeIdArrayName)
      {
      // to quickly filter out duplicate points
      merged->SetGlobalIdArrayName(nodeIdArrayName);
      }

    ugrid = vtkUnstructuredGrid::New();

    merged->SetUnstructuredGrid(ugrid);

    merged->MergeDataSet(level0Grid);
    level0Grid->Delete();

    merged->MergeDataSet(ghostCellGrid);
    ghostCellGrid->Delete();

    merged->Finish();
    merged->Delete();
    }
  else if (ghostCellGrid)
    {
    ugrid = ghostCellGrid;
    }
  else if (level0Grid)
    {
    ugrid = level0Grid;
    }
  else
    {
    // create an empty ugrid with the correct field arrays

    extCells = vtkExtractCells::New();
    extCells->SetInput(tmpInput);

    extCells->Update();

    ugrid = extCells->GetOutput();
    ugrid->Register(this);
    extCells->Delete();
    this->AddConstantUnsignedCharCellArray(ugrid, "vtkGhostLevels", 
                                vtkDistributedDataFilter::UnsetGhostLevel);
    }
  
  tmpInput->Delete();

  // For now set the ghost level of every point to "undefined".  This
  // will be set by the receiver in FixGhostLevels().

  this->AddConstantUnsignedCharPointArray(ugrid, "vtkGhostLevels", 
                         vtkDistributedDataFilter::UnsetGhostLevel);

  return ugrid;
}
vtkUnstructuredGrid *vtkDistributedDataFilter::ExtractOneGhostLevel(
                      vtkIntArray *regions, vtkDataSet *in)
{
  int reg;
  int nregions = regions->GetNumberOfTuples();

  vtkDataSet* tmpInput = in->NewInstance();
  tmpInput->ShallowCopy(in);

  vtkUnstructuredGrid *ugrid = NULL;

  vtkExtractCells *extCells = vtkExtractCells::New();

  extCells->SetInput(tmpInput);

  for (reg=0; reg < nregions; reg++)
    {
    extCells->AddCellList(this->Kdtree->GetCellList(regions->GetValue(reg)));

    if (this->IncludeAllIntersectingCells)
      {
      extCells->
        AddCellList(this->Kdtree->GetBoundaryCellList(regions->GetValue(reg)));
      }
    }

  extCells->Update();

  // If this process has no cells for these regions, a ugrid gets
  // created anyway with field array information

  ugrid = extCells->GetOutput();

  if (this->GhostLevel > 0)
    {
    // If we got here, then IncludeAllIntersectingCells is ON, and all
    // of these cells are recipient's 0-level ghost cells

    this->AddConstantUnsignedCharCellArray(ugrid, "vtkGhostLevels", 0);
    this->AddConstantUnsignedCharPointArray(ugrid, "vtkGhostLevels", 0);
    }

  ugrid->Register(this);

  extCells->Delete();

  tmpInput->Delete();

  return ugrid;
}


// Logarithmic fan-in of only the processes holding data for
// these regions.  Root of fan-in is the process assigned to
// the regions.

vtkUnstructuredGrid *vtkDistributedDataFilter::ReduceUgridMerge(
                                  vtkUnstructuredGrid *ugrid, int root)
{
  int i, ii;

  int iHaveData = (ugrid->GetNumberOfCells() > 0);
  int iAmRoot   = (root == this->MyId);

  if (!iHaveData && !iAmRoot)
    {
    ugrid->Delete();
    return 0;
    }

  vtkUnstructuredGrid *newGrid = vtkUnstructuredGrid::New();

  // get list of participants

  int nAllProcs = this->NumProcesses;
  
  int *haveData = new int [nAllProcs];
  memset(haveData, 0, sizeof(int) * nAllProcs);

  vtkIntArray *Ids = vtkIntArray::New();

  vtkIntArray *regions = vtkIntArray::New();

  int nregions = this->Kdtree->GetRegionAssignmentList(root, regions);
  
  for (int reg=0; reg < nregions; reg++)
    {
    // Get list of all processes that have data for this region

    Ids->Initialize();
    int nIds = this->Kdtree->GetProcessListForRegion(regions->GetValue(reg), Ids);

    for (int p=0; p<nIds; p++)
      {
      haveData[Ids->GetValue(p)] = 1;
      } 
    } 
  regions->Delete();

  Ids->Delete();

  int nParticipants = 0;

  haveData[root] = 1;

  for (i=0; i<nAllProcs; i++)
    {
    if (haveData[i])
      {
      nParticipants++;
      }
    }

  if (nParticipants == 1)
    {
    newGrid->ShallowCopy(ugrid);
    ugrid->Delete();

    return newGrid;
    }

  int *member = new int [nParticipants];
  int myLocalRank;

  member[0] = root;

  if (iAmRoot) myLocalRank = 0;

  for (i=0, ii=1; i<nAllProcs; i++)
    {
    if (haveData[i] && (i != root))
      {
      if (i == this->MyId)
        {
        myLocalRank = ii;
        }
      member[ii++] = i;
      }
    }

  delete [] haveData;

  // determine who sends me ugrids, and who I send to

  int *source;
  int target, nsources, ntargets;

  vtkDistributedDataFilter::ComputeFanIn(member, 
                              nParticipants, myLocalRank,
                              &source, &nsources,
                              &target, &ntargets);

  delete [] member;

  // How many points and cells total in my final ugrid

  vtkIdType TotalPoints = ugrid->GetNumberOfPoints();
  vtkIdType TotalCells = ugrid->GetNumberOfCells();
  int data[2];
  int OKToSend = 1;
  int tag = root;   // uniquely identifies this fan-in

  for (i=0; i<nsources; i++)
    {
    this->Controller->Receive(data, 2, source[i], tag);

    TotalPoints += data[0];
    TotalCells += data[1];
    }

  vtkMergeCells *merged = vtkMergeCells::New();

  merged->SetTotalNumberOfCells(TotalCells);
  merged->SetTotalNumberOfPoints(TotalPoints);
  merged->SetTotalNumberOfDataSets(nsources + 1);   // upper bound

  merged->SetUnstructuredGrid(newGrid);

  const char *arrayName = this->GetGlobalIdArrayName();

  if (arrayName) // filter out duplicate points
    {
    merged->SetGlobalIdArrayName(arrayName);
    }
  else
    {
    merged->SetPointMergeTolerance(this->Kdtree->GetFudgeFactor());
    }

  if (iHaveData)
    {
    merged->MergeDataSet(ugrid);
    }

  ugrid->Delete();

  for (i=0; i<nsources; i++)
    {
    // throttle sends to better manage memory

    this->Controller->Send(&OKToSend, 1, source[i], tag);

    vtkUnstructuredGrid *remoteGrid = vtkUnstructuredGrid::New();

    this->Controller->Receive(static_cast<vtkDataObject *>(remoteGrid),
                              source[i], tag);

    merged->MergeDataSet(remoteGrid);

    remoteGrid->Delete();
    }
  delete [] source;

  merged->Finish();
  merged->Delete();

  if (ntargets > 0)
    {
    data[0] = (int)newGrid->GetNumberOfPoints();
    data[1] = (int)newGrid->GetNumberOfCells();

    this->Controller->Send(data, 2, target, tag);

    this->Controller->Receive(&OKToSend, 1, target, tag);

    this->Controller->Send(static_cast<vtkDataObject *>(newGrid),
                           target, tag);

    newGrid->Delete();
    newGrid = NULL;
    }
  return newGrid;  // non-Null only if I am root
}

void vtkDistributedDataFilter::ComputeFanIn(int *member, 
                              int nParticipants, int myLocalRank,
                              int **source, int *nsources,
                              int *target, int *ntargets)
{
  int nTo = 0;
  int nFrom = 0;

  int fanInTo;
  int *fanInFrom = new int [20];

  for (int i = 1; i < nParticipants; i <<= 1)
    {
    int other = myLocalRank ^ i;

    if (other >= nParticipants) continue;

    if (myLocalRank > other)
      {
      fanInTo = member[other];

      nTo++;   /* one at most */

      break;
      }
    else
      {
      fanInFrom[nFrom++] = member[other];
      }
    }

  *source = fanInFrom;
  *target = fanInTo;
  *nsources = nFrom;
  *ntargets = nTo;

  return;
}

//-------------------------------------------------------------------------
// Code related to clipping cells to the spatial region 
//-------------------------------------------------------------------------

static int insideBoxFunction(vtkIdType cellId, vtkUnstructuredGrid *grid, void *data)
{   
  char *arrayName = (char *)data;

  vtkDataArray *da= grid->GetCellData()->GetArray(arrayName);
  vtkUnsignedCharArray *inside = vtkUnsignedCharArray::SafeDownCast(da);

  unsigned char where = inside->GetValue(cellId);

  return where;   // 1 if cell is inside spatial region, 0 otherwise
}
void vtkDistributedDataFilter::AddConstantUnsignedCharPointArray(
  vtkUnstructuredGrid *grid, const char *arrayName, unsigned char val)
{
  vtkIdType npoints = grid->GetNumberOfPoints();

  unsigned char *vals = new unsigned char [npoints];

  memset(vals, val, npoints);

  vtkUnsignedCharArray *Array = vtkUnsignedCharArray::New();
  Array->SetName(arrayName);
  Array->SetArray(vals, npoints, 0);

  grid->GetPointData()->AddArray(Array);

  Array->Delete();
}
void vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(
  vtkUnstructuredGrid *grid, const char *arrayName, unsigned char val)
{
  vtkIdType ncells = grid->GetNumberOfCells();

  unsigned char *vals = new unsigned char [ncells];

  memset(vals, val, ncells);

  vtkUnsignedCharArray *Array = vtkUnsignedCharArray::New();
  Array->SetName(arrayName);
  Array->SetArray(vals, ncells, 0);

  grid->GetCellData()->AddArray(Array);

  Array->Delete();
}

// this is here temporarily, until vtkBoxClipDataSet is fixed to
// be able to generate the clipped output

void vtkDistributedDataFilter::ClipWithVtkClipDataSet(
           vtkUnstructuredGrid *grid, double *bounds, 
           vtkUnstructuredGrid **outside, vtkUnstructuredGrid **inside)
{
  vtkUnstructuredGrid *in;
  vtkUnstructuredGrid *out ;

  vtkClipDataSet *clipped = vtkClipDataSet::New();

  vtkBox *box = vtkBox::New();
  box->SetBounds(bounds);

  clipped->SetClipFunction(box);
  box->Delete();
  clipped->SetValue(0.0);
  clipped->InsideOutOn();

  clipped->SetInput(grid);

  if (outside)
    {
    clipped->GenerateClippedOutputOn(); 
    }

  clipped->Update();

  if (outside)
    {
    out = clipped->GetClippedOutput();
    out->Register(this);
    *outside = out;
    }

  in = clipped->GetOutput();
  in->Register(this);
  *inside = in;

  clipped->Delete();
}

// In general, vtkBoxClipDataSet is much faster and makes fewer errors.

void vtkDistributedDataFilter::ClipWithBoxClipDataSet(
           vtkUnstructuredGrid *grid, double *bounds, 
           vtkUnstructuredGrid **outside, vtkUnstructuredGrid **inside)
{
  vtkUnstructuredGrid *in;
  vtkUnstructuredGrid *out ;

  vtkBoxClipDataSet *clipped = vtkBoxClipDataSet::New();

  clipped->SetBoxClip(bounds[0], bounds[1],
                      bounds[2], bounds[3], bounds[4], bounds[5]);

  clipped->SetInput(grid);

  if (outside)
    {
    clipped->GenerateClippedOutputOn();  
    }

  clipped->Update();

  if (outside)
    {
    out = clipped->GetClippedOutput();
    out->Register(this);
    *outside = out;
    }

  in = clipped->GetOutput();
  in->Register(this);
  *inside = in;

  clipped->Delete();
}

void vtkDistributedDataFilter::ClipCellsToSpatialRegion(vtkUnstructuredGrid *grid)
{
  if (this->NumConvexSubRegions > 1)
    {
    // here we would need to divide the grid into a separate grid for
    // each convex region, and then do the clipping

    vtkErrorMacro(<<
       "vtkDistributedDataFilter::ClipCellsToSpatialRegion - "
       "assigned regions do not form a single convex region");

    return ;
    }

  double *bounds = this->ConvexSubRegionBounds;

  if (this->GhostLevel > 0)
    {
    // We need cells outside the clip box as well.  

    vtkUnstructuredGrid *outside;
    vtkUnstructuredGrid *inside;

    TIMER("Clip");

#if 1
    this->ClipWithBoxClipDataSet(grid, bounds, &outside, &inside);
#else
    this->ClipWithVtkClipDataSet(grid, bounds, &outside, &inside);
#endif

    TIMERDONE("Clip");

    grid->Initialize();

    // Mark the outside cells with a 0, the inside cells with a 1.

    int arrayNameLen = strlen(vtkDistributedDataFilter::TemporaryInsideBoxFlag);
    char *arrayName = new char [arrayNameLen + 1];
    strcpy(arrayName, vtkDistributedDataFilter::TemporaryInsideBoxFlag);

    this->AddConstantUnsignedCharCellArray(outside, arrayName, 0);
    this->AddConstantUnsignedCharCellArray(inside, arrayName, 1);

    // Combine inside and outside into a single ugrid.

    TIMER("Merge inside and outside");

    vtkUnstructuredGrid *combined = vtkUnstructuredGrid::New();
    vtkMergeCells *merge = vtkMergeCells::New();

    merge->SetTotalNumberOfCells( outside->GetNumberOfCells() +
                             inside->GetNumberOfCells());
    merge->SetTotalNumberOfPoints( outside->GetNumberOfPoints() +
                             inside->GetNumberOfPoints());
    merge->SetTotalNumberOfDataSets(2);
    merge->SetUnstructuredGrid(combined);

    // try not to duplicate the points on the boundary of the box

    merge->SetPointMergeTolerance(this->Kdtree->GetFudgeFactor());

    merge->MergeDataSet(inside);
    inside->Delete();

    merge->MergeDataSet(outside);
    outside->Delete();

    merge->Finish();
    merge->Delete();

    TIMERDONE("Merge inside and outside");

    // Extract the piece inside the box (level 0) and the requested
    // number of levels of ghost cells.

    TIMER("Extract desired cells");

    vtkExtractUserDefinedPiece *ep = vtkExtractUserDefinedPiece::New();

    ep->SetConstantData(arrayName, arrayNameLen + 1);
    ep->SetPieceFunction(insideBoxFunction);
    ep->CreateGhostCellsOn();

    ep->GetOutput()->SetUpdateGhostLevel(this->GhostLevel);
    ep->SetInput(combined);

    ep->Update();

    grid->ShallowCopy(ep->GetOutput());
    grid->GetCellData()->RemoveArray(arrayName);
    
    ep->Delete();
    combined->Delete();

    TIMERDONE("Extract desired cells");

    delete [] arrayName;
    }
  else
    {
    vtkUnstructuredGrid *inside;

#if 1
    this->ClipWithBoxClipDataSet(grid, bounds, NULL, &inside);
#else
    this->ClipWithVtkClipDataSet(grid, bounds, NULL, &inside);
#endif

    grid->ShallowCopy(inside);
    inside->Delete();
    }

  return;
}

//-------------------------------------------------------------------------
// Code related to assigning global node IDs and cell IDs
//-------------------------------------------------------------------------

void vtkDistributedDataFilter::AssignGlobalNodeIds(vtkUnstructuredGrid *grid)
{
  int nprocs = this->NumProcesses;
  int processId;
  int ptId;
  vtkIdType nGridPoints = grid->GetNumberOfPoints();

  int *numPointsInside = new int [nprocs];

  int *numPointsOutside = new int [nprocs];
  memset(numPointsOutside, 0, sizeof(int) * nprocs);

  vtkIntArray *globalIds = vtkIntArray::New();
  globalIds->SetNumberOfValues(nGridPoints);

  // 1. Count the points in grid which lie within my assigned spatial region

  int nInsidePoints = 0;

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    double *pt = grid->GetPoints()->GetPoint(ptId);

    if (this->InMySpatialRegion(pt[0], pt[1], pt[2]))
      {
      globalIds->SetValue(ptId, 0);  // flag it as mine
      nInsidePoints++;
      }
    else
      {
      // Well, whose region is this point in?

      int regionId = this->Kdtree->GetRegionContainingPoint(pt[0],pt[1],pt[2]);

      processId = this->Kdtree->GetProcessAssignedToRegion(regionId);

      numPointsOutside[processId]++;
      
      processId += 1;
      processId *= -1;

      globalIds->SetValue(ptId, processId);
      }
    }

  // 2. Gather and Broadcast this number of "Inside" points for each process.

  int *arrayLength = new int [nprocs];
  char **ptarrays = new char * [nprocs]; 

  for (processId = 0; processId < nprocs; processId++)
    {
    arrayLength[processId] = (processId == this->MyId) ? 0 : sizeof(int);
    ptarrays[processId] = (char *)&nInsidePoints;
    }
  
  int tag = 0x0177;

  int rc = this->PairWiseDataExchange(arrayLength, ptarrays, tag);
                                                                                              for (processId = 0; processId < nprocs; processId++)
    {
    if (processId == this->MyId)
      {
      numPointsInside[processId] = nInsidePoints;
      }
    else 
      {
      numPointsInside[processId] = *(int *)ptarrays[processId];
 
      delete [] ptarrays[processId];
      }
    }

  // 3. Assign global Ids to the points inside my spatial region

  int firstId = 0;

  for (processId = 0; processId < this->MyId; processId++)
    {
    firstId += numPointsInside[processId];
    }

  delete [] numPointsInside;

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    if (globalIds->GetValue(ptId) == 0)
      {
      globalIds->SetValue(ptId, firstId++);
      }
    }

  // -----------------------------------------------------------------
  // All processes have assigned global IDs to the points within their
  // assigned spatial region.  Now they have to get the IDs for the
  // points in their grid which lie outside their region.
  // -----------------------------------------------------------------

  // 4. For every other process, build a list of points I have
  // which are in the region of that process.  In practice, the
  // processes for which I need to request points IDs should be
  // a small subset of all the other processes.

  // question: if the vtkPointArray has type double, should we
  // send doubles instead of floats to insure we get the right
  // global ID back?

  float **ptarrayOut     = new float * [nprocs];
  int **localIds = new int * [nprocs];

  for (processId = 0; processId < nprocs; processId++)
    {
    int numpoints = numPointsOutside[processId];

    if (numpoints > 0)
      {
      ptarrayOut[processId] = new float [numpoints * 3];
      localIds[processId] = new int [numpoints];
      }
    else
      {
      ptarrayOut[processId] = NULL;
      localIds[processId] = NULL;
      }
    }

  delete [] numPointsOutside;

  int *countYou = new int [nprocs];
  memset(countYou, 0, sizeof(int) * nprocs);

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    processId = globalIds->GetValue(ptId);

    if (processId >= 0) continue;   // that's one of mine

    processId *= -1;
    processId -= 1;

    int nextId = countYou[processId];

    localIds[processId][nextId] = ptId;

    float *p = ptarrayOut[processId] + (nextId * 3);

    double *dp = grid->GetPoints()->GetPoint(ptId);

    p[0] = (float)dp[0]; p[1] = (float)dp[1]; p[2] = (float)dp[2];

    countYou[processId]++;
    }

  // 5. Do pairwise exchanges of the points we want global IDs for

  for (processId = 0; processId < nprocs; processId++)
    {
    arrayLength[processId] = countYou[processId] * 3 * sizeof(float);
    ptarrays[processId] = (char *)ptarrayOut[processId];
    }

  tag = 0x0013;

  rc = this->PairWiseDataExchange(arrayLength, ptarrays, tag);

  int *countMe = new int [nprocs];
  float **ptarrayIn = new float * [nprocs];

  for (processId = 0; processId < nprocs; processId++)
    {
    countMe[processId] = arrayLength[processId] / (3 * sizeof(float));
    ptarrayIn[processId] = (float *)ptarrays[processId];

    if (ptarrayOut[processId])
      {
      delete [] ptarrayOut[processId];
      }
    }

  delete [] ptarrayOut;

  // 6. Find the global point IDs that have been requested of me

  int **idarrayOut = new int * [nprocs];
  memset(idarrayOut, 0, sizeof(int *) * nprocs);

  this->FindGlobalPointIds(idarrayOut, countMe, ptarrayIn, globalIds, grid);

  for (processId = 0; processId < nprocs; processId++)
    {
    if (ptarrays[processId]) delete [] ptarrays[processId];
    }

  delete [] ptarrayIn;

  // 7. Do pairwise exchanges of the global point IDs

  for (processId = 0; processId < nprocs; processId++)
    {
    arrayLength[processId] = countMe[processId] * sizeof(int);
    ptarrays[processId] = (char *)idarrayOut[processId];
    }

  tag = 0x0017;

  rc = this->PairWiseDataExchange(arrayLength, ptarrays, tag);

  int **idarrayIn = new int * [nprocs];
  
  for (processId = 0; processId < nprocs; processId++)
    {
    idarrayIn[processId] = (int *)ptarrays[processId];

    if (idarrayOut[processId])
      {
      delete [] idarrayOut[processId];
      }
    }

  delete [] idarrayOut;
  delete [] countMe;

  delete [] arrayLength;

  // 8. Update my ugrid with these mutually agreed upon global point IDs

  for (processId = 0; processId < nprocs; processId++)
    {
    if (countYou[processId] > 0)
      {
      for (ptId = 0; ptId < countYou[processId]; ptId++)
        {
        int myLogicalId = localIds[processId][ptId];

        globalIds->SetValue(myLogicalId, idarrayIn[processId][ptId]);
        }
      delete [] localIds[processId];
      delete [] ptarrays[processId];
      }
    }

  delete [] ptarrays;
  delete [] localIds;
  delete [] idarrayIn;
  delete [] countYou;

  // 9. Add global node Id array to the grid

  if (this->GlobalIdArrayName)
    {
    globalIds->SetName(this->GlobalIdArrayName);
    }
  else
    {
    globalIds->SetName(this->TemporaryGlobalNodeIds);
    }

  grid->GetPointData()->AddArray(globalIds);

  globalIds->Delete();
}
void vtkDistributedDataFilter::FindGlobalPointIds(int **idarray, 
                     int *count, float **ptarray, vtkIntArray *ids,
                     vtkUnstructuredGrid *grid)
{
  vtkKdTree *kd = vtkKdTree::New();

  kd->BuildLocatorFromPoints(grid->GetPoints());

  int procId;
  int ptId, localId;

  int nprocs = this->NumProcesses;

  for (procId = 0; procId < nprocs; procId++)
    {
    float *pt = ptarray[procId];
    int npoints = count[procId];

    idarray[procId] = new int [npoints];

    for (ptId = 0; ptId < npoints; ptId++)
      {
      localId = kd->FindPoint(pt);

      if (localId < 0)
        {
        float d2;
        localId = kd->FindClosestPoint(pt, d2);
        }

      idarray[procId][ptId] = ids->GetValue(localId);  // global Id
      pt += 3;
      }
    }

  kd->Delete();
}
vtkIntArray *vtkDistributedDataFilter::AssignGlobalCellIds()
{
  int proc;
  int reg;

  vtkPKdTree *kd = this->Kdtree;

  int *numCells = new int [this->NumProcesses];

  for (proc=0; proc < this->NumProcesses; proc++)
    {
    vtkIntArray *regionList = vtkIntArray::New();

    kd->GetRegionListForProcess(proc, regionList);

    vtkIdType numRegions = regionList->GetNumberOfTuples();

    numCells[proc] = 0;

    if (numRegions > 0)
      {
      int *counts = new int [numRegions];
      kd->GetRegionsCellCountForProcess(proc, counts, numRegions);
      for (reg=0; reg<numRegions; reg++)
        {
        numCells[proc] += counts[reg];
        } 
      delete [] counts;
      }
    regionList->Delete();
    }

  int myNumCells = numCells[this->MyId];

  vtkIntArray *globalCellIds = vtkIntArray::New();
  globalCellIds->SetNumberOfValues(myNumCells);

  int StartId = 0;

  for (proc=0; proc < this->MyId; proc++)
    {
    StartId += numCells[proc];
    } 

  for (int i=0; i<myNumCells; i++)
    {
    globalCellIds->SetValue(i, StartId++);
    }

  globalCellIds->SetName(vtkDistributedDataFilter::TemporaryGlobalCellIds);

  delete [] numCells;

  return globalCellIds;
}

//-------------------------------------------------------------------------
// Code related to acquiring ghost cells
//-------------------------------------------------------------------------

int vtkDistributedDataFilter::InMySpatialRegion(float x, float y, float z)
{
  return this->InMySpatialRegion((double)x, (double)y, (double)z);
}
int vtkDistributedDataFilter::InMySpatialRegion(double x, double y, double z)
{
  double *box = this->ConvexSubRegionBounds;

  if (!box) return 0;

  // To avoid ambiguity, a point on a boundary is assigned to 
  // the region for which it is on the upper boundary.  Or
  // (in one dimension) the region between points A and B
  // contains all points p such that A < p <= B.

  if ( (x <= box[0]) || (x > box[1]) ||
       (y <= box[2]) || (y > box[3]) ||
       (z <= box[4]) || (z > box[5])   )
    {
      return 0;
    }

  return 1;
}
int vtkDistributedDataFilter::StrictlyInsideMyBounds(float x, float y, float z)
{
  return this->StrictlyInsideMyBounds((double)x, (double)y, (double)z);
}
int vtkDistributedDataFilter::StrictlyInsideMyBounds(double x, double y, double z)
{
  double *box = this->ConvexSubRegionBounds;

  if (!box) return 0;

  if ( (x <= box[0]) || (x >= box[1]) ||
       (y <= box[2]) || (y >= box[3]) ||
       (z <= box[4]) || (z >= box[5])   )
    {
      return 0;
    }

  return 1;
}
vtkUnstructuredGrid *
  vtkDistributedDataFilter::AddGhostCells(vtkUnstructuredGrid *myGrid)
{
  // Obtain ghost cells from other processes.  We divide into two
  // cases:
  //
  // IncludeAllIntersectingCells == 1
  //   In this case, each process has all the cells which intersect
  //   it's spatial region.  These are all the ghost level 0 cells.
  //   Cells that straddle a region boundary are ghost level 0
  //   cells for more than one process.  We begin by obtaining level
  //   1 ghost cells for points we have that are outside our spatial 
  //   region, by querying the processes in which those points lie.
  //
  // IncludeAllIntersectingCells == 0
  //   In this case, each process should only have the cells that 
  //   have their centroid falling within the process' assigned
  //   spatial region.  These are the process' ghost level 0 cells.
  //   Each cell is a ghost level 0 cell for only one process.
  //   But in fact, to speed the computation of ghost cells in this
  //   case, we redistributed ugrids so that we have all the cells that
  //   intersect the process' spatial region.  The vtkGhostLevel
  //   cell array indicates which of the cells are the level 0
  //   cells.  We begin by finding the other cells in our grid
  //   which are our level 1 ghost cells.  Any cell in the grid
  //   that is not level 1 or level 0 is discarded.  Then we
  //   procede as in the first case, querying other processes for
  //   the rest of our level 1 ghost cells.
  //
  // In both cases:
  // After obtaining ghost level 1 cells, query other processes for
  // all cells using our ghost level 1 points, and so on.

  if (this->IncludeAllIntersectingCells == 0)
    {
    // For every ghost level 0 cell, set it's points to level 0,
    // and find all cells using each point, and if not level 0
    // set them to level 1.  Remove any cells that were not set
    // to level 0 or level 1.

    TIMER("Fix received ghost levels");
    this->FixGhostLevels(myGrid);
    TIMERDONE("Fix received ghost levels");
    }

  // Create a search structure mapping global point IDs to local point IDs

  TIMER("Build id search structure");

  this->GlobalNodeIdAccessStart(myGrid);

  vtkstd::map<int, int> globalToLocalMap;
  vtkIdType numPoints = myGrid->GetNumberOfPoints();

  for (int localPtId = 0; localPtId < numPoints; localPtId++)
    {
    int gid = this->GlobalNodeIdAccessGetId(localPtId);

    globalToLocalMap.insert(vtkstd::pair<int, int>(gid, localPtId));
    }

  TIMERDONE("Build id search structure");

  // All new cells will go into a separate grid for now, since merging will
  // be much faster.

  // GHOST LEVEL 1 CELLS - these are the neighbors of all the
  // ghost level 0 points we have that are outside of our assigned
  // spatial region

  int getGhostLevel = 1;

  // the grid containing the points we need ghost cells for, and the
  // first point in it's point array that we should check

  vtkUnstructuredGrid *ghostPointsFromGrid = myGrid;
  int firstPotentialGhostPoint = 0;
  vtkIdType prevNumGhostPoints;

  // the grid containing cells we will send to other processes upon request

  vtkUnstructuredGrid *ghostCellsFromGrid = myGrid;

  // the grid to which we will write the ghost cells we receive from other
  // processes

  vtkUnstructuredGrid *ghostCellsToGrid = NULL;

  ghostCellsToGrid = this->AddGhostLevel(ghostCellsFromGrid, 
                      ghostPointsFromGrid, firstPotentialGhostPoint, 
                      ghostCellsToGrid,
                      &globalToLocalMap, getGhostLevel);

  if (this->GhostLevel > 1)
    {
    // GHOST LEVEL 2 CELLS
    //  If IncludeAllIntersectingCells ON: All the level 2 ghost cells
    //  are found by getting neighbors to all the level 1 points in the
    //  ghost cell grid just created.
    //  If IncludeAllIntersectingCells OFF: In addition, we have level 1
    //  points in our original grid, that we need to get ghost cells for.

    getGhostLevel = 2;
    ghostPointsFromGrid = ghostCellsToGrid;
    firstPotentialGhostPoint = 0;

    prevNumGhostPoints = ghostCellsToGrid->GetNumberOfPoints();

    ghostCellsToGrid = this->AddGhostLevel(ghostCellsFromGrid, 
                        ghostPointsFromGrid, firstPotentialGhostPoint, 
                        ghostCellsToGrid,
                        &globalToLocalMap, getGhostLevel);

    if (this->IncludeAllIntersectingCells == 0)
      {
      ghostPointsFromGrid = myGrid;
      firstPotentialGhostPoint = 0;

      ghostCellsToGrid = this->AddGhostLevel(ghostCellsFromGrid, 
                          ghostPointsFromGrid, firstPotentialGhostPoint, 
                          ghostCellsToGrid,
                          &globalToLocalMap, getGhostLevel);
      }

    // GHOST LEVEL 3 AND HIGHER

    while (++getGhostLevel <= this->GhostLevel)
      {
      ghostPointsFromGrid = ghostCellsToGrid;
      firstPotentialGhostPoint = prevNumGhostPoints;
      prevNumGhostPoints = ghostCellsToGrid->GetNumberOfPoints();

      ghostCellsToGrid = this->AddGhostLevel(ghostCellsFromGrid, 
                          ghostPointsFromGrid, firstPotentialGhostPoint, 
                          ghostCellsToGrid,
                          &globalToLocalMap, getGhostLevel);

      }
    }

  globalToLocalMap.clear();

  vtkUnstructuredGrid *gridPlus = NULL;

  if (ghostCellsToGrid && (ghostCellsToGrid->GetNumberOfCells() > 0))
    {
    gridPlus = vtkUnstructuredGrid::New();

    vtkMergeCells *mc = vtkMergeCells::New();
    mc->SetUnstructuredGrid(gridPlus);
    mc->SetTotalNumberOfCells(myGrid->GetNumberOfCells() + 
                         ghostCellsToGrid->GetNumberOfCells());
    mc->SetTotalNumberOfPoints(myGrid->GetNumberOfPoints() + 
                          ghostCellsToGrid->GetNumberOfPoints());
    mc->SetTotalNumberOfDataSets(2);
    mc->SetGlobalIdArrayName(this->GlobalIdArrayName ? this->GlobalIdArrayName : 
                             vtkDistributedDataFilter::TemporaryGlobalNodeIds);

    mc->MergeDataSet(myGrid);
    mc->MergeDataSet(ghostCellsToGrid);

    myGrid->Delete();
    ghostCellsToGrid->Delete();
    mc->Delete();
    }
  else
    {
    gridPlus = myGrid;
    }

  return gridPlus;
}
vtkUnstructuredGrid *vtkDistributedDataFilter::AddGhostLevel(
     vtkUnstructuredGrid *myGrid,
     vtkUnstructuredGrid *ghostPointGrid, int firstGhostPoint,
     vtkUnstructuredGrid *ghostCellGrid, 
     vtkstd::map<int, int> *globalToLocalMap,
     int ghostLevel)
{
  TIMER("Get my ghost points");
  vtkIntArray **myPtIds = this->GetGhostPoints(ghostLevel, ghostPointGrid, firstGhostPoint);
  TIMERDONE("Get my ghost points");

  TIMER("Exchange ghost points");
  vtkIntArray **yourPtIds = this->ExchangePointIds(myPtIds);
  TIMERDONE("Exchange ghost points");

  vtkDistributedDataFilter::FreeIdArrays(myPtIds, this->NumProcesses);

  TIMER("Build ghost grids");
  vtkUnstructuredGrid **subGrids =
    this->BuildRequestedGrids(yourPtIds, myGrid, globalToLocalMap);
  TIMERDONE("Build ghost grids");

  vtkDistributedDataFilter::FreeIdArrays(yourPtIds, this->NumProcesses);

  TIMER("Exchange ghost grids");
  vtkUnstructuredGrid *newGhostCellGrid =
    this->ExchangeMergeSubGrids(subGrids, ghostCellGrid, ghostLevel,
                                globalToLocalMap);
  TIMERDONE("Exchange ghost grids");

  return newGhostCellGrid;
}
void vtkDistributedDataFilter::FreeIdArrays(vtkIntArray **idArray, int len)
{
  if (!idArray) return;

  for (int i=0; i<len ; i++)
    {
    if (idArray[i])
      {
      idArray[i]->Delete();
      idArray[i] = NULL;
      }
    }

  delete [] idArray;
}

// Find all points for which I need ghost cells.  Create a list of
// each global point ID, followed by the number of cells I already
// have for that point, followed by the global IDs of those cells.
// There will be one list for each process I am requesting ghost
// cells from.  

vtkIntArray **vtkDistributedDataFilter::GetGhostPoints(int ghostLevel,
                          vtkUnstructuredGrid *grid, int firstPoint) 
{
  int ptId, cellId, proc;
  int nprocs = this->NumProcesses;
  int processId[8], regionId;

  vtkIntArray **ghostPts = new vtkIntArray * [nprocs];
  memset(ghostPts, 0, sizeof(vtkIntArray *) * nprocs);

  int level = (unsigned char)(ghostLevel - 1);
  vtkIdType npoints = grid->GetNumberOfPoints();

  vtkDataArray *da = grid->GetPointData()->GetArray("vtkGhostLevels");
  vtkUnsignedCharArray *pointGLarray = vtkUnsignedCharArray::SafeDownCast(da);
  unsigned char *pointGL = pointGLarray->GetPointer(0);

  da = grid->GetCellData()->GetArray(
          vtkDistributedDataFilter::TemporaryGlobalCellIds);
  vtkIntArray *globalCellIds = vtkIntArray::SafeDownCast(da);
  int *gidsCell = globalCellIds->GetPointer(0);

  this->GlobalNodeIdAccessStart(grid);

  // We don't know the data type of the points.  We assume the most common
  // case is that points are floats, and optimize for that case.

  int ptArrayType = grid->GetPoints()->GetDataType();
  float *gridPointArray = NULL;
  float gridPoint[3];
  float *pt;

  if (ptArrayType == VTK_FLOAT)
    {
    vtkFloatArray *fa = vtkFloatArray::SafeDownCast(grid->GetPoints()->GetData());
    gridPointArray = fa->GetPointer(firstPoint * 3);

    pt = gridPointArray;
    }
  else
    {
    pt = gridPoint;
    }

  vtkIdList *localCellIds = vtkIdList::New();

  double *bdry = this->ConvexSubRegionBounds;

  for (ptId=firstPoint; ptId < npoints; ptId++)
    {
    if (ptArrayType == VTK_FLOAT)
      {
      if (ptId > firstPoint)
        {
        pt += 3;
        }
      }
    else
      {
      double *dpt = grid->GetPoints()->GetPoint(ptId);
      gridPoint[0] = (float)dpt[0];
      gridPoint[1] = (float)dpt[1];
      gridPoint[2] = (float)dpt[2];
      }

    // GhostLevel one is ugly.  If the point is strictly within my
    // assigned spatial region, then I have all cells that contain that
    // point, and I can skip it.  If it's strictly outside my spatial
    // region, I can find the process owning that region and query it
    // for ghost cells. If the point is exactly on the boundary of a
    // k-d tree region, and it's one of my lower boundaries, that point
    // technically belongs to another process and I query that process.
    // However if the point is on one of my upper boundaries, it's
    // technically my region, so no sense in querying myself.  But I
    // need to query every process that shares that boundary for
    // cells that use the point.

    // For ghost levels greater than one, we simply look for all our
    // points that have ghost level one less, and query the processes
    // that own the regions they are in.

    if (((ghostLevel == 1) && this->StrictlyInsideMyBounds(pt[0], pt[1], pt[2])) 
        ||
        (pointGL[ptId] != level) )
      {
      // I already have all cells using this point
  
      continue;
      }
    
    int numProcesses = 1;    // almost always
    
    if ((ghostLevel == 1) &&
        ((pt[0] == bdry[1]) || (pt[1] == bdry[3]) || (pt[2] == bdry[5])))
      {

      // point is on my upper boundary

      vtkIntArray *plist = vtkIntArray::New();
       
      this->Kdtree->GetAllProcessesBorderingOnPoint(pt[0], pt[1], pt[2],
                                                    plist);
       
      numProcesses = 0;
      for (proc = 0; proc < plist->GetNumberOfTuples(); proc++)
        {  
        if (plist->GetValue(proc) != this->MyId)
          {
          processId[numProcesses++] = plist->GetValue(proc);
          }
        }
      }
    else
      {
      // point is on my lower boundary or outside my region

      regionId = this->Kdtree->GetRegionContainingPoint(pt[0], pt[1], pt[2]);

      processId[0] = this->Kdtree->GetProcessAssignedToRegion(regionId);
      }

    if (numProcesses == 0) continue;

    for (proc = 0; proc < numProcesses; proc++)
      {
      if (ghostPts[processId[proc]] == NULL)
        {
        ghostPts[processId[proc]] = vtkIntArray::New();
        ghostPts[processId[proc]]->SetNumberOfComponents(1);
        }

      ghostPts[processId[proc]]->
        InsertNextValue(this->GlobalNodeIdAccessGetId(ptId));
      }

    // Tell remote processes which cells I already have, so they will
    // refrain from sending me those.  This is faster than filtering out
    // duplicate cells after the pair-wise exchange of ghost cells.

    grid->GetPointCells(ptId, localCellIds);

    vtkIdType ncells = localCellIds->GetNumberOfIds();
    vtkIdType *lidsCell= localCellIds->GetPointer(0);


    for (proc = 0; proc < numProcesses; proc++)
      {
      ghostPts[processId[proc]]->InsertNextValue(ncells);

      for (cellId = 0; cellId < ncells; cellId++)
        {
        ghostPts[processId[proc]]->InsertNextValue(gidsCell[lidsCell[cellId]]);
        }
      }
    }

  localCellIds->Delete();

  return ghostPts;

}
// Send to other processes the point IDs that I need ghost cells for.  
// Return the lists of point IDs that other processes sent me.

vtkIntArray **
  vtkDistributedDataFilter::ExchangePointIds(vtkIntArray **myPtIds)
{
  int proc;
  int nprocs = this->NumProcesses;
  int elementSize = sizeof(int);

  char **idArrays = new char * [nprocs];
  int *sizes = new int [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    vtkIdType nvals = myPtIds[proc] ? myPtIds[proc]->GetNumberOfTuples() : 0;

    if (nvals > 0)
      {
      idArrays[proc] = (char *)(myPtIds[proc]->GetPointer(0));
      }
    else
      {
      idArrays[proc] = NULL;
      }

    sizes[proc] = nvals * elementSize;
    }

  int tag = 0x0037;

  int rc = this->PairWiseDataExchange(sizes, idArrays, tag);

  if (rc)
    {
    delete [] idArrays;
    delete [] sizes;
    vtkErrorMacro(<< "ExchangePointIds error");
    return NULL;
    }

  vtkIntArray **remoteArrays = new vtkIntArray * [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    if (sizes[proc] == 0)
      {
      remoteArrays[proc] = NULL;
      }
    else
      {
      remoteArrays[proc] = vtkIntArray::New();
      int numIds = sizes[proc] / elementSize;

      remoteArrays[proc]->SetArray((int *)idArrays[proc], numIds, 0);
      }
    }

  delete [] idArrays;
  delete [] sizes;
  
  return remoteArrays;
}

// For every process that sent me a list of point IDs, create a sub grid
// containing all the cells I have containing those points, omitting cells
// the remote process already has.

vtkUnstructuredGrid **vtkDistributedDataFilter::BuildRequestedGrids(
                        vtkIntArray **globalPtIds, 
                        vtkUnstructuredGrid *grid, 
                        vtkstd::map<int, int> *ptIdMap)
{
  int id, proc;
  int nprocs = this->NumProcesses;
  vtkIdType cellId;
  vtkIdType nelts;

  // for each process, create a list of the ids of cells I need
  // to send to it

  vtkstd::map<int, int>::iterator imap;

  vtkIdList *cellList = vtkIdList::New();

  vtkIdList **procCellList = new vtkIdList* [nprocs];
  memset(procCellList, 0, sizeof(vtkIdList *) * nprocs);

  vtkDataArray *da = grid->GetCellData()->GetArray(
                vtkDistributedDataFilter::TemporaryGlobalCellIds);

  vtkIntArray *gidCells = vtkIntArray::SafeDownCast(da);

  for (proc = 0; proc < nprocs; proc++)
    {
    if (globalPtIds[proc] == NULL)
      {
      continue;
      }

    if ((nelts = globalPtIds[proc]->GetNumberOfTuples()) == 0)
      {
      continue;
      }
      
    int *ptarray = globalPtIds[proc]->GetPointer(0);

    int nYourCells = 0;

    for (id = 0; id < nelts; id += (nYourCells + 2))
      {
      int ptId = ptarray[id];
      nYourCells = ptarray[id+1];

      imap = ptIdMap->find(ptId);

      if (imap == ptIdMap->end())
        {
        // error - I don't have this point
        vtkErrorMacro(<< 
          "vtkDistributedDataFilter::BuildRequestedGrids - invalid request");

        cellList->Delete();
        for (proc = 0; proc < nprocs; proc++)
          {
          if (procCellList[proc]) procCellList[proc]->Delete();
          }
        delete [] procCellList;
        return NULL;
        }

      vtkIdType myPtId = (vtkIdType)imap->second;   // convert to my local point Id

      grid->GetPointCells(myPtId, cellList);

      vtkIdType nMyCells = cellList->GetNumberOfIds();

      if (nMyCells == 0)
        {
        continue;
        }

      if (nYourCells > 0)
        {
        // We don't send cells the remote process tells us it already
        // has.  This is much faster than removing duplicate cells on
        // the receive side.

        int *remoteCells = ptarray + id + 2;
        this->RemoveRemoteCellsFromList(cellList, gidCells, remoteCells, nYourCells);
        }

      vtkIdType nSendCells = cellList->GetNumberOfIds();

      if (nSendCells == 0)
        {
        continue;
        }

      if (procCellList[proc] == NULL)
        {
        procCellList[proc] = vtkIdList::New();
        }

      for (cellId = 0; cellId < nSendCells; cellId++)
        {
        // InsertUniqueId is very slow.  In practice this should
        // be a short list, right?

        procCellList[proc]->InsertUniqueId(cellList->GetId(cellId));
        }
      }
    }

  cellList->Delete();

  // Create a subgrid for each process that requires cells from me

  vtkUnstructuredGrid **subGrids = new vtkUnstructuredGrid * [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    if (procCellList[proc] == NULL)
      {
      subGrids[proc] = NULL;
      continue;
      }

    vtkExtractCells *ec = vtkExtractCells::New();
    ec->SetInput(grid);
    ec->SetCellList(procCellList[proc]);
    ec->Update();

    subGrids[proc] = ec->GetOutput();
    subGrids[proc]->Register(this);
    ec->Delete();

    procCellList[proc]->Delete();
    }

  delete [] procCellList;

  return subGrids;
}
void vtkDistributedDataFilter::RemoveRemoteCellsFromList(
        vtkIdList *cellList, vtkIntArray *gidCells, int *remoteCells, int nRemoteCells)
{
  vtkIdType id, nextId;
  int id2;
  int nLocalCells = cellList->GetNumberOfIds();

  // both lists should be very small, so we just do an n^2 lookup

  for (id = 0, nextId = 0; id < nLocalCells; id++)
    {
    vtkIdType localCellId  = cellList->GetId(id);
    int globalCellId = gidCells->GetValue(localCellId);

    int found = 0;

    for (id2 = 0; id2 < nRemoteCells; id2++)
      {
      if (remoteCells[id2] == globalCellId)
        {
        found = 1;
        break;
        } 
      }

    if (!found)
      {
      cellList->SetId(nextId++, localCellId);
      }
    }

  cellList->SetNumberOfIds(nextId);
}

// Send the ghost cells I gathered for other processes to them, and
// receive the ghost cells they gathered for me.  Set the ghost levels
// for the points and cells in the received cells.  Merge the new ghost
// cells into the supplied grid, and return the new grid.  Delete
// all the grids that were passed in as arguments.

vtkUnstructuredGrid *vtkDistributedDataFilter::ExchangeMergeSubGrids(
          vtkUnstructuredGrid **grids, vtkUnstructuredGrid *ghostCellGrid,
          int ghostLevel, vtkstd::map<int, int> *idMap)
{
  int proc, i;
  int nprocs = this->NumProcesses;

  char **gridArrays = new char * [nprocs];
  int *sizes = new int [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    if (grids[proc])
      {
      gridArrays[proc] = this->MarshallDataSet(grids[proc], sizes[proc]);
      grids[proc]->Delete();
      grids[proc] = NULL;
      }
    else
      {
      gridArrays[proc] = NULL;
      sizes[proc] = 0;
      }
    }

  int tag = 0x0057;

  int rc = this->PairWiseDataExchange(sizes, gridArrays, tag);

  if (rc)
    {
    delete [] gridArrays;
    delete [] sizes;
    ghostCellGrid->Delete();
    vtkErrorMacro(<< "vtkDistributedDataFilter::ExchangeMergeSubGrids error");
    return NULL;
    }

  int numSources = 0;
  vtkIdType numPoints = 0;
  vtkIdType numCells = 0;

  const char *nodeIdArrayName  = NULL;

  for (proc = 0; proc < nprocs; proc++)
    {
    if (sizes[proc] > 0)
      {
      grids[proc] = this->UnMarshallDataSet(gridArrays[proc], sizes[proc]);

      delete [] gridArrays[proc];

      numSources++;
      numPoints += grids[proc]->GetNumberOfPoints();
      numCells += grids[proc]->GetNumberOfCells();

      if (nodeIdArrayName == NULL)
        {
        nodeIdArrayName  = this->GetGlobalNodeIdArray(grids[proc]);
        }
      }
    else
      {
      grids[proc] = NULL;
      }
    }

  delete [] gridArrays;

  // Set the ghost level of all new cells, and set the ghost level of all
  // the points.  We know some points in the new grids actually have ghost
  // level one lower, because they were on the boundary of the previous
  // grid.  This is OK if ghostLevel is > 1.  When we merge, vtkMergeCells 
  // will skip these points because they are already in the previous grid.
  // But if ghostLevel is 1, those boundary points were in our original
  // grid, and we need to use the global ID map to determine if the
  // point ghost levels should be set to 0.  We'll fix these after the
  // merge, since there may be fewer points then and it will go quicker.

  for (proc = 0; proc < nprocs; proc++)
    {
    if (sizes[proc] > 0)
      {
      vtkDataArray *da = grids[proc]->GetCellData()->GetArray("vtkGhostLevels");
      vtkUnsignedCharArray *cellGL = vtkUnsignedCharArray::SafeDownCast(da);
      
      da  = grids[proc]->GetPointData()->GetArray("vtkGhostLevels");
      vtkUnsignedCharArray *ptGL = vtkUnsignedCharArray::SafeDownCast(da);

      unsigned char *ia = cellGL->GetPointer(0);

      for (i=0; i < grids[proc]->GetNumberOfCells(); i++)
        {
        ia[i] = (unsigned char)ghostLevel;
        }

      ia = ptGL->GetPointer(0);

      for (i=0; i < grids[proc]->GetNumberOfPoints(); i++)
        {
        ia[i] = (unsigned char)ghostLevel;
        }
      }
    }

  // now merge

  vtkUnstructuredGrid *returnGrid = NULL;

  char mergeToGrid = ghostCellGrid && (ghostCellGrid->GetNumberOfCells() > 0);

  if (!mergeToGrid && ghostCellGrid) ghostCellGrid->Delete();

  if ((numSources == 1) && !mergeToGrid)
    {
    for (proc = 0; proc < nprocs; proc++)
      {
      if (sizes[proc] > 0)
        {
        returnGrid = grids[proc];
        break;    
        }
      }
    }
  else if ((numSources == 0) && mergeToGrid)
    {
    returnGrid = ghostCellGrid;
    }
  else if ( ((numSources > 0) && mergeToGrid) || (numSources > 1))
    {
    returnGrid = vtkUnstructuredGrid::New();
    vtkMergeCells *mc = vtkMergeCells::New();
    mc->SetUnstructuredGrid(returnGrid);

    if (mergeToGrid)
      {
      numSources++;
      numPoints += ghostCellGrid->GetNumberOfPoints();
      numCells += ghostCellGrid->GetNumberOfCells();
      }

    mc->SetTotalNumberOfDataSets(numSources);
    mc->SetTotalNumberOfPoints(numPoints);
    mc->SetTotalNumberOfCells(numCells);

    mc->SetGlobalIdArrayName(nodeIdArrayName);

    if (this->IncludeAllIntersectingCells)
      {
      // It's possible that different remote processes will send me the
      // same cell.  I'll ask vtkMergeCells to filter these out.

      mc->SetGlobalCellIdArrayName(vtkDistributedDataFilter::TemporaryGlobalCellIds);
      }

    if (mergeToGrid)
      {
      mc->MergeDataSet(ghostCellGrid);
      ghostCellGrid->Delete();
      }

    for (proc=0; proc < nprocs; proc++)
      {
      if (sizes[proc] > 0)
        {
        mc->MergeDataSet(grids[proc]);
        grids[proc]->Delete();
        }
      }
    mc->Finish();
    mc->Delete(); 
    }

  // If this is ghost level 1, mark any points from our original grid
  // as ghost level 0.

  if ((ghostLevel == 1) && returnGrid && (returnGrid->GetNumberOfCells() > 0))
    {
    vtkDataArray *da = returnGrid->GetPointData()->GetArray("vtkGhostLevels");
    vtkUnsignedCharArray *ptGL = vtkUnsignedCharArray::SafeDownCast(da);

    this->GlobalNodeIdAccessStart(returnGrid);
    int npoints = returnGrid->GetNumberOfPoints();

    vtkstd::map<int, int>::iterator imap;

    for (i=0; i < npoints; i++)
      {
      int globalId = this->GlobalNodeIdAccessGetId(i);

      imap = idMap->find(globalId);

      if (imap != idMap->end())
        {
        ptGL->SetValue(i,0);   // found among my ghost level 0 cells
        }
      }
    }
  
  delete [] sizes;
  delete [] grids;

  return returnGrid;
}
void vtkDistributedDataFilter::FixGhostLevels(vtkUnstructuredGrid *ugrid)
{
  vtkIdType pidx1, pidx2, cidx;
  vtkIdType pointId1, pointId2, cellId1, cellId2;
  vtkIdType *pts1, *pts2;
  vtkIdType numPts1, numPts2;
  unsigned char cellGL1, cellGL2, pointGL1, pointGL2;
  int where1, where2;
  
  vtkDataArray *da = ugrid->GetPointData()->GetArray("vtkGhostLevels");
  vtkUnsignedCharArray *ptLevels  = vtkUnsignedCharArray::SafeDownCast(da);

  da = ugrid->GetCellData()->GetArray("vtkGhostLevels");
  vtkUnsignedCharArray *cellLevels  = vtkUnsignedCharArray::SafeDownCast(da);

  // For every ghost level 0 cell, set it's points to level 0,
  // and find all cells using each point, and if not level 0
  // set them to level 1.

  vtkCellArray *cellArray = ugrid->GetCells();

  vtkIntArray *locs = ugrid->GetCellLocationsArray();
  vtkIdTypeArray *cells = cellArray->GetData();

  int numLeveledCells = 0;
  vtkIdList *ptCells = vtkIdList::New();

  vtkIdType numCellsTotal = ugrid->GetNumberOfCells();

  for (cellId1=0; cellId1 < numCellsTotal; cellId1++)
    {
    cellGL1 = cellLevels->GetValue(cellId1);
    if (cellGL1 != (unsigned char)0) continue;

    numLeveledCells++;

    where1 = locs->GetValue(cellId1);
    pts1 = cells->GetPointer(where1);

    numPts1 = pts1[0];
    pts1++;

    for (pidx1 = 0; pidx1 < numPts1; pidx1++)
      {
      pointId1 = pts1[pidx1];
      pointGL1 = ptLevels->GetValue(pointId1);
      if (pointGL1 == (unsigned char)0) continue;  // already processed this point

      ptLevels->SetValue(pointId1, 0);

      ugrid->GetPointCells(pointId1, ptCells);

      unsigned short numPtCells = ptCells->GetNumberOfIds();

      for (cidx = 0; cidx < numPtCells; cidx++)
        {
        cellId2 = ptCells->GetId(cidx);

        if (cellId2 == cellId1) continue;

        cellGL2 = cellLevels->GetValue(cellId2);

        if (cellGL2 == vtkDistributedDataFilter::UnsetGhostLevel)
          {
          cellLevels->SetValue(cellId2, 1);
          numLeveledCells++;
          
          where2 = locs->GetValue(cellId2);
          pts2 = cells->GetPointer(where2);
          numPts2 = pts2[0];
          pts2++;

          for (pidx2=0; pidx2 < numPts2; pidx2++)
            {
            pointId2 = pts2[pidx2];
            pointGL2 = ptLevels->GetValue(pointId2);

            if (pointGL2 == vtkDistributedDataFilter::UnsetGhostLevel)
              {
              ptLevels->SetValue(pointId2, 1);
              }
            }
          }
        }
      } 
    } 

  ptCells->Delete();

  if (numLeveledCells < numCellsTotal)
    {
    // My zero level cells are all cells with centroid in my
    // region.  My temporary ugrid has all cells that intersect
    // my region.  It could happen that some of those are
    // disconnected from my zero level cells,  So they are
    // not ghost level 1 cells, I need to remove them.

    vtkIdList *ids = vtkIdList::New();

    ids->SetNumberOfIds(numLeveledCells);
    vtkIdType nextId = 0;

    for (cellId1=0; cellId1 < numCellsTotal; cellId1++)
      {
      cellGL1 = cellLevels->GetValue(cellId1);

      if (cellGL1 != vtkDistributedDataFilter::UnsetGhostLevel)
        {
        ids->SetId(nextId++, cellId1);
        }
      }

    vtkExtractCells *ec = vtkExtractCells::New();
    ec->SetInput(ugrid);
    ec->SetCellList(ids);
    ec->Update();

    ugrid->ShallowCopy(ec->GetOutput());

    ec->Delete();
    }
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void vtkDistributedDataFilter::PrintTiming(ostream& os, vtkIndent indent)
{
  (void)indent;
  vtkTimerLog::DumpLogWithIndents(&os, (float)0.0);
}
void vtkDistributedDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{  
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Kdtree: " << this->Kdtree << endl;
  os << indent << "Controller: " << this->Controller << endl;
  if (this->GlobalIdArrayName)
    {
    os << indent << "GlobalIdArrayName: " << this->GlobalIdArrayName << endl;
    }
  os << indent << "RetainKdtree: " << this->RetainKdtree << endl;
  os << indent << "IncludeAllIntersectingCells: " << this->IncludeAllIntersectingCells << endl;
  os << indent << "ClipCells: " << this->ClipCells << endl;

  os << indent << "NumProcesses: " << this->NumProcesses << endl;
  os << indent << "MyId: " << this->MyId << endl;
  os << indent << "Timing: " << this->Timing << endl;
  os << indent << "TimerLog: " << this->TimerLog << endl;
}
