// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedDataFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  Copyright (C) 2003 Sandia Corporation
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the U.S. Government.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that this Notice and any statement
  of authorship are reproduced on all copies.

  Contact: Lee Ann Fisk, lafisk@sandia.gov

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDistributedDataFilter
//
// .SECTION Description
//
// .SECTION See Also

#include <vtkDistributedDataFilter.h>
#include <vtkExtractCells.h>
#include <vtkMergeCells.h>
#include <vtkObjectFactory.h>
#include <vtkPKdTree.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetAttributes.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkIdList.h>
#include <vtkMultiProcessController.h>
#include <vtkDataSetWriter.h>
#include <vtkDataSetReader.h>
#include <vtkCharArray.h>
#include <vtkBoxClipDataSet.h>
#include <vtkPlanes.h>
#include <vtkPlane.h>

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

vtkCxxRevisionMacro(vtkDistributedDataFilter, "1.4");

vtkStandardNewMacro(vtkDistributedDataFilter);

vtkDistributedDataFilter::vtkDistributedDataFilter()
{
  this->Kdtree = NULL;

  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

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
  if (this->Controller == c)
    {
    return;
    }
  this->Modified();

  if (this->Kdtree)
    {
    this->Kdtree->SetController(c);
    }

  if (this->Controller)
    {
    this->Controller->UnRegister(this);
    this->Controller = NULL;
    }
  if (c == NULL)
    {
    return;
    }

  this->Controller = c;
  this->Controller->Register(this);

  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyLocalId = c->GetLocalProcessId();
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
  vtkDataSet *input           = this->GetInput();

  vtkDebugMacro(<< "vtkDistributedDataFilter::Execute()");

  if (this->Kdtree == NULL)
    {
    this->Kdtree = vtkPKdTree::New();
    this->Kdtree->SetController(this->Controller);
    }

  if (this->Controller == NULL)
    {
    vtkErrorMacro("Must SetController first");
    return;
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

  // Stage (2) - Redistribute data, so that each process gets a ugrid
  //   containing the cells in it's assigned spatial regions

  TIMER("Redistribute data among processors");

  vtkUnstructuredGrid *finalGrid = NULL;

#ifdef VTK_USE_MPI

  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  if (mpiContr)
    {
    finalGrid = this->MPIRedistribute(mpiContr);   // faster
    }
  else
    {
    finalGrid = this->GenericRedistribute();
    }
#else
  finalGrid = this->GenericRedistribute();
#endif

  TIMERDONE("Redistribute data among processors");

  if (finalGrid == NULL)
    {
    vtkErrorMacro("Unable to redistribute data");
    }

  // Possible Stage (3) - Clip cells to the spatial region boundaries

  if (this->ClipCells)
    {
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

#ifdef VTK_USE_MPI

vtkUnstructuredGrid 
  *vtkDistributedDataFilter::MPIRedistribute(vtkMPIController *mpiContr)
{
  int proc, offset, source, target;

  int me = this->MyLocalId;
  int nnodes = this->NumProcesses;

  vtkUnstructuredGrid *mySubGrid = NULL;
  char **packedGrids = new char * [nnodes];

  int *yourNumCells  = new int [nnodes];
  int *yourNumPoints = new int [nnodes];
  int *yourSizeData  = new int [nnodes];

  int *myNumCells  = new int [nnodes];
  int *myNumPoints = new int [nnodes];
  int *mySizeData  = new int [nnodes];

  // create a ugrid for every process from data I have read in

  TIMER("Create cell lists");
  
  if (this->IncludeAllIntersectingCells)
    {
    this->Kdtree->IncludeRegionBoundaryCellsOn();
    }
  
  this->Kdtree->CreateCellLists();  // req'd by ExtractCellsForProcess
    
  TIMERDONE("Create cell lists");
    
  TIMER("Extract sub grids");

  for (proc=0; proc < nnodes; proc++)
    {
    yourNumCells[proc] = yourNumPoints[proc] = yourSizeData[proc] = 0;
    packedGrids[proc] = NULL;

    vtkUnstructuredGrid *extractedGrid = this->ExtractCellsForProcess(proc);

    if (!extractedGrid )
      {
      continue;
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

    yourNumCells[proc] =   extractedGrid->GetNumberOfCells();
    yourNumPoints[proc] =  extractedGrid->GetNumberOfPoints();

    if (proc != me)
      {
      packedGrids[proc] = this->MarshallDataSet(extractedGrid, yourSizeData[proc]);
      extractedGrid->UnRegister(this);
      }
    else
      {
      mySubGrid = extractedGrid;
      }
    }
  TIMERDONE("Extract sub grids");

  this->Kdtree->DeleteCellLists();

  // every process learns how many cells/points will be in it's new ugrid

  int TotalCells  = myNumCells[me]  = yourNumCells[me];
  int TotalPoints = myNumPoints[me] = yourNumPoints[me];
  int TotalSets = (mySubGrid ? 1 : 0);

  int indata[3], outdata[3];

  int largestSizeData=0;

  TIMER("Transmit data size info");

  for (offset = 1; offset < nnodes; offset++)
    {
    target = (me + offset) % nnodes;
    source = (me + nnodes - offset) % nnodes;

    outdata[0] = yourNumCells[target];
    outdata[1] = yourNumPoints[target];
    outdata[2] = yourSizeData[target];

    // post receive from source

    vtkMPICommunicator::Request req;

    mpiContr->NoBlockReceive(indata, 3, source, 0x01, req);

    // send to target

    mpiContr->Send(outdata, 3, target, 0x01);

    // await info from source

    req.Wait();

    myNumCells[source] = indata[0];
    myNumPoints[source] = indata[1];
    mySizeData[source] = indata[2];

    TotalCells +=  myNumCells[source];
    TotalPoints += myNumPoints[source];

    if (myNumCells[source] > 0) TotalSets++;

    if (mySizeData[source] > largestSizeData)
      {
      largestSizeData = mySizeData[source]; 
      }
    }

  TIMERDONE("Transmit data size info");

  // initialize my new ugrid - use vtkMergeCells object which can merge
  //   in ugrids with same field arrays, filtering out duplicate points
  //   as it goes.

  TIMER("Set up merge process");

  vtkUnstructuredGrid *newGrid = vtkUnstructuredGrid::New();

  vtkMergeCells *merged = vtkMergeCells::New();

  merged->SetTotalCells(TotalCells);
  merged->SetTotalPoints(TotalPoints);
  merged->SetTotalNumberOfDataSets(TotalSets);

  merged->SetUnstructuredGrid(newGrid);

  if (this->GlobalIdArrayName)
    {
    merged->SetGlobalIdArrayName(this->GlobalIdArrayName);
    }

  if (mySubGrid)
    {
    merged->MergeDataSet(mySubGrid);
    mySubGrid->UnRegister(this);
    mySubGrid = NULL;
    }

  // every process sends it's ugrid contribution to every other

  char *buf = new char [largestSizeData];

  if (!buf)
    {
    vtkErrorMacro("MPIRedistribute: memory allocation");
    return NULL;
    }

  TIMERDONE("Set up merge process");

  TIMER("Send/Receive/merge");

  for (offset = 1; offset < nnodes; offset++)
    {
    target = (me + offset) % nnodes;
    source = (me + nnodes - offset) % nnodes;

    // post receive from source

    vtkMPICommunicator::Request req;

    if (mySizeData[source] > 0)
      {
      mpiContr->NoBlockReceive(buf, mySizeData[source], source, 0x02, req);
      }

    // send to target

    if (packedGrids[target])
      {
      mpiContr->Send(packedGrids[target], yourSizeData[target], target, 0x02);
      delete [] packedGrids[target];
      }

    // await info from source

    if (mySizeData[source] > 0)
      {
      req.Wait();

      vtkUnstructuredGrid *remoteGrid = this->UnMarshallDataSet(buf, mySizeData[source]);

      merged->MergeDataSet(remoteGrid);

      remoteGrid->Delete();
      }
    }
  TIMERDONE("Send/Receive/merge");

  delete [] buf;

  delete [] packedGrids;

  delete [] yourNumCells;
  delete [] yourNumPoints;
  delete [] yourSizeData;
  delete [] myNumCells;
  delete [] myNumPoints;
  delete [] mySizeData;

  TIMER("Finish merge");

  merged->Finish();
  merged->Delete();

  TIMERDONE("Finish merge");

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
#endif
vtkUnstructuredGrid *vtkDistributedDataFilter::GenericRedistribute()
{
  vtkUnstructuredGrid *myGrid = NULL; 

  if (this->IncludeAllIntersectingCells)
    {
    this->Kdtree->IncludeRegionBoundaryCellsOn();
    }
  
  this->Kdtree->CreateCellLists();  // req'd by ExtractCellsForProcess

  for (int proc = 0; proc < this->NumProcesses; proc++)
    {
    vtkUnstructuredGrid *ugrid = this->ExtractCellsForProcess(proc);

    if (ugrid == NULL) continue;   // process is assigned no regions

    // Fan in and merge ugrids *************************************
    // If I am "proc", my grid is returned *************************
    // This call also deletes ugrid at the earliest opportunity ****

    vtkUnstructuredGrid *someGrid = this->ReduceUgridMerge(ugrid, proc);

    if (this->MyLocalId == proc)
      {
      myGrid = someGrid;
      }
    }
  this->Kdtree->DeleteCellLists();

  return myGrid;
}
vtkUnstructuredGrid *vtkDistributedDataFilter::ExtractCellsForProcess(int proc)
{
  vtkIdList *regions = vtkIdList::New();

  int nregions = this->Kdtree->GetRegionAssignmentList(proc, regions);

  if (nregions == 0) return NULL;

  // Create a new ugrid composed of these cells *******************

  vtkExtractCells *extCells = vtkExtractCells::New();

  // Copy input so the update call does not change the actual input.
  vtkDataSet* input = this->GetInput();
  vtkDataSet* tmp = input->NewInstance();
  tmp->ShallowCopy(input);
  extCells->SetInput(tmp);
  tmp->Delete();

  for (int reg=0; reg < nregions; reg++)
    {
    extCells->AddCellList(this->Kdtree->GetCellList(regions->GetId(reg)));

    if (this->IncludeAllIntersectingCells)
      {
      extCells->
        AddCellList(this->Kdtree->GetBoundaryCellList(regions->GetId(reg)));
      }
    }

  extCells->Update();

  // If this process has no cells for these regions, a ugrid gets
  // created anyway with field array information

  vtkUnstructuredGrid *ugrid = extCells->GetOutput();

  ugrid->Register(this);

  extCells->Delete();

  regions->Delete();

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
  int iAmRoot   = (root == this->MyLocalId);

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

  vtkIdList *Ids = vtkIdList::New();

  vtkIdList *regions = vtkIdList::New();

  int nregions = this->Kdtree->GetRegionAssignmentList(root, regions);
  
  for (int reg=0; reg < nregions; reg++)
    {
    // Get list of all processes that have data for this region

    Ids->Initialize();
    int nIds = this->Kdtree->GetProcessListForRegion(regions->GetId(reg), Ids);

    for (int p=0; p<nIds; p++)
      {
      haveData[Ids->GetId(p)] = 1;
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
      if (i == this->MyLocalId)
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

  int TotalPoints = ugrid->GetNumberOfPoints();
  int TotalCells = ugrid->GetNumberOfCells();
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

  merged->SetTotalCells(TotalCells);
  merged->SetTotalPoints(TotalPoints);
  merged->SetTotalNumberOfDataSets(nsources + 1);   // upper bound

  merged->SetUnstructuredGrid(newGrid);

  if (this->GlobalIdArrayName) // filter out duplicate points
    {
    merged->SetGlobalIdArrayName(this->GlobalIdArrayName);
    }

  if (iHaveData) merged->MergeDataSet(ugrid);

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
    data[0] = newGrid->GetNumberOfPoints();
    data[1] = newGrid->GetNumberOfCells();

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
void vtkDistributedDataFilter::ClipCellsToSpatialRegion(vtkUnstructuredGrid *grid)
{
  vtkPKdTree *kd = this->Kdtree;

  // Get a list of the ids of my spatial regions

  vtkIdList *myRegions = vtkIdList::New();

  kd->GetRegionAssignmentList(this->MyLocalId, myRegions);

  // Decompose it into convex sub-regions.  These sub-regions
  // are axis aligned boxes
  
  float *bounds;
  
  int numSubRegions = kd->MinimalNumberOfConvexSubRegions(
                            myRegions, &bounds);

  myRegions->Delete();

  if (numSubRegions > 1)
    {
    // here we would need to divide the grid into a separate grid for
    // each convex region, and then do the clipping

    vtkErrorMacro(<<
       "vtkDistributedDataFilter::ClipCellsToSpatialRegion - "
       "assigned regions do not form a single convex region");

    delete [] bounds;
    return ;
    }

  vtkBoxClipDataSet *clipped = vtkBoxClipDataSet::New();

  clipped->SetBoxClip(bounds[0], bounds[1],
                      bounds[2], bounds[3], bounds[4], bounds[5]);

  delete [] bounds;

  clipped->GenerateClipScalarsOn();

  clipped->SetInput(grid);

  clipped->Update();

  grid->ShallowCopy(clipped->GetOutput());

  clipped->Delete();

  return;
}


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
  os << indent << "MyLocalId: " << this->MyLocalId << endl;
  os << indent << "Timing: " << this->Timing << endl;
  os << indent << "TimerLog: " << this->TimerLog << endl;
}
