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

#include "vtkToolkits.h"
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
#include <vtkstd/set>
#include <vtkstd/map>

#ifdef VTK_USE_MPI
#include <vtkMPIController.h>
#endif

// Timing data ---------------------------------------------

#include <vtkTimerLog.h>

#define MSGSIZE 60

static char dots[MSGSIZE] = "...........................................................";
static char msg[MSGSIZE];

static char * makeEntry(const char *s)
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

vtkCxxRevisionMacro(vtkDistributedDataFilter, "1.23");

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

  if (this->GlobalIdArrayName && (!this->GlobalIdArrayName[0]))
    {
    delete [] this->GlobalIdArrayName;
    this->GlobalIdArrayName = NULL;
    }

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
void vtkDistributedDataFilter::AssignBoundaryCellsToOneRegionOn()
{
  this->SetAssignBoundaryCellsToOneRegion(1);
}
void vtkDistributedDataFilter::AssignBoundaryCellsToOneRegionOff()
{
  this->SetAssignBoundaryCellsToOneRegion(0);
}
void vtkDistributedDataFilter::SetAssignBoundaryCellsToOneRegion(int val)
{
  if (val)
    {
    this->IncludeAllIntersectingCells = 0;
    this->ClipCells = 0;
    }
}
void 
vtkDistributedDataFilter::AssignBoundaryCellsToAllIntersectingRegionsOn()
{
  this->SetAssignBoundaryCellsToAllIntersectingRegions(1);
}
void 
vtkDistributedDataFilter::AssignBoundaryCellsToAllIntersectingRegionsOff()
{
  this->SetAssignBoundaryCellsToAllIntersectingRegions(0);
}
void 
vtkDistributedDataFilter::SetAssignBoundaryCellsToAllIntersectingRegions(int val)
{
  if (val)
    {
    this->IncludeAllIntersectingCells = 1;
    this->ClipCells = 0;
    }
}
void vtkDistributedDataFilter::DivideBoundaryCellsOn()
{
  this->SetDivideBoundaryCells(1);
}
void vtkDistributedDataFilter::DivideBoundaryCellsOff()
{
  this->SetDivideBoundaryCells(0);
}
void vtkDistributedDataFilter::SetDivideBoundaryCells(int val)
{
  if (val)
    {
    this->IncludeAllIntersectingCells = 1;
    this->ClipCells = 1;
    }
}
//-------------------------------------------------------------------------
// Execute
//-------------------------------------------------------------------------

