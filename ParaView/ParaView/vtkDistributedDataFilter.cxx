// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedDataFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include <vtkMultiProcessController.h>
#include <vtkMPIController.h>
#include <vtkDataSetWriter.h>
#include <vtkDataSetReader.h>
#include <vtkCharArray.h>

#define TIMER(s) if (this->Timing){ this->TimerLog->MarkStartEvent(s); }
#define TIMERDONE(s) if (this->Timing){ this->TimerLog->MarkEndEvent(s); }

vtkCxxRevisionMacro(vtkDistributedDataFilter, "1.1.2.2");

vtkStandardNewMacro(vtkDistributedDataFilter);

vtkDistributedDataFilter::vtkDistributedDataFilter()
{
  this->Kdtree = NULL;

  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->GlobalIdArrayName = NULL;

  this->RetainKdtree = 0;

  this->Timing = 0;
  this->MemInfo = 0;
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

  if (this->GlobalIdArrayName){
    delete [] this->GlobalIdArrayName;
  }

  if (this->TimerLog){
    this->TimerLog->Delete();
  }
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
  if (this->Controller == c){
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
  int fail;
  vtkDataSet *input           = this->GetInput();

  vtkDebugMacro(<< "vtkDistributedDataFilter::Execute()");

  if (this->Kdtree == NULL)
    {
    this->Kdtree = vtkPKdTree::New();
    this->Kdtree->SetController(this->Controller);
    }

  if (this->Controller == NULL){
    vtkErrorMacro("Must SetController first");
    return;
  }

  if (this->Timing){
    if (this->TimerLog == NULL) this->TimerLog = vtkTimerLog::New();
  }

  if (this->MemInfo){
    //#ifndef WIN32
    //if (this->MemInfoReader == NULL) this->MemInfoReader = vtkLinuxResources::New();
    //this->MemInfoReader->Update();
    //cout << "vtkDistributedDataFilter, Execute()" << endl;
    //this->MemInfoReader->PrintSelf(cout, 2);
    //#else
    cout << "Memory info not available." << endl;
    //#endif
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

  if (regionAssignmentScheme == vtkPKdTree::NoRegionAssignment){
    this->Kdtree->AssignRegionsContiguous();
  }

  this->Kdtree->SetDataSet(input);

  this->Kdtree->ProcessCellCountDataOn();

  this->Kdtree->RetainCellLocationsOn();

  TIMER("Build K-d tree in parallel");

  this->Kdtree->BuildLocator();

  TIMERDONE("Build K-d tree in parallel");

  if (this->Kdtree->GetNumberOfRegions() == 0){
    vtkErrorMacro("Unable to build k-d tree structure");
    return;
  }
  if (this->MemInfo){
    //#ifndef WIN32
    //if (this->MemInfoReader == NULL) this->MemInfoReader = vtkLinuxResources::New();
    //this->MemInfoReader->Update();
    //cout << "vtkDistributedDataFilter, after build of k-d tree, tables,  and cell lists" << endl;
    //this->MemInfoReader->PrintSelf(cout, 2);
    //#else
    cout << "Memory info not available." << endl;
    //#endif
  }

  // Stage (2) - Redistribute data, so that each process gets a ugrid
  //   containing the cells in it's assigned spatial regions

  vtkMPIController *mpiContr = vtkMPIController::SafeDownCast(this->Controller);

  TIMER("Redistribute data among processors");

  if (mpiContr){
    fail = this->MPIRedistribute(mpiContr);   // faster
  }
  else{
    fail = this->GenericRedistribute();
  }

  TIMERDONE("Redistribute data among processors");

  if (this->MemInfo){
    //#ifndef WIN32
    //if (this->MemInfoReader == NULL) this->MemInfoReader = vtkLinuxResources::New();
    //this->MemInfoReader->Update();
    //cout << "vtkDistributedDataFilter, after redistribution of data set" << endl;
    //this->MemInfoReader->PrintSelf(cout, 2);
    //#else
    cout << "Memory info not available." << endl;
    //#endif
  }

  if (fail){
    vtkErrorMacro("Unable to redistribute data");
  }

  if (!this->RetainKdtree){
    this->Kdtree->ReleaseTables();
    this->Kdtree->FreeSearchStructure();
    this->Kdtree->Delete();
    this->Kdtree = NULL;
  }
  if (this->MemInfo){
    //#ifndef WIN32
    //if (this->MemInfoReader == NULL) this->MemInfoReader = vtkLinuxResources::New();
    //this->MemInfoReader->Update();
    //cout << "vtkDistributedDataFilter, Execute() completed" << endl;
    //this->MemInfoReader->PrintSelf(cout, 2);
    //#else
    cout << "Memory info not available." << endl;
    //#endif
  }

  return;
}
int vtkDistributedDataFilter::MPIRedistribute(vtkMPIController *mpiContr)
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

  for (proc=0; proc < nnodes; proc++){

    yourNumCells[proc] = yourNumPoints[proc] = yourSizeData[proc] = 0;
    packedGrids[proc] = NULL;

    vtkUnstructuredGrid *extractedGrid = this->ExtractCellsForProcess(proc);

    if (!extractedGrid || (extractedGrid->GetNumberOfCells()==0)){
      continue;
    }

    yourNumCells[proc] =   extractedGrid->GetNumberOfCells();
    yourNumPoints[proc] =  extractedGrid->GetNumberOfPoints();

    if (proc != me){

      packedGrids[proc] = this->MarshallDataSet(extractedGrid, yourSizeData[proc]);
      extractedGrid->Delete();
    }
    else{

      mySubGrid = extractedGrid;
    }
  }

  // every process learns how many cells/points will be in it's new ugrid

  int TotalCells  = myNumCells[me]  = yourNumCells[me];
  int TotalPoints = myNumPoints[me] = yourNumPoints[me];

  int indata[3], outdata[3];

  int largestSizeData=0;

  for (offset = 1; offset < nnodes; offset++){
   
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

    if (mySizeData[source] > largestSizeData){
      largestSizeData = mySizeData[source]; 
    }
  }
  // initialize my new ugrid - use vtkMergeCells object which can merge
  //   in ugrids with same field arrays, filtering out duplicate points
  //   as it goes.

  vtkUnstructuredGrid *newGrid = this->GetOutput();

  vtkMergeCells *merged = vtkMergeCells::New();

  merged->SetTotalCells(TotalCells);
  merged->SetTotalPoints(TotalPoints);
  merged->SetUnstructuredGrid(newGrid);

  if (this->GlobalIdArrayName){
    merged->SetGlobalIdArrayName(this->GlobalIdArrayName);
  }

  if (mySubGrid){
    merged->MergeDataSet(mySubGrid);
    mySubGrid->Delete();
  }

  // every process sends it's ugrid contribution to every other

  char *buf = new char [largestSizeData];

  if (!buf){
  }

  for (offset = 1; offset < nnodes; offset++){
   
    target = (me + offset) % nnodes;
    source = (me + nnodes - offset) % nnodes;


    // post receive from source

    vtkMPICommunicator::Request req;

    if (mySizeData[source] > 0){

      mpiContr->NoBlockReceive(buf, mySizeData[source], source, 0x02, req);
    }

    // send to target

    if (packedGrids[target]){

      mpiContr->Send(packedGrids[target], yourSizeData[target], target, 0x02);
      delete [] packedGrids[target];
    }

    // await info from source

    if (mySizeData[source] > 0){

      req.Wait();

      vtkUnstructuredGrid *remoteGrid = this->UnMarshallDataSet(buf, mySizeData[source]);

      merged->MergeDataSet(remoteGrid);

      remoteGrid->Delete();
    }
  }

  delete [] buf;

  delete [] packedGrids;

  delete [] yourNumCells;
  delete [] yourNumPoints;
  delete [] yourSizeData;
  delete [] myNumCells;
  delete [] myNumPoints;
  delete [] mySizeData;

  merged->Finish();
  merged->Delete();

  return 0;
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
int vtkDistributedDataFilter::GenericRedistribute()
{
  for (int proc = 0; proc < this->NumProcesses; proc++){

    vtkUnstructuredGrid *ugrid = this->ExtractCellsForProcess(proc);

    if (ugrid == NULL) continue;   // process is assigned no regions

    // Fan in and merge ugrids *************************************
    // If I am "proc", output gets written with the result *********
    // This call also deletes ugrid at the earliest opportunity ****

    int fail = this->ReduceUgridMerge(ugrid, proc);

    if (fail) return 1;
  }

  return 0;
}
vtkUnstructuredGrid *vtkDistributedDataFilter::ExtractCellsForProcess(int proc)
{
    vtkIdList *regions = vtkIdList::New();

    int nregions = this->Kdtree->GetRegionAssignmentList(proc, regions);

    if (nregions == 0) return NULL;

    // Get list of the IDs of my cells that are in these regions. ***
    // This is why I specified RetainCellLocations before. **********

    this->Kdtree->CreateCellList(regions->GetPointer(0), nregions);

    // Create a new ugrid composed of these cells *******************

    vtkExtractCells *extCells = vtkExtractCells::New();

    // Copy input so the update call does not change the actual input.
    vtkDataSet* input = this->GetInput();
    vtkDataSet* tmp = input->NewInstance();
    tmp->ShallowCopy(input);
    extCells->SetInput(tmp);
    tmp->Delete();

    for (int reg=0; reg < nregions; reg++){

      extCells->AddCellList(this->Kdtree->GetCellList(regions->GetId(reg)));
    }

    this->Kdtree->DeleteCellList();

    extCells->Update();

    vtkUnstructuredGrid *ugrid = extCells->GetOutput();

    ugrid->Register(this);

    extCells->Delete();

    regions->Delete();

    return ugrid;
}


// Logarithmic fan-in of only the processes holding data for
// these regions.  Root of fan-in is the process assigned to
// the regions.

int vtkDistributedDataFilter::ReduceUgridMerge(
                                  vtkUnstructuredGrid *ugrid, int root)
{
  int i, ii;
  vtkUnstructuredGrid *newGrid;

  int iHaveData = (ugrid->GetNumberOfCells() > 0);
  int iAmRoot   = (root == this->MyLocalId);

  if (!iHaveData && !iAmRoot){
    ugrid->Delete();
    return 0;
  }

  if (iAmRoot){
    newGrid = this->GetOutput();
  }
  else{
    newGrid = vtkUnstructuredGrid::New();
  }

  // get list of participants

  int nAllProcs = this->NumProcesses;
  
  int *haveData = new int [nAllProcs];
  memset(haveData, 0, sizeof(int) * nAllProcs);

  vtkIdList *Ids = vtkIdList::New();

  vtkIdList *regions = vtkIdList::New();

  int nregions = this->Kdtree->GetRegionAssignmentList(root, regions);
  
  for (int reg=0; reg < nregions; reg++){

    // Get list of all processes that have data for this region

    Ids->Initialize();
    int nIds = this->Kdtree->GetProcessListForRegion(regions->GetId(reg), Ids);

    for (int p=0; p<nIds; p++){
      haveData[Ids->GetId(p)] = 1;
    } 
  } 
  regions->Delete();

  Ids->Delete();

  int nParticipants = 0;

  haveData[root] = 1;

  for (i=0; i<nAllProcs; i++){
    if (haveData[i]){
      nParticipants++;
    }
  }

  if (nParticipants == 1){

    newGrid->ShallowCopy(ugrid);
    ugrid->Delete();

    return 0;
  }

  int *member = new int [nParticipants];
  int myLocalRank;

  member[0] = root;

  if (iAmRoot) myLocalRank = 0;

  for (i=0, ii=1; i<nAllProcs; i++){
    if (haveData[i] && (i != root)){

       if (i == this->MyLocalId){
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

  for (i=0; i<nsources; i++){

    this->Controller->Receive(data, 2, source[i], tag);

    TotalPoints += data[0];
    TotalCells += data[1];
  }

  vtkMergeCells *merged = vtkMergeCells::New();

  merged->SetTotalCells(TotalCells);
  merged->SetTotalPoints(TotalPoints);
  merged->SetUnstructuredGrid(newGrid);

  if (this->GlobalIdArrayName){

    // merged Ugrid will filter out duplicate points

    merged->SetGlobalIdArrayName(this->GlobalIdArrayName);
  }

  if (iHaveData) merged->MergeDataSet(ugrid);

  ugrid->Delete();

  for (i=0; i<nsources; i++){

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

  if (ntargets > 0){

    data[0] = newGrid->GetNumberOfPoints();
    data[1] = newGrid->GetNumberOfCells();

    this->Controller->Send(data, 2, target, tag);

    this->Controller->Receive(&OKToSend, 1, target, tag);

    this->Controller->Send(static_cast<vtkDataObject *>(newGrid),
                           target, tag);

    newGrid->Delete();
  }

  return 0;
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

  for (int i = 1; i < nParticipants; i <<= 1){

    int other = myLocalRank ^ i;

    if (other >= nParticipants) continue;

    if (myLocalRank > other){
      fanInTo = member[other];

      nTo++;   /* one at most */

      break;
    }
    else{
      fanInFrom[nFrom++] = member[other];
    }
  }

  *source = fanInFrom;
  *target = fanInTo;
  *nsources = nFrom;
  *ntargets = nTo;

  return;
}
void vtkDistributedDataFilter::PrintTiming(ostream& os, vtkIndent indent)
{
  vtkTimerLog::DumpLogWithIndents(&os, (float)0.0);
}
void vtkDistributedDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{  
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Kdtree: " << this->Kdtree << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "GlobalIdArrayName: " << this->GlobalIdArrayName << endl;
  os << indent << "RetainKdtree: " << this->RetainKdtree << endl;
  os << indent << "NumProcesses: " << this->NumProcesses << endl;
  os << indent << "MyLocalId: " << this->MyLocalId << endl;
  os << indent << "Timing: " << this->Timing << endl;
  os << indent << "TimerLog: " << this->TimerLog << endl;
}