void vtkDistributedDataFilter::ComputeInputUpdateExtents( vtkDataObject *o)
{
  // Since this filter redistibutes data, ghost cells computed upstream
  // will not be valid.

  this->GetInput()->SetUpdateGhostLevel( 0);
}

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
  vtkUnstructuredGrid *output  = this->GetOutput();
  vtkDataSet* inputPlus = NULL;

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
    }

  this->Kdtree->SetController(this->Controller);
  this->Kdtree->SetTiming(this->Timing);
  this->Kdtree->SetNumRegionsOrMore(this->NumProcesses);
  this->Kdtree->SetMinCells(2);

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
    finalGrid = this->MPIRedistribute(inputPlus);   // faster
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

  TIMERDONE("Redistribute data among processors");

  if (inputPlus != input)
    {
    inputPlus->Delete();
    inputPlus = NULL;
    }

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

    // Create a search structure mapping global point IDs to local point IDs
  
    TIMER("Build id search structure");
  
    this->GlobalNodeIdAccessStart(finalGrid);
  
    vtkstd::map<int, int> globalToLocalMap;
    vtkIdType numPoints = finalGrid->GetNumberOfPoints();
  
    for (int localPtId = 0; localPtId < numPoints; localPtId++)
      {
      int gid = this->GlobalNodeIdAccessGetId(localPtId);
      globalToLocalMap.insert(vtkstd::pair<int, int>(gid, localPtId));
      }
  
    TIMERDONE("Build id search structure");

    TIMER("Add ghost cells");

    vtkUnstructuredGrid *expandedGrid= NULL;

    if (this->IncludeAllIntersectingCells)
      {
      expandedGrid = 
        this->AddGhostCellsDuplicateCellAssignment(finalGrid, &globalToLocalMap);
      }
    else
      {
      expandedGrid = 
        this->AddGhostCellsUniqueCellAssignment(finalGrid, &globalToLocalMap);
      }

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

  vtkDataSet* tmp = input->NewInstance();
  tmp->ShallowCopy(input);

  vtkUnstructuredGrid *clean = 
    vtkDistributedDataFilter::MergeGrids(1, &tmp,
        this->GetGlobalNodeIdArray(input), 0.0, NULL);

  output->ShallowCopy(clean);
  clean->Delete();

  if (this->GhostLevel > 0)
    {
    // Add the vtkGhostLevels arrays.  We have the whole
    // data set, so all cells are level 0.

    vtkDistributedDataFilter::AddConstantUnsignedCharPointArray(
                              output, "vtkGhostLevels", 0);
    vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(
                              output, "vtkGhostLevels", 0);
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

  vtkMPIController *mpiContr = 
    vtkMPIController::SafeDownCast(this->Controller);

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
      mpiContr->NoBlockReceive(myData[source], mySize[source], source, 
                               tag, req);
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
#else
  (void)yourSize;
  (void)tag;
  (void)yourData;
#endif

  return 0;
}
void vtkDistributedDataFilter::FreeFloatArrays(vtkFloatArray **array)
{
  if (!array) return;

  for (int i=0; i<this->NumProcesses; i++)
    {
    if (array[i]) array[i]->Delete();
    }
  delete [] array;
}
vtkFloatArray **
  vtkDistributedDataFilter::ExchangeFloatArrays(vtkFloatArray **myArray, int tag)
{
  int proc;
  int nprocs = this->NumProcesses;
  int elementSize = sizeof(float);

  char **arrays= new char * [nprocs];
  int *sizes = new int [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    vtkIdType nvals = myArray[proc] ? myArray[proc]->GetNumberOfTuples() : 0;

    if (nvals > 0)
      {
      arrays[proc] = (char *)(myArray[proc]->GetPointer(0));
      }
    else
      {
      arrays[proc] = NULL;
      }

    sizes[proc] = nvals * elementSize;
    }

  int rc = this->PairWiseDataExchange(sizes, arrays, tag);

  if (rc)
    {
    delete [] arrays;
    delete [] sizes;
    vtkErrorMacro(<< "ExchangeFloatArrays error");
    return NULL;
    }

  vtkFloatArray **remoteArrays = new vtkFloatArray * [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    if (sizes[proc] == 0)
      {
      remoteArrays[proc] = NULL;
      }
    else
      {
      remoteArrays[proc] = vtkFloatArray::New();
      int numIds = sizes[proc] / elementSize;

      remoteArrays[proc]->SetArray((float *)arrays[proc], numIds, 0);
      }
    }

  delete [] arrays;
  delete [] sizes;
  
  return remoteArrays;
}
void vtkDistributedDataFilter::FreeIntArrays(vtkIntArray **ar)
{
  if (!ar) return;

  for (int i=0; i<this->NumProcesses; i++)
    {
    if (ar[i]) ar[i]->Delete();
    }

  delete [] ar;
}
vtkIntArray **
  vtkDistributedDataFilter::ExchangeIntArrays(vtkIntArray **arIn, int tag)
{
  int proc;
  int nprocs = this->NumProcesses;
  int elementSize = sizeof(int);

  char **dataPtr= new char * [nprocs];
  int *sizes = new int [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    vtkIdType nvals = (arIn[proc] ? arIn[proc]->GetNumberOfTuples() : 0);

    if (nvals > 0)
      {
      dataPtr[proc] = (char *)(arIn[proc]->GetPointer(0));
      }
    else
      {
      dataPtr[proc] = NULL;
      }

    sizes[proc] = nvals * elementSize;
    }

  int rc = this->PairWiseDataExchange(sizes, dataPtr, tag);

  if (rc)
    {
    delete [] dataPtr;
    delete [] sizes;
    vtkErrorMacro(<< "ExchangeIntArrays error");
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

      remoteArrays[proc]->SetArray((int *)dataPtr[proc], numIds, 0);
      }
    }

  delete [] dataPtr;
  delete [] sizes;
  
  return remoteArrays;
}
int *vtkDistributedDataFilter::ExchangeCounts(int myCount, int tag)
{
  int i;
  int me = this->MyId;
  int nprocs = this->NumProcesses;

  int *sizes = new int [nprocs];
  char **dataPtr = new char * [nprocs];

  for (i=0; i<nprocs; i++)
    {
    if (i == me)
      {
      sizes[i] = 0;
      dataPtr[i] = NULL;
      }
    else
      {
      sizes[i] = sizeof(int);
      dataPtr[i] = (char *)&myCount;
      }
    }

  int rc = this->PairWiseDataExchange(sizes, dataPtr, tag);

  if (rc)
    {
    delete [] dataPtr;
    delete [] sizes;
    vtkErrorMacro(<< "ExchangeCounts error");
    return NULL;
    }

  int *allCounts = new int [nprocs];

  for (i=0; i<nprocs; i++)
    {
    if (i == me)
      {
      allCounts[i] = myCount;
      }
    else
      {
      allCounts[i] = *(int *)dataPtr[i];
      delete [] dataPtr[i];
      }
    }

  delete [] sizes;
  delete [] dataPtr;

  return allCounts;
}

vtkUnstructuredGrid *vtkDistributedDataFilter::MPIRedistribute(vtkDataSet *in)
{
  int proc;
  int me = this->MyId;
  int nnodes = this->NumProcesses;

  // Every process has been assigned a spatial region.  Create a
  // ugrid for every process, containing the cells I have read in
  // that are in their spatial region.

  TIMER("Create cell lists");
  
  if (this->IncludeAllIntersectingCells)
    {
    this->Kdtree->IncludeRegionBoundaryCellsOn();   // SLOW!!
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

  memcpy(mySizeData, yourSizeData, sizeof(int) * nnodes);
  memcpy(myPackedGrids, yourPackedGrids, sizeof(char *) * nnodes);

  int rc = this->PairWiseDataExchange(mySizeData, myPackedGrids, 0x1001);

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

  int TotalSets = 1;

  for (proc=0; proc<nnodes; proc++)
    {
    if (proc == me) continue;

    if (mySizeData[proc] > 0)
      {
      subGrid[proc] = 
        this->UnMarshallDataSet(myPackedGrids[proc], mySizeData[proc]);

      delete [] myPackedGrids[proc];
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

  vtkDataSet **grids = new vtkDataSet * [TotalSets];

  grids[0] = mySubGrid;
  int next = 1;

  for (proc=0; proc<nnodes; proc++)
    {
    if (subGrid[proc]) grids[next++] = subGrid[proc];
    }

  vtkUnstructuredGrid *newGrid = vtkDistributedDataFilter::MergeGrids(
         TotalSets, grids, this->GetGlobalNodeIdArray(mySubGrid),
         this->Kdtree->GetFudgeFactor(), NULL);

  delete [] grids;
  delete [] subGrid;

  TIMERDONE("Merge into new grid");

  vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(
                            newGrid, "vtkGhostLevels", 0);
  vtkDistributedDataFilter::AddConstantUnsignedCharPointArray(
                            newGrid, "vtkGhostLevels", 0);

  return newGrid;
}

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
  vtkIntArray *regions = vtkIntArray::New();

  int nregions = this->Kdtree->GetRegionAssignmentList(proc, regions);

  if (nregions == 0)
    {
    return NULL;
    }

  vtkDataSet* tmpInput = in->NewInstance();
  tmpInput->ShallowCopy(in);

  vtkExtractCells *extCells = vtkExtractCells::New();

  extCells->SetInput(tmpInput);

  for (int reg=0; reg < nregions; reg++)
    {
    extCells->AddCellList(this->Kdtree->GetCellList(regions->GetValue(reg)));

    if (this->IncludeAllIntersectingCells)
      {
      extCells->
        AddCellList(this->Kdtree->GetBoundaryCellList(regions->GetValue(reg)));
      }
    }

  regions->Delete();

  extCells->Update();

  // If this process has no cells for these regions, a ugrid gets
  // created anyway with field array information

  vtkUnstructuredGrid *ugrid = extCells->GetOutput();

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
  int myLocalRank=0;

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

  int fanInTo=0;
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

    vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(outside, arrayName, 0);
    vtkDistributedDataFilter::AddConstantUnsignedCharCellArray(inside, arrayName, 1);

    // Combine inside and outside into a single ugrid.

    TIMER("Merge inside and outside");

    vtkDataSet *grids[2];
    grids[0] = inside;
    grids[1] = outside;

    vtkUnstructuredGrid *combined = 
      vtkDistributedDataFilter::MergeGrids(2, grids, NULL,
                         this->Kdtree->GetFudgeFactor(), NULL);

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

  int *numPointsOutside = new int [nprocs];
  memset(numPointsOutside, 0, sizeof(int) * nprocs);

  vtkIntArray *globalIds = vtkIntArray::New();
  globalIds->SetNumberOfValues(nGridPoints);

  // 1. Count the points in grid which lie within my assigned spatial region

  int myNumPointsInside = 0;

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    double *pt = grid->GetPoints()->GetPoint(ptId);

    if (this->InMySpatialRegion(pt[0], pt[1], pt[2]))
      {
      globalIds->SetValue(ptId, 0);  // flag it as mine
      myNumPointsInside++;
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

  int *numPointsInside = this->ExchangeCounts(myNumPointsInside, 0x1003);

  // 3. Assign global Ids to the points inside my spatial region

  int firstId = 0;
  int numGlobalIds = 0;

  for (processId = 0; processId < nprocs; processId++)
    {
    if (processId < this->MyId) firstId += numPointsInside[processId];
    numGlobalIds += numPointsInside[processId];
    }

  delete [] numPointsInside;

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    if (globalIds->GetValue(ptId) == 0)
      {
      globalIds->SetValue(ptId, firstId++);
      }
    }

  int myNumPointsUnassigned = nGridPoints - myNumPointsInside;

  // -----------------------------------------------------------------
  // All processes have assigned global IDs to the points within their
  // assigned spatial region.  Now they have to get the IDs for the
  // points in their grid which lie outside their region, and which
  // are within the grid of another process.
  // -----------------------------------------------------------------

  // 4. For every other process, build a list of points I have
  // which are in the region of that process.  In practice, the
  // processes for which I need to request points IDs should be
  // a small subset of all the other processes.

  // question: if the vtkPointArray has type double, should we
  // send doubles instead of floats to insure we get the right
  // global ID back?

  vtkFloatArray **ptarrayOut = new vtkFloatArray * [nprocs];
  memset(ptarrayOut, 0, sizeof(vtkFloatArray *) * nprocs);
  vtkIntArray **localIds     = new vtkIntArray * [nprocs];
  memset(localIds, 0, sizeof(vtkIntArray *) * nprocs);
  int *next = new int [nprocs];
  int *next3 = new int [nprocs];

  for (ptId = 0; ptId < nGridPoints; ptId++)
    {
    processId = globalIds->GetValue(ptId);

    if (processId >= 0) continue;   // that's one of mine

    processId *= -1;
    processId -= 1;

    if (ptarrayOut[processId] == NULL)
      {
      int npoints = numPointsOutside[processId];

      ptarrayOut[processId] = vtkFloatArray::New();
      ptarrayOut[processId]->SetNumberOfValues(npoints* 3);
      localIds[processId] = vtkIntArray::New();
      localIds[processId]->SetNumberOfValues(npoints);
      next[processId] = 0;
      next3[processId] = 0;
      }

    localIds[processId]->SetValue(next[processId]++,ptId);

    double *dp = grid->GetPoints()->GetPoint(ptId);

    ptarrayOut[processId]->SetValue(next3[processId]++, (float)dp[0]);
    ptarrayOut[processId]->SetValue(next3[processId]++, (float)dp[1]);
    ptarrayOut[processId]->SetValue(next3[processId]++, (float)dp[2]);
    }

  delete [] numPointsOutside;
  delete [] next;
  delete [] next3;

  // 5. Do pairwise exchanges of the points we want global IDs for

  vtkFloatArray **ptarrayIn = this->ExchangeFloatArrays(ptarrayOut, 0x1005);

  this->FreeFloatArrays(ptarrayOut);

  // 6. Find the global point IDs that have been requested of me

  vtkIntArray **idarrayOut = 
    this->FindGlobalPointIds(ptarrayIn, globalIds, grid);

  this->FreeFloatArrays(ptarrayIn);

  // 7. Do pairwise exchanges of the global point IDs

  vtkIntArray **idarrayIn = this->ExchangeIntArrays(idarrayOut, 0x1007);

  this->FreeIntArrays(idarrayOut);

  // 8. Update my ugrid with these mutually agreed upon global point IDs

  for (processId = 0; processId < nprocs; processId++)
    {
    if (idarrayIn[processId] == NULL)
      {
      continue;
      }

    int count = idarrayIn[processId]->GetNumberOfTuples();
      
    for (ptId = 0; ptId < count; ptId++)
      {
      int myLocalId = localIds[processId]->GetValue(ptId);
      int yourGlobalId = idarrayIn[processId]->GetValue(ptId);


      if (yourGlobalId >= 0)
        {
        globalIds->SetValue(myLocalId, yourGlobalId);
        myNumPointsUnassigned -= 1;
        }
      }
    }

  this->FreeIntArrays(idarrayIn);
  this->FreeIntArrays(localIds);

  // 10. One class of points remains without a global node id assigned
  // to it.  These are points in my grid, which lie outside my spatial
  // region, and which are not part of the grid belonging to the process
  // assigned to that region.  The processes exchange the count of how
  // many such points there are.

  int *numPointsUnassigned = this->ExchangeCounts(myNumPointsUnassigned, 0x100f);

  // 11. Assign a global node ID to these points
      

  if (myNumPointsUnassigned > 0)
    {
    firstId = numGlobalIds;
    for (processId = 0; processId < nprocs; processId++)
      {
      if (processId < this->MyId) firstId += numPointsUnassigned[processId];
      } 
    
    for (ptId = 0; ptId < nGridPoints; ptId++)
      {
      if (globalIds->GetValue(ptId) < 0)
        {
        globalIds->SetValue(ptId, firstId++);
        }
      }
    }

  delete [] numPointsUnassigned;

  // 12. Add global node Id array to the grid

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
vtkIntArray **vtkDistributedDataFilter::FindGlobalPointIds(
     vtkFloatArray **ptarray, vtkIntArray *ids, vtkUnstructuredGrid *grid)
{
  vtkKdTree *kd = vtkKdTree::New();

  kd->BuildLocatorFromPoints(grid->GetPoints());

  int procId;
  int ptId, localId;

  int nprocs = this->NumProcesses;

  vtkIntArray **gids = new vtkIntArray * [nprocs];

  for (procId = 0; procId < nprocs; procId++)
    {
    if ((ptarray[procId] == NULL) || 
        (ptarray[procId]->GetNumberOfTuples() == 0))
      {
      gids[procId] = NULL;
      continue;
      }

    gids[procId] = vtkIntArray::New();

    int npoints = ptarray[procId]->GetNumberOfTuples() / 3;

    gids[procId]->SetNumberOfValues(npoints);
    int next = 0;

    float *pt = ptarray[procId]->GetPointer(0);

    for (ptId = 0; ptId < npoints; ptId++)
      {
      localId = kd->FindPoint(pt);

      if (localId >= 0)
        {
        gids[procId]->SetValue(next++, ids->GetValue(localId));  // global Id
        }
      else
        {
        gids[procId]->SetValue(next++, -1);   // This point is not in my grid
        }
      pt += 3;
      }
    }

  kd->Delete();

  return gids;
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

vtkIntArray **vtkDistributedDataFilter::MakeProcessLists(
                                    vtkIntArray **pointIds,
                                    vtkstd::multimap<int,int> *procs)
{
  // Build a list of pointId/processId pairs for each process that
  // sent me point IDs.  The process Ids are all those processes
  // that had the specified point in their ghost level zero grid.

  int nprocs = this->NumProcesses;

  vtkstd::multimap<int, int>::iterator mapIt;

  vtkIntArray **processList = new vtkIntArray * [nprocs];
  memset(processList, 0, sizeof (vtkIntArray *) * nprocs);

  for (int i=0; i<nprocs; i++)
    {
    if (pointIds[i] == NULL) continue;

    int size = pointIds[i]->GetNumberOfTuples();

    if (size > 0)
      {
      for (int j=0; j<size; )
        {
        // These are all the points in my spatial region
        // for which process "i" needs ghost cells.

        int gid = pointIds[i]->GetValue(j);
        int ncells = pointIds[i]->GetValue(j+1);

        mapIt = procs->find(gid);

        if (mapIt != procs->end())
          {
          while (mapIt->first == gid)
            {
            int processId = mapIt->second;
  
            if (processId != i)
              {
              // Process "i" needs to know that process
              // "processId" also has cells using this point
  
              if (processList[i] == NULL)
                {
                processList[i] = vtkIntArray::New();
                }
              processList[i]->InsertNextValue(gid);
              processList[i]->InsertNextValue(processId);
              }
            ++mapIt;
            }
          }
        j += (2 + ncells);
        }
      }
    }

  return processList;
}
vtkIntArray *vtkDistributedDataFilter::AddPointAndCells(
                        int gid, int localId, vtkUnstructuredGrid *grid, 
                        int *gidCells, vtkIntArray *ids)
{
  if (ids == NULL)
    {
    ids = vtkIntArray::New();
    }

  ids->InsertNextValue(gid);

  vtkIdList *cellList = vtkIdList::New();

  grid->GetPointCells(localId, cellList);

  vtkIdType numCells = cellList->GetNumberOfIds(); 

  ids->InsertNextValue((int)numCells);

  for (int j=0; j<numCells; j++)
    {
    int globalCellId = gidCells[cellList->GetId(j)];
    ids->InsertNextValue(globalCellId);
    }

  return ids;
}
vtkIntArray **vtkDistributedDataFilter::GetGhostPointIds(
                             int ghostLevel, vtkUnstructuredGrid *grid,
                             int AddCellsIAlreadyHave)
{
  int processId = -1;
  int regionId = -1;

  vtkPKdTree *kd = this->Kdtree;
  int nprocs = this->NumProcesses;
  int me = this->MyId;

  vtkPoints *pts = grid->GetPoints();
  vtkIdType numPoints = pts->GetNumberOfPoints();

  int rc = this->GlobalNodeIdAccessStart(grid);

  if (rc == 0)
    {
    return NULL;
    }

  vtkIntArray **ghostPtIds = new vtkIntArray * [nprocs];
  memset(ghostPtIds, 0, sizeof(vtkIntArray *) * nprocs);

  vtkDataArray *da = grid->GetCellData()->GetArray(
          vtkDistributedDataFilter::TemporaryGlobalCellIds);
  vtkIntArray *ia= vtkIntArray::SafeDownCast(da);
  int *gidsCell = ia->GetPointer(0);

  da = grid->GetPointData()->GetArray("vtkGhostLevels");
  vtkUnsignedCharArray *uca = vtkUnsignedCharArray::SafeDownCast(da);
  unsigned char *levels = uca->GetPointer(0);

  unsigned char level = (unsigned char)(ghostLevel - 1);

  for (int i=0; i<numPoints; i++)
    {
    double *pt = pts->GetPoint(i);
    regionId = kd->GetRegionContainingPoint(pt[0], pt[1], pt[2]);
    processId = kd->GetProcessAssignedToRegion(regionId);

    if (ghostLevel == 1)
      {
      // I want all points that are outside my spatial region

      if (processId == me) continue;

      // Don't include points that are not part of any cell

      int used = vtkDistributedDataFilter::LocalPointIdIsUsed(grid, i);

      if (!used) continue;
      }
    else
      {
      // I want all points having the correct ghost level

      if (levels[i] != level) continue;
      }

    int gid = this->GlobalNodeIdAccessGetId(i);

    if (AddCellsIAlreadyHave)
      {
      // To speed up exchange of ghost cells and creation of
      // new ghost cell grid, we tell other
      // processes which cells we already have, so they don't
      // send them to us.

      ghostPtIds[processId] = 
        vtkDistributedDataFilter::AddPointAndCells(gid, i, grid, gidsCell,
                                       ghostPtIds[processId]);
      }
    else
      {
      if (ghostPtIds[processId] == NULL)
        {
        ghostPtIds[processId] = vtkIntArray::New();
        }
      ghostPtIds[processId]->InsertNextValue(gid);
      ghostPtIds[processId]->InsertNextValue(0);
      }
    }
  return ghostPtIds;
}
int vtkDistributedDataFilter::LocalPointIdIsUsed(
                              vtkUnstructuredGrid *grid, int ptId)
{
  int used = 1;

  int numPoints = grid->GetNumberOfPoints();

  if ((ptId < 0) || (ptId >= numPoints))
    {
    used = 0;
    }
  else
    {
    vtkIdType id = (vtkIdType)ptId;
    vtkIdList *cellList = vtkIdList::New();

    grid->GetPointCells(id, cellList);

    if (cellList->GetNumberOfIds() == 0)
      {
      used = 0;
      }

    cellList->Delete();
    }

  return used;
}
int vtkDistributedDataFilter::GlobalPointIdIsUsed(vtkUnstructuredGrid *grid, 
                              int ptId, vtkstd::map<int, int> *globalToLocal)
{
  int used = 1;

  vtkstd::map<int, int>::iterator mapIt;

  mapIt = globalToLocal->find(ptId);

  if (mapIt == globalToLocal->end())
    {
    used = 0;
    }
  else
    {
    int id = mapIt->second;

    used = vtkDistributedDataFilter::LocalPointIdIsUsed(grid, id);
    }

  return used;
}
int vtkDistributedDataFilter::FindId(vtkIntArray *ids, int gid, int startLoc)
{
  int gidLoc = -1;

  if (ids == NULL) return gidLoc;

  int numIds = ids->GetNumberOfTuples();

  while ((ids->GetValue(startLoc) != gid) && (startLoc < numIds))
    {
    int ncells = ids->GetValue(++startLoc);
    startLoc += (ncells + 1);
    }

  if (startLoc < numIds)
    {
    gidLoc = startLoc;
    }

  return gidLoc;
}

// We create an expanded grid with the required number of ghost
// cells.  This is for the case where IncludeAllIntersectingCells is OFF.
// This means that when the grid was redistributed, each cell was 
// uniquely assigned to one process, the process owning the spatial 
// region that the cell's centroid lies in.

vtkUnstructuredGrid *
vtkDistributedDataFilter::AddGhostCellsUniqueCellAssignment(
                                     vtkUnstructuredGrid *myGrid,
                                     vtkstd::map<int, int> *globalToLocalMap)
{
  int i,j,k;
  int ncells=0;
  int processId=0; 
  int gid=0; 
  int size=0;

  int nprocs = this->NumProcesses;
  int me = this->MyId;

  int gl = 1;

  // For each ghost level, processes request and send ghost cells

  vtkUnstructuredGrid *newGhostCellGrid = NULL;
  vtkIntArray **ghostPointIds = NULL;
  
  vtkstd::multimap<int, int> insidePointMap;
  vtkstd::multimap<int, int>::iterator mapIt;

  while (gl <= this->GhostLevel)
    {
    // For ghost level 1, create a list for each process (not 
    // including me) of all points I have in that process' 
    // assigned region.  We use this list for two purposes:
    // (1) to build a list on each process of all other processes
    // that have cells containing points in our region, (2)
    // these are some of the points that we need ghost cells for.
    //
    // For ghost level above 1, create a list for each process
    // (including me) of all my points in that process' assigned 
    // region for which I need ghost cells.
  
    if (gl == 1)
      {
      ghostPointIds = this->GetGhostPointIds(gl, myGrid, 0);
      }
    else
      {
      ghostPointIds = this->GetGhostPointIds(gl, newGhostCellGrid, 1);
      }
  
    // Exchange these lists.
  
    vtkIntArray **insideIds = this->ExchangeIntArrays(ghostPointIds, 0x101f);

    if (gl == 1)
      {
      // For every point in my region that was sent to me by another process,
      // I now know the identity of all processes having cells containing
      // that point.  Begin by building a mapping from point IDs to the IDs
      // of processes that sent me that point.
    
      for (i=0; i<nprocs; i++)
        {
        if (insideIds[i] == NULL) continue;

        size = insideIds[i]->GetNumberOfTuples();
    
        if (size > 0)
          {
          for (j=0; j<size; j+=2)
            {
            // map global point id to process ids
            insidePointMap.insert(
              vtkstd::pair<int, int>(insideIds[i]->GetValue(j), i));
            }
          }
        }
      }

    // Build a list of pointId/processId pairs for each process that
    // sent me point IDs.  To process P, for every point ID sent to me
    // by P, I send the ID of every other process (not including myself
    // and P) that has cells in it's ghost level 0 grid which use
    // this point. 

    vtkIntArray **processListSent = MakeProcessLists(insideIds, &insidePointMap);

    // Exchange these new lists.

    vtkIntArray **processList = this->ExchangeIntArrays(processListSent, 0x103f);

    vtkDistributedDataFilter::FreeIntArrays(processListSent);

    // I now know the identity of every process having cells containing
    // points I need ghost cells for.  Create a request to each process
    // for these cells.

    vtkIntArray **ghostCellsPlease = new vtkIntArray * [nprocs];
    for (i=0; i<nprocs; i++)
      {
      ghostCellsPlease[i] = vtkIntArray::New();
      ghostCellsPlease[i]->SetNumberOfComponents(1);
      }

    for (i=0; i<nprocs; i++)
      {
      if (i == me) continue;

      if (ghostPointIds[i])       // points I have in your spatial region,
        {                         // maybe you have cells that use them?

        for (j=0; j<ghostPointIds[i]->GetNumberOfTuples(); j++)
          {
          ghostCellsPlease[i]->InsertNextValue(ghostPointIds[i]->GetValue(j));
          }
        }
      if (processList[i])         // other processes you say that also have
        {                         // cells using those points
        int size = processList[i]->GetNumberOfTuples();
        int *array = processList[i]->GetPointer(0);
        int nextLoc = 0;
  
        for (j=0; j < size; j += 2)
          {
          gid = array[j];
          processId = array[j+1];
  
          ghostCellsPlease[processId]->InsertNextValue(gid);

          // add the list of cells I already have for this point

          int where = 
            vtkDistributedDataFilter::FindId(ghostPointIds[i], gid, nextLoc);

          if (where < 0)
            {
            // error
            cout << "error 1" << endl;
            }

          ncells = ghostPointIds[i]->GetValue(where + 1);

          ghostCellsPlease[processId]->InsertNextValue(ncells);

          for (k=0; k <ncells; k++)
            {
            int cellId = ghostPointIds[i]->GetValue(where + 2 + k);
            ghostCellsPlease[processId]->InsertNextValue(cellId);
            }

          nextLoc = where;
          }
        }
      if ((gl==1) && insideIds[i])   // points you have in my spatial region,
        {                            // which I may need ghost cells for
        for (j=0; j<insideIds[i]->GetNumberOfTuples();)
          {
          gid = insideIds[i]->GetValue(j);  
          int used = vtkDistributedDataFilter::GlobalPointIdIsUsed(
                                  myGrid, gid, globalToLocalMap);
          if (used)
            {
            ghostCellsPlease[i]->InsertNextValue(gid);
            ghostCellsPlease[i]->InsertNextValue(0);
            }

          ncells = insideIds[i]->GetValue(j+1);
          j += (ncells + 2);
          }
        }
      }

    if (gl > 1)
      {
      if (ghostPointIds[me])   // these points are actually inside my region
        {
        size = ghostPointIds[me]->GetNumberOfTuples();

        for (i=0; i<size;)
          {
          gid = ghostPointIds[me]->GetValue(i);
          ncells = ghostPointIds[me]->GetValue(i+1);

          mapIt = insidePointMap.find(gid);

          if (mapIt != insidePointMap.end())
            {
            while (mapIt->first == gid)
              { 
              processId = mapIt->second;
              ghostCellsPlease[processId]->InsertNextValue(gid);
              ghostCellsPlease[processId]->InsertNextValue(ncells);

              for (k=0; k<ncells; k++)
                {
                int cellId = ghostPointIds[me]->GetValue(i+1+k);
                ghostCellsPlease[processId]->InsertNextValue(cellId);
                }

              ++mapIt;
              }
            }
          i += (ncells + 2);
          }
        }
      }

    this->FreeIntArrays(ghostPointIds);
    this->FreeIntArrays(insideIds);
    this->FreeIntArrays(processList);

    // Exchange these ghost cell requests.

    vtkIntArray **ghostCellRequest 
      = this->ExchangeIntArrays(ghostCellsPlease, 0x107f);
  
    this->FreeIntArrays(ghostCellsPlease);
  
    // Build a sub grid satisfying each request received.
  
    vtkUnstructuredGrid **subGrids =
      this->BuildRequestedGrids(ghostCellRequest, myGrid, globalToLocalMap);

    this->FreeIntArrays(ghostCellRequest);
  
    // Exchange subgrids, and merge them into a ghost cell grid. 
  
    newGhostCellGrid = this->ExchangeMergeSubGrids(subGrids, 
                              newGhostCellGrid, gl, globalToLocalMap);

    gl++;
  }

  insidePointMap.erase(insidePointMap.begin(),insidePointMap.end());

  vtkUnstructuredGrid *newGrid = NULL;

  if (newGhostCellGrid && (newGhostCellGrid->GetNumberOfCells() > 0))
    {
    vtkDataSet *grids[2];

    grids[0] = myGrid;
    grids[1] = newGhostCellGrid;
   
    newGrid = vtkDistributedDataFilter::MergeGrids(2, grids,
             this->GetGlobalIdArrayName(), 0.0, NULL);
    }
  else
    {
    newGrid = myGrid;
    }

  return newGrid;
}

// We create an expanded grid that contains the ghost cells we need.
// This is in the case where IncludeAllIntersectingCells is ON.  This
// is easier in some respects because we know if that if a point lies
// in a region owned by a particular process, that process has all
// cells which use that point.  So it is easy to find ghost cells.
// On the otherhand, because cells are not uniquely assigned to regions,
// we may get multiple processes sending us the same cell, so we
// need to filter these out.

vtkUnstructuredGrid *
vtkDistributedDataFilter::AddGhostCellsDuplicateCellAssignment(
                                     vtkUnstructuredGrid *myGrid,
                                     vtkstd::map<int, int> *globalToLocalMap)
{
  int i,j;

  int nprocs = this->NumProcesses;
  int me = this->MyId;

  int gl = 1;

  // For each ghost level, processes request and send ghost cells

  vtkUnstructuredGrid *newGhostCellGrid = NULL;
  vtkIntArray **ghostPointIds = NULL;
  vtkIntArray **extraGhostPointIds = NULL;

  vtkstd::map<int, int>::iterator mapIt;

  vtkPoints *pts = myGrid->GetPoints();

  while (gl <= this->GhostLevel)
    {
    // For ghost level 1, create a list for each process of points
    // in my grid which lie in that other process' spatial region.
    // This is normally all the points for which I need ghost cells, 
    // with one EXCEPTION.  If a cell is axis-aligned, and a face of 
    // the cell is on my upper boundary, then the vertices of this
    // face are in my spatial region, but I need their ghost cells.
    // I can detect this case when the process across the boundary
    // sends me a request for ghost cells of these points.
    //
    // For ghost level above 1, create a list for each process of
    // points in my ghost grid which are in that process' spatial
    // region and for which I need ghost cells.
  
    if (gl == 1)
      {
      ghostPointIds = this->GetGhostPointIds(gl, myGrid, 1);
      }
    else
      {
      ghostPointIds = this->GetGhostPointIds(gl, newGhostCellGrid, 1);
      }
  
    // Exchange these lists.
  
    vtkIntArray **insideIds = this->ExchangeIntArrays(ghostPointIds,0x10ff);

    this->FreeIntArrays(ghostPointIds);

    // For ghost level 1, examine the points Ids I received from
    // other processes, to see if the exception described above
    // applies and I need ghost cells from them for those points.

    if (gl == 1)
      {
      vtkDataArray *da = myGrid->GetCellData()->GetArray(
              vtkDistributedDataFilter::TemporaryGlobalCellIds);
      vtkIntArray *ia= vtkIntArray::SafeDownCast(da);
      int *gidsCell = ia->GetPointer(0);

      extraGhostPointIds = new vtkIntArray * [nprocs];

      for (i=0; i<nprocs; i++)
        {
        extraGhostPointIds[i] = NULL;

        if (i == me) continue;

        if (insideIds[i] == NULL) continue;
  
        int size = insideIds[i]->GetNumberOfTuples();
  
        for (j=0; j<size;)
          {
          int gid = insideIds[i]->GetValue(j);
  
          mapIt = globalToLocalMap->find(gid);
  
          if (mapIt == globalToLocalMap->end())
            {
            // error
            cout << " error 2 " << endl;
            }
          int localId = mapIt->second;

          double *pt = pts->GetPoint(localId);

          int interior = this->StrictlyInsideMyBounds(pt[0], pt[1], pt[2]);

          if (!interior)
            {
            extraGhostPointIds[i] = this->AddPointAndCells(gid, localId, 
                            myGrid, gidsCell, extraGhostPointIds[i]);
            }

          int ncells = insideIds[i]->GetValue(j+1);
          j += (ncells + 2);
          }
        }

      // Exchange these lists.
  
      vtkIntArray **extraInsideIds = 
        this->ExchangeIntArrays(extraGhostPointIds, 0x11ff);

      this->FreeIntArrays(extraGhostPointIds);
  
      // Add the extra point ids to the previous list

      for (i=0; i<nprocs; i++)
        {
        if (i == me) continue;

        if (extraInsideIds[i])
          { 
          int size = extraInsideIds[i]->GetNumberOfTuples();

          for (j=0; j<size; j++)
            {
            insideIds[i]->InsertNextValue(extraInsideIds[i]->GetValue(j));
            }
          }
        }
        this->FreeIntArrays(extraInsideIds);
      }

    // Build a sub grid satisfying each request received.
  
    vtkUnstructuredGrid **subGrids =
      this->BuildRequestedGrids(insideIds, myGrid, globalToLocalMap);
  
    this->FreeIntArrays(insideIds);
  
    // Exchange subgrids, and merge them into a ghost cell grid. 
  
    newGhostCellGrid = this->ExchangeMergeSubGrids(subGrids, 
                              newGhostCellGrid, gl, globalToLocalMap);

    gl++;
  }

  vtkUnstructuredGrid *newGrid = NULL;

  if (newGhostCellGrid && (newGhostCellGrid->GetNumberOfCells() > 0))
    {
    vtkDataSet *grids[2];

    grids[0] = myGrid;
    grids[1] = newGhostCellGrid;

    newGrid = vtkDistributedDataFilter::MergeGrids(2, grids,
             this->GetGlobalIdArrayName(), 0.0, NULL);
    }
  else
    {
    newGrid = myGrid;
    }

  return newGrid;
}

// For every process that sent me a list of point IDs, create a sub grid
// containing all the cells I have containing those points.  We omit
// cells the remote process already has.

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

  vtkDataArray *da = grid->GetCellData()->GetArray(
                vtkDistributedDataFilter::TemporaryGlobalCellIds);

  vtkIntArray *gidCells = vtkIntArray::SafeDownCast(da);

  vtkUnstructuredGrid **subGrids = new vtkUnstructuredGrid * [nprocs];

  for (proc = 0; proc < nprocs; proc++)
    {
    subGrids[proc] = NULL;

    if (globalPtIds[proc] == NULL)
      {
      continue;
      }

    if ((nelts = globalPtIds[proc]->GetNumberOfTuples()) == 0)
      {
      continue;
      }

    int *ptarray = globalPtIds[proc]->GetPointer(0);

    vtkstd::set<vtkIdType> subGridCellIds;

    int nYourCells = 0;

    for (id = 0; id < nelts; id += (nYourCells + 2))
      {
      int ptId = ptarray[id];
      nYourCells = ptarray[id+1];

      imap = ptIdMap->find(ptId);

      if (imap == ptIdMap->end())
        {
        continue; // I don't have this point
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
        vtkDistributedDataFilter::RemoveRemoteCellsFromList(cellList, 
                                     gidCells, remoteCells, nYourCells);
        }

      vtkIdType nSendCells = cellList->GetNumberOfIds();

      if (nSendCells == 0)
        {
        continue;
        }

      for (cellId = 0; cellId < nSendCells; cellId++)
        {
        subGridCellIds.insert(cellList->GetId(cellId));
        }
      }

    int numUniqueCellIds = subGridCellIds.size();

    if (numUniqueCellIds == 0)
      {
      subGrids[proc] = NULL;
      continue;
      }

    vtkIdList *cellIdList = vtkIdList::New();

    cellIdList->SetNumberOfIds(numUniqueCellIds);
    vtkIdType next = 0;

    vtkstd::set<vtkIdType>::iterator it;

    for (it = subGridCellIds.begin(); it != subGridCellIds.end(); ++it)
      {
      cellIdList->SetId(next++, *it);
      }
  
    vtkExtractCells *ec = vtkExtractCells::New();
    ec->SetInput(grid);
    ec->SetCellList(cellIdList);
    ec->Update();

    subGrids[proc] = ec->GetOutput();
    subGrids[proc]->Register(this);
    ec->Delete();
    }

  cellList->Delete();

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

  int rc = this->PairWiseDataExchange(sizes, gridArrays, 0x13ff);

  if (rc)
    {
    delete [] gridArrays;
    delete [] sizes;
    ghostCellGrid->Delete();
    vtkErrorMacro(<< "vtkDistributedDataFilter::ExchangeMergeSubGrids error");
    return NULL;
    }

  int numSources = 0;

  const char *nodeIdArrayName  = NULL;

  for (proc = 0; proc < nprocs; proc++)
    {
    if (sizes[proc] > 0)
      {
      grids[proc] = this->UnMarshallDataSet(gridArrays[proc], sizes[proc]);

      delete [] gridArrays[proc];

      numSources++;

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

  int ngrids = numSources + (ghostCellGrid ? 1 : 0);
  vtkDataSet **gridPtr = new vtkDataSet * [ngrids];

  int next = 0;
  if (ghostCellGrid) gridPtr[next++] = ghostCellGrid;
  for (proc=0; proc < nprocs; proc++)
    {
    if (sizes[proc] > 0)
      {
      gridPtr[next++] = grids[proc];
      }
    }
  
  delete [] sizes;
  delete [] grids;

  const char *cellIdArrayName;
  if (this->IncludeAllIntersectingCells)
    {
    // It's possible that different remote processes will send me the
    // same cell.  I'll ask vtkMergeCells to filter these out.

    cellIdArrayName = vtkDistributedDataFilter::TemporaryGlobalCellIds;
    }
  else
    {
    cellIdArrayName = NULL;
    }

  vtkUnstructuredGrid *returnGrid = 
          vtkDistributedDataFilter::MergeGrids(ngrids, gridPtr,
          nodeIdArrayName, 0.0, cellIdArrayName);

  delete [] gridPtr;

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

  return returnGrid;
}
vtkUnstructuredGrid *vtkDistributedDataFilter::MergeGrids(int nsets, 
         vtkDataSet **sets,
         const char *globalNodeIdArrayName, float pointMergeTolerance, 
         const char *globalCellIdArrayName)
{
  if (nsets == 0)
    {
    return NULL;
    }

  vtkUnstructuredGrid *newGrid = vtkUnstructuredGrid::New();
  
  vtkMergeCells *mc = vtkMergeCells::New();
  mc->SetUnstructuredGrid(newGrid);
  
  mc->SetTotalNumberOfDataSets(nsets);

  int totalPoints = 0;
  int totalCells = 0;

  for (int i=0; i<nsets; i++)
    {
    totalPoints += sets[i]->GetNumberOfPoints();
    totalCells += sets[i]->GetNumberOfCells();
    }

  mc->SetTotalNumberOfPoints(totalPoints);
  mc->SetTotalNumberOfCells(totalCells);

  if (globalNodeIdArrayName)
    {
    mc->SetGlobalIdArrayName(globalNodeIdArrayName);
    }
  else
    {
    mc->SetPointMergeTolerance(pointMergeTolerance);
    }

  if (globalCellIdArrayName)
    {
    mc->SetGlobalCellIdArrayName(globalCellIdArrayName);
    }

  for (int i=0; i<nsets; i++)
    {
    mc->MergeDataSet(sets[i]);
    sets[i]->Delete();
    }
  
  mc->Finish();
  mc->Delete();

  return newGrid;
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
