// -*- c++ -*-

/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkPKdTree.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
=========================================================================*/

#include <vtkPKdTree.h>
#include <vtkDataSet.h>
#include <vtkCellCenters.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataArray.h>
#include <vtkCellData.h>
//#include <unistd.h>
#include <queue>
#include <algorithm>

vtkCxxRevisionMacro(vtkPKdTree, "1.1.2.1");
vtkStandardNewMacro(vtkPKdTree);

const int vtkPKdTree::NoRegionAssignment = 0;   // default
const int vtkPKdTree::ContiguousAssignment = 1; // default if RegionAssignmentOn
const int vtkPKdTree::UserDefinedAssignment = 2;
const int vtkPKdTree::RoundRobinAssignment  = 3;
const int vtkPKdTree::MinimizeDataMovementAssignment = 4;

#define FreeList(list)   if (list) {delete [] list; list = NULL;}
#define FreeItem(item)   if (item) {delete item; item = NULL;}

#define TIMER(s) if (this->Timing){ this->TimerLog->MarkStartEvent(s); }
#define TIMERDONE(s) if (this->Timing){ this->TimerLog->MarkEndEvent(s); }

static char errstr[256];

#define VTKERROR(s) \
{                   \
  sprintf(errstr,"(process %d) %s",this->MyId,s); \
  vtkErrorMacro(<< errstr);                       \
}
#define VTKWARNING(s) \
{                   \
  sprintf(errstr,"(process %d) %s",this->MyId,s); \
  vtkWarningMacro(<< errstr);                       \
}

vtkPKdTree::vtkPKdTree()
{
  this->NumRegionsOrLess = 0;
  this->NumRegionsOrMore = 0;
  this->GhostLevel = 0;
  this->BlockData  = 0;
  this->ProcessCellCountData  = 0;
  this->RegionAssignment = NoRegionAssignment;

  this->Controller = NULL;
  this->SubGroup   = NULL;

  this->NumProcesses = 0;
  this->MyId         = -1;

  this->InitializeGlobalIndexLists();
  this->InitializeRegionAssignmentLists();
  this->InitializeProcessDataLists();

  this->TotalNumCells = 0;

  this->PtArray = NULL;
  this->PtArray2 = NULL;
  this->CurrentPtArray = NULL;
  this->NextPtArray = NULL;

  this->SelectBuffer = NULL;
}
vtkPKdTree::~vtkPKdTree()
{
  this->SetController(NULL);
  this->FreeSelectBuffer();
  this->FreeDoubleBuffer();

  this->FreeGlobalIndexLists();
  this->FreeRegionAssignmentLists();
  this->FreeProcessDataLists();
}
void vtkPKdTree::SetController(vtkMultiProcessController *c)
{
  if (this->Controller == c) 
    {
    return;
    }
  this->Modified();
  if (this->Controller != NULL)
    {
    this->Controller->UnRegister(this);
    this->Controller == NULL;
    }
  if (c == NULL)
    {
    return;
    }

  this->NumProcesses = c->GetNumberOfProcesses();

  this->Controller = c;
  this->MyId = c->GetLocalProcessId();
  c->Register(this);

}

//--------------------------------------------------------------------
// Parallel k-d tree build, Floyd and Rivest (1975) select algorithm
// for median finding.
//--------------------------------------------------------------------

int vtkPKdTree::AllCheckForFailure(int rc, char *where, char *how)
{
  int vote;
  char errmsg[256];

  this->SubGroup->ReduceSum(&rc, &vote, 1, 0);
  this->SubGroup->Broadcast(&vote, 1, 0);

  if (vote){

    if (rc){
      sprintf(errmsg,"%s on my node (%s)",how, where);
    }
    else{
      sprintf(errmsg,"%s on a remote node (%s)",how, where);
    }
    VTKWARNING(errmsg);

    return 1;
  }
  return 0;
}

void vtkPKdTree::AllCheckParameters()
{
  int param[10];
  int useparam[10];

  // All the parameters that determine how k-d tree is built and
  //  what tables get created afterward - there's no point in
  //  trying to build unless these match on all processes.

  param[0] = this->ValidDirections;
  param[1] = this->MaxLevel;
  param[2] = this->MinCells;
  param[3] = this->NumRegionsOrLess;
  param[4] = this->NumRegionsOrMore;
  param[5] = this->GhostLevel;
  param[6] = this->BlockData;
  param[7] = this->ProcessCellCountData;
  param[8] = this->RegionAssignment;
  param[9] = 0;

  if (this->MyId == 0){
    this->SubGroup->Broadcast(param, 10, 0);
    return;
  }

  this->SubGroup->Broadcast(useparam, 10, 0);

  int diff = 0;

  for (int i=0; i < 10; i++){
    if (useparam[i] != param[i]){
      diff = 1;
      break;
    }
  }
  if (diff){

    VTKWARNING("Changing my runtime parameters to match process 0");

    this->ValidDirections  = useparam[0];
    this->MaxLevel         = useparam[1];
    this->MinCells         = useparam[2];
    this->NumRegionsOrLess = useparam[3];
    this->NumRegionsOrMore = useparam[4];
    this->GhostLevel       = useparam[5];
    this->BlockData        = useparam[6];
    this->ProcessCellCountData   = useparam[7];
    this->RegionAssignment = useparam[8];
  }

  return;
}

#define BoundsToMinMax(bounds,min,max) \
{                                      \
  min[0] = bounds[0]; min[1] = bounds[2]; min[2] = bounds[4]; \
  max[0] = bounds[1]; max[1] = bounds[3]; max[2] = bounds[5]; \
}
#define MinMaxToBounds(bounds,min,max) \
{                                      \
  bounds[0] = min[0]; bounds[2] = min[1]; bounds[4] = min[2]; \
  bounds[1] = max[0]; bounds[3] = max[1]; bounds[5] = max[2]; \
}
#define BoundsToMinMaxUpdate(bounds,min,max) \
{                                            \
  min[0] = (bounds[0] < min[0] ? bounds[0] : min[0]); \
  min[1] = (bounds[2] < min[1] ? bounds[2] : min[1]); \
  min[2] = (bounds[4] < min[2] ? bounds[4] : min[2]); \
  max[0] = (bounds[1] > max[0] ? bounds[1] : max[0]); \
  max[1] = (bounds[3] > max[1] ? bounds[3] : max[1]); \
  max[2] = (bounds[5] > max[2] ? bounds[5] : max[2]); \
}

float *vtkPKdTree::VolumeBounds()
{
  int i;

  // Get the spatial bounds of the whole volume
  
  float *volBounds = new float [6];
  float localMin[3], localMax[3], globalMin[3], globalMax[3];

  for (i=0; i<this->NumDataSets; i++){
  
    this->DataSets[i]->GetBounds(volBounds);
  
    if (i==0){
      BoundsToMinMax(volBounds, localMin, localMax);
    }
    else{
      BoundsToMinMaxUpdate(volBounds, localMin, localMax);
    }
  }

  this->SubGroup->ReduceMin(localMin, globalMin, 3, 0);
  this->SubGroup->Broadcast(globalMin, 3, 0);

  this->SubGroup->ReduceMax(localMax, globalMax, 3, 0);
  this->SubGroup->Broadcast(globalMax, 3, 0);

  MinMaxToBounds(volBounds, globalMin, globalMax);

  // push out a little if flat

  float diff[3], aLittle = 0.0;

  for (i=0; i<3; i++){
     diff[i] = volBounds[2*i+1] - volBounds[2*i];
     aLittle = (diff[i] > aLittle) ? diff[i] : aLittle;
  }
  if ((aLittle /= 100.0) <= 0.0){
     VTKERROR("VolumeBounds - degenerate volume");
     return NULL;
  }
  for (i=0; i<3; i++){
     if (diff[i] <= 0){
        volBounds[2*i]   -= aLittle;
        volBounds[2*i+1] += aLittle;
     }
  }

  return volBounds;
}


// BuildLocator must be called by all processes in the parallel application

void vtkPKdTree::BuildLocator()
{
  int i, fail;
  unsigned int maxTime=0;
  int rebuildLocator = 1;
  int totalPts=0;
  float *volBounds = NULL;
  int deleteFlag = 0;

  // parallel environment

  if (this->Controller == NULL){
    VTKERROR("BuildLocator - must SetController first");
    return;
  }

  this->SubGroup = new vtkSubGroup(0, this->NumProcesses-1,
             this->MyId, 0x00001000, this->Controller->GetCommunicator());

  // do I need to rebuild

  for (i=0; i<this->NumDataSets; i++){
    maxTime = (this->DataSets[i]->GetMTime() > maxTime) ?
               this->DataSets[i]->GetMTime() : maxTime;
  }

  if ((this->Top != NULL) && (this->BuildTime > this->MTime) &&
       (this->BuildTime > maxTime))
  { 
    rebuildLocator = 0;
  } 

  // If anyone needs to rebuild, we all need to rebuild

  int vote;
  this->SubGroup->ReduceSum(&rebuildLocator, &vote, 1, 0);
  this->SubGroup->Broadcast(&vote, 1, 0);

  if (vote == 0){
    FreeItem(this->SubGroup);
    return;
  }

  vtkDebugMacro( << "Creating Kdtree in parallel" );

  if (this->Timing){
    if (this->TimerLog == NULL) this->TimerLog = vtkTimerLog::New();
  }

  if (this->BlockData) this->ProcessCellCountData = 1;

  this->AllCheckParameters();

  this->FreeSearchStructure();

  this->ReleaseTables();

  // Locally, create a single list of the coordinates of the centers of the 
  //   cells of my data sets 

  //TIMER("Compute cell centers");

  this->PtArray = NULL;

  char *errString = "";

  fail = this->ComputeCellCenters();

  //TIMERDONE("Compute cell centers");

  if (!fail){
    this->PtArray  = this->GetCellCenters();
    this->CurrentPtArray = this->PtArray;
    totalPts = this->GetNumberOfCellCenters();

    if (totalPts == 0){
      errString = "no cells";
    }
  }
  else{
    errString = "memory allocation";
  }

  if (this->AllCheckForFailure(fail, "BuildLocator", errString)){

    goto doneError; 
  }

  // Get total number of cells across all processes, assign global indices
  //   for select operation

  //TIMER("Build index lists (global op)");

  fail = this->BuildGlobalIndexLists(totalPts);

  //TIMERDONE("Build index lists (global op)");

  if (fail){
    goto doneError;
  }

  // Get the bounds of the entire volume

  //TIMER("Compute volume bounds (global op)");

  volBounds = this->VolumeBounds();

  //TIMERDONE("Compute volume bounds (global op)");

  if (volBounds == NULL){

    goto doneError; 
  }

  // In parallel, build the k-d tree structure, partitioning all
  //   the points into spatial regions.  Sub-groups of processors
  //   will form vtkSubGroups to divide sub-regions of space.

  FreeItem(this->SubGroup);

  //TIMER("Compute tree (global op)");

  fail = this->BreadthFirstDivide(volBounds);

  //TIMERDONE("Compute tree (global op)");

  // no longer valid, we overwrote them during k-d tree parallel build
  this->FreeCellCenters();  

  this->SubGroup = new vtkSubGroup(0, this->NumProcesses-1,
             this->MyId, 0x00002000, this->Controller->GetCommunicator());

  if (this->AllCheckForFailure(fail, "BreadthFirstDivide", "memory allocation")){

    goto doneError;
  }
  // I only have a partial tree at this point, the regions in which
  //   I participated.  Now collect the entire tree.


  this->SubGroup = new vtkSubGroup(0, this->NumProcesses-1,
             this->MyId, 0x00003000, this->Controller->GetCommunicator());

  //TIMER("Complete tree (global op)");

  fail = this->CompleteTree();
  //TIMERDONE("Complete tree (global op)");

  if (fail){
    goto doneError;
  }
  // Build tables indicating for each member, how much data they
  //   have for each of the spatial regions.  If block level data
  //   is required, count cells for each block for each region as well.

  if (this->RegionAssignment == vtkPKdTree::MinimizeDataMovementAssignment){

    // I have to know who has what data to do this

    if (!this->ProcessCellCountData){
      deleteFlag = 1;
      this->ProcessCellCountDataOn();
    }
  }

  if (this->ProcessCellCountData || this->GetRetainCellLocations()){

    // Recompute the cell centers of our input data.  They were
    // over-written with other process' data during k-d tree
    // build

    //TIMER("Re-compute cell centers");

    fail = this->ComputeCellCenters();
    this->PtArray  = this->GetCellCenters();
    this->CurrentPtArray = this->PtArray;

    //TIMERDONE("Re-compute cell centers");

    if (fail){
      goto doneError;
    }
  }

  if (this->ProcessCellCountData){

    //TIMER("Compute tables (global op)");

    fail = this->BuildRegionProcessTables();
    //TIMERDONE("Compute tables (global op)");

    if (fail){
      goto doneError;
    }
  }

  if (this->GetRetainCellLocations() == 0){
    this->FreeCellCenters();
  }

  // Partition the regions among the processes

  if (this->RegionAssignment != vtkPKdTree::NoRegionAssignment){

    //TIMER("Assign spatial regions to processors");

    fail = this->UpdateRegionAssignment();

    //TIMERDONE("Assign spatial regions to processors");

    if (this->AllCheckForFailure(fail, "UpdateRegionAssignment", 
                                "memory allocation")){

      goto doneError;
    }

  }

  if (deleteFlag) this->FreeProcessDataLists();

  goto done;

doneError:

  this->FreeRegionAssignmentLists();
  this->FreeProcessDataLists();
  this->FreeSearchStructure();
  this->FreeCellCenters();

done:

  FreeItem(this->SubGroup);
  if (volBounds)
    {
    FreeList(volBounds);
    }
  this->FreeGlobalIndexLists();

  this->BuildTime.Modified();

  return;
}

typedef struct _vtkNodeInfo{
  vtkKdNode *kd;
  int L;
  int level;
  int tag;
} *vtkNodeInfo;

#define ENQUEUE(a, b, c, d)  \
{                            \
  vtkNodeInfo rec = new struct _vtkNodeInfo; \
  rec->kd = a; \
  rec->L = b; \
  rec->level = c; \
  rec->tag = d; \
  Queue.push(rec); \
}

int vtkPKdTree::BreadthFirstDivide(float *volBounds)
{
  int returnVal = 0;

  vtkstd::queue <vtkNodeInfo> Queue;

  if (this->AllocateDoubleBuffer()){
    VTKERROR("memory allocation for double buffering");  
    return 1;
  }

  if (this->AllocateSelectBuffer()){

    this->FreeDoubleBuffer();

    VTKERROR("memory allocation for select buffers");
    return 1;
  }

  vtkKdNode *kd = this->Top = new vtkKdNode();

  kd->SetBounds((double)volBounds[0], (double)volBounds[1],
                (double)volBounds[2], (double)volBounds[3],
                (double)volBounds[4], (double)volBounds[5]);

  kd->SetNumberOfCells(this->TotalNumCells);

  kd->SetDataBounds((double)volBounds[0], (double)volBounds[1],
                (double)volBounds[2], (double)volBounds[3],
                (double)volBounds[4], (double)volBounds[5]);

  int midpt = this->DivideRegion(kd, 0, 0, 0x00000001);

  if (midpt > 0){

    ENQUEUE(kd->Left, 0, 1, 0x00000002);

    ENQUEUE(kd->Right, midpt, 1, 0x00000003);
  }
  else if (midpt < 0){

    this->FreeSelectBuffer();
    this->FreeDoubleBuffer();

    return 1;
  }

  while (!Queue.empty()){

    vtkNodeInfo info = Queue.front();
    Queue.pop();

    vtkKdNode *kd = info->kd;
    int L = info->L;
    int level = info->level;
    int tag   = info->tag;

    int midpt = this->DivideRegion(kd, L, level, tag);

    if (midpt > 0){

      ENQUEUE(kd->Left, L, level+1, tag << 1);

      ENQUEUE(kd->Right, midpt, level+1, (tag << 1) | 1);
    }
    else if (midpt < 0){

      returnVal = 1;  // have to keep going, or remote ops may hang

    }

    delete info;
  }

  this->FreeSelectBuffer();

  if (this->CurrentPtArray == this->PtArray2){
    memcpy(this->PtArray, this->PtArray2, this->PtArraySize * sizeof(float));
  }

  this->FreeDoubleBuffer();

  return returnVal;
}
int vtkPKdTree::DivideTest(int L, int R, int level)
{
  if (level == this->MaxLevel) return 0;

  int minCells = this->GetMinCells();
  int numCells   = R - L + 1;

  if ((numCells < 2) || (minCells && (minCells > (numCells/2)))) return 0;

  int nRegionsNow  = 1 << level;
  int nRegionsNext = nRegionsNow << 1;

  if (this->NumRegionsOrLess && (nRegionsNext > this->NumRegionsOrLess)) return 0;
  if (this->NumRegionsOrMore && (nRegionsNow > this->NumRegionsOrMore)) return 0;

  return 1;
}
int vtkPKdTree::DivideRegion(vtkKdNode *kd, int L, int level, int tag)
{
  int R = L + kd->GetNumberOfCells() - 1;

  if (!this->DivideTest(L, R, level)) return 0;

  int p1 = this->WhoHas(L);
  int p2 = this->WhoHas(R);

  if ((this->MyId < p1) || (this->MyId > p2)) return 0;

  this->SubGroup = new vtkSubGroup(p1, p2, this->MyId, tag, 
              this->Controller->GetCommunicator());

  int maxdim = this->SelectCutDirection(kd);

  kd->SetDim(maxdim);

  int midpt = this->Select(maxdim, L, R);

  if (midpt < L + 1){

    // couldn't divide along maxdim - all points we're at same location
    // should probably try a different direction

    FreeItem(this->SubGroup);
    return 0;
  }

  float *newDataBounds = this->DataBounds(L, midpt, R);

  if (newDataBounds == NULL){
    FreeItem(this->SubGroup);
    return -1;
  }

  vtkKdNode *left = new vtkKdNode();
  vtkKdNode *right = new vtkKdNode();

  int fail = ( (left == NULL) || (right == NULL) );

  if (this->AllCheckForFailure(fail, "Divide Region", "memory allocation")){

    FreeList(newDataBounds);
    FreeItem(left);
    FreeItem(right);
    FreeItem(this->SubGroup);
    return -1;
  }

  double coord = (newDataBounds[maxdim*2 + 1] +   // max on left side
                 newDataBounds[6 + maxdim*2] )*   // min on right side
                  0.5;

  kd->AddChildNodes(left, right);

  double bounds[6];
  kd->GetBounds(bounds);

  left->SetBounds(
     bounds[0], ((maxdim == xdim) ? coord : bounds[1]),
     bounds[2], ((maxdim == ydim) ? coord : bounds[3]),
     bounds[4], ((maxdim == zdim) ? coord : bounds[5]));

  left->SetNumberOfCells(midpt - L);

  right->SetBounds(
     ((maxdim == xdim) ? coord : bounds[0]), bounds[1],
     ((maxdim == ydim) ? coord : bounds[2]), bounds[3],
     ((maxdim == zdim) ? coord : bounds[4]), bounds[5]);

  right->SetNumberOfCells(R - midpt + 1);

  left->SetDataBounds(newDataBounds[0], newDataBounds[1], 
                      newDataBounds[2], newDataBounds[3],
                      newDataBounds[4], newDataBounds[5]);

  right->SetDataBounds(newDataBounds[6], newDataBounds[7], 
                      newDataBounds[8], newDataBounds[9],
                      newDataBounds[10], newDataBounds[11]);

  delete [] newDataBounds;

  FreeItem(this->SubGroup);

  return midpt;
}

void vtkPKdTree::ExchangeVals(int pos1, int pos2)
{
  vtkCommunicator *comm = this->Controller->GetCommunicator();

  float *myval; 
  float otherval[3];

  int player1 = this->WhoHas(pos1);
  int player2 = this->WhoHas(pos2);

  if ((player1 == this->MyId) && (player2 == this->MyId)){

    this->ExchangeLocalVals(pos1, pos2);
  }

  else if (player1 == this->MyId){

    myval = this->GetLocalVal(pos1);

    comm->Send(myval, 3, player2, this->SubGroup->tag);

    comm->Receive(otherval, 3, player2, this->SubGroup->tag);

    this->SetLocalVal(pos1, otherval);
  }
  else if (player2 == this->MyId){

    myval = this->GetLocalVal(pos2);

    comm->Receive(otherval, 3, player1, this->SubGroup->tag);

    comm->Send(myval, 3, player1, this->SubGroup->tag);

    this->SetLocalVal(pos2, otherval);
  }
  return;
}

// Given an array X with element indices ranging from L to R, and
// a K such that L <= K <= R, rearrange the elements such that
// X[K] contains the ith sorted element,  where i = K - L + 1, and 
// all the elements X[j], j < k satisfy X[j] <= X[K], and all the
// elements X[j], j > k satisfy X[j] >= X[K].

#define sign(x) ((x<0) ? (-1) : (1))
#ifndef max
#define max(x,y) ((x>y) ? (x) : (y))
#endif
#ifndef min 
#define min(x,y) ((x<y) ? (x) : (y))
#endif

void vtkPKdTree::_select(int L, int R, int K, int dim)
{
  int N, I, J, S, SD, LL, RR;
  float Z;

  while (R > L){

    if ( R - L > 600){

      // "Recurse on a sample of size S to get an estimate for the
      // (K-L+1)-th smallest element into X[K], biased slightly so
      // that the (K-L+1)-th element is expected to lie in the
      // smaller set after partitioning"

      N = R - L + 1;
      I = K - L + 1;
      Z = log(N); 
      S = static_cast<int>(.5 * exp(2*Z/3));
      SD = static_cast<int>(.5 * sqrt(Z*S*(N-S)/N) * sign(1 - N/2));
      LL = max(L, K - (I*S/N) + SD);
      RR = min(R, K + (N-1) * S/N + SD);
      this->_select(LL, RR, K, dim);
    }

    int p1 = this->WhoHas(L);
    int p2 = this->WhoHas(R);

    // Processes p1 through p2 will rearrange array elements L through R
    // so they are partitioned by the value at K.  The value at K will
    // appear in array element J, all values less than X[K] will appear
    // between L and J-1, all values greater or equal to X[K] will appear
    // between J+1 and R.

    J = this->PartitionSubArray(L, R, K, dim, p1, p2);

    // "now adjust L,R so they surround the subset containing
    // the (K-L+1)-th smallest element"

    if (J <= K) L = J + 1;
    if (K <= J) R = J - 1;
  }
}
int vtkPKdTree::Select(int dim, int L, int R)
{
  int K = ((R + L) / 2) + 1;

  this->_select(L, R, K, dim);

  if (K == L) return K;

  // The global array is now re-ordered, partitioned around X[K]. 
  // (In particular, for all i, i<K, X[i] <= X[K] and for all i,
  // i > K, X[i] >= X[K].)
  // However the value at X[K] may occur more than once, and by
  // construction of the reordered array, there is a J <= K such that
  // for all i < J, X[i] < X[K] and for all J <= i < K X[i] = X[K].
  //
  // We want to roll K back to this value J, so that all points are
  // unambiguously assigned to one region or the other.

  int hasK = this->WhoHas(K);  
  int hasKrank = this->SubGroup->getLocalRank(hasK);

  int hasKleft = this->WhoHas(K-1);
  int hasKleftrank = this->SubGroup->getLocalRank(hasKleft);

  float Kval;
  float Kleftval;
  float *pt;

  if (hasK == this->MyId){
    pt = this->GetLocalVal(K) + dim;
    Kval = *pt;
  }

  this->SubGroup->Broadcast(&Kval, 1, hasKrank);

  if (hasKleft == this->MyId){
    pt = this->GetLocalVal(K-1) + dim;
    Kleftval = *pt;
  }

  this->SubGroup->Broadcast(&Kleftval, 1, hasKleftrank);

  if (Kleftval != Kval) return K;

  int firstKval = this->TotalNumCells;  // greater than any valid index

  if (this->MyId <= hasKleft){

    int start = this->EndVal[this->MyId];
    if (start > K-1) start = K-1;

    pt = this->GetLocalVal(start) + dim;

    if (*pt == Kval){

      firstKval = start;
    
      int finish = this->StartVal[this->MyId];

      for (int idx=start-1; idx >= finish; idx--){

        pt -= 3;
        if (*pt < Kval) break;

      firstKval--;
      }
    }
  }

  int newK;

  this->SubGroup->ReduceMin(&firstKval, &newK, 1, hasKrank);
  this->SubGroup->Broadcast(&newK, 1, hasKrank);

  return newK;
}

int vtkPKdTree::_whoHas(int L, int R, int pos)
{
  if (L == R) return L;

  int M = (L + R) >> 1;

  if ( pos < this->StartVal[M]){

    return _whoHas(L, M-1, pos);
  }
  else if (pos < this->StartVal[M+1]){

    return M;
  }
  else{
    return _whoHas(M+1, R, pos);
  }
}
int vtkPKdTree::WhoHas(int pos)
{
  if ( (pos < 0) || (pos >= this->TotalNumCells)){
    return -1;
  }
  return _whoHas(0, this->NumProcesses-1, pos);
}
float *vtkPKdTree::GetLocalVal(int pos)
{
  if ( (pos < this->StartVal[this->MyId]) || (pos > this->EndVal[this->MyId])){
    return NULL;
  }
  int localPos = pos - this->StartVal[this->MyId];

  return this->CurrentPtArray + (3*localPos);
}
float *vtkPKdTree::GetLocalValNext(int pos)
{
  if ( (pos < this->StartVal[this->MyId]) || (pos > this->EndVal[this->MyId])){
    return NULL;
  }
  int localPos = pos - this->StartVal[this->MyId];

  return this->NextPtArray + (3*localPos);
}
void vtkPKdTree::SetLocalVal(int pos, float *val)
{
  if ( (pos < this->StartVal[this->MyId]) || (pos > this->EndVal[this->MyId])){
    VTKERROR("SetLocalVal - bad index");
    return;
  }

  int localOffset = (pos - this->StartVal[this->MyId]) * 3;

  this->CurrentPtArray[localOffset]   = val[0];
  this->CurrentPtArray[localOffset+1] = val[1];
  this->CurrentPtArray[localOffset+2] = val[2];

  return;
}
void vtkPKdTree::ExchangeLocalVals(int pos1, int pos2)
{
  float temp[3];

  float *pt1 = this->GetLocalVal(pos1);
  float *pt2 = this->GetLocalVal(pos2);

  if (!pt1 || !pt2){
    VTKERROR("ExchangeLocalVal - bad index");
    return;
  }

  temp[0] = pt1[0];
  temp[1] = pt1[1];
  temp[2] = pt1[2];

  pt1[0] = pt2[0];
  pt1[1] = pt2[1];
  pt1[2] = pt2[2];

  pt2[0] = temp[0];
  pt2[1] = temp[1];
  pt2[2] = temp[2];

  return;
}


// Global array [L:R] spans the contiguous processes p1 through p2.  In
// parallel, rearrange the array interval [L:R] so that there is a J
// satisfying all elements in [L:J-1] are < T, element J is T, and all
// elements [J+1:R] are >= T.

int vtkPKdTree::PartitionSubArray(int L, int R, int K, int dim, int p1, int p2)
{
  int TLocation;

  int rootrank = this->SubGroup->getLocalRank(p1);

  int me     = this->MyId;

  if ( (me < p1) || (me > p2)){

    this->SubGroup->Broadcast(&TLocation, 1, rootrank);

    return TLocation;
  }

  if (p1 == p2){

    TLocation = this->PartitionAboutMyValue(L, R, K, dim);

    this->SubGroup->Broadcast(&TLocation, 1, rootrank);

    return TLocation;
  }

  // Each process will rearrange their subarray into a left region of values
  // less than X[K] and a right region of values greater or equal to X[K].
  // J will be the index of the first value greater or equal to X[K].  If
  // all values are less, J will be the one index past the last element.
  // In the case of the process holding the Kth array value, X[K] will be
  // found at location J.

  int tag = this->SubGroup->tag;

  vtkSubGroup *sg = new vtkSubGroup(p1, p2, me, tag,
                      this->Controller->GetCommunicator());

  int hasK   = this->WhoHas(K);

  int Krank    = sg->getLocalRank(hasK);
  int myrank   = sg->getLocalRank(me);

  int myL = this->StartVal[me];
  int myR = this->EndVal[me];

  if (myL < L) myL = L;
  if (myR > R) myR = R;

  // Get Kth element
    
  float T;

  if (hasK == me){
    T = this->GetLocalVal(K)[dim];

  }

  sg->Broadcast(&T, 1, Krank);

  int J;   // dividing point in rearranged sub array
    
  if (hasK == me){
    J = this->PartitionAboutMyValue(myL, myR, K, dim);
  }
  else{
    J = this->PartitionAboutOtherValue(myL, myR, T, dim);
  }


  // Now the ugly part.  The processes redistribute the array so that
  // globally the interval [L:R] is partitioned by the value formerly
  // at X[K].

  int nprocs = p2 - p1 + 1;

  int *buf  = this->SelectBuffer;

  int *left       = buf; buf += nprocs; // global index of my leftmost
  int *right      = buf; buf += nprocs; // global index of my rightmost
  int *Jval       = buf; buf += nprocs; // global index of my first val >= T

  int *leftArray  = buf; buf += nprocs; // number of my vals < T
  int *leftUsed   = buf; buf += nprocs; // how many scheduled to be sent so far

  int *rightArray = buf; buf += nprocs; // number of my vals >= T
  int *rightUsed  = buf; buf += nprocs; // how many scheduled to be sent so far


  rootrank = sg->getLocalRank(p1);

  sg->Gather(&myL, left, 1, rootrank);
  sg->Broadcast(left, nprocs, rootrank);
  
  sg->Gather(&myR, right, 1, rootrank);
  sg->Broadcast(right, nprocs, rootrank); 

  sg->Gather(&J, Jval, 1, rootrank);
  sg->Broadcast(Jval, nprocs, rootrank);


  delete sg;

  int leftRemaining = 0;

  int p, sndr, recvr;

  for (p = 0; p < nprocs; p++){

    leftArray[p]  = Jval[p] - left[p];
    rightArray[p] = right[p] - Jval[p] + 1;

    leftRemaining += leftArray[p];
                      
    leftUsed[p] = 0;
    rightUsed[p] = 0;
  }

  int nextLeftProc = 0;
  int nextRightProc = 0;

  int need, have, take;

  int FirstRightArrayElementLocation;
  int FirstRight = 1;

  if ( (myL > this->StartVal[me]) || (myR < this->EndVal[me])){
    memcpy(this->NextPtArray, this->CurrentPtArray, this->PtArraySize * sizeof(float));
  }

  for (recvr = 0; recvr < nprocs; recvr++){

    need = leftArray[recvr] + rightArray[recvr];
    have = 0;

    if (leftRemaining >= 0){

      for (sndr = nextLeftProc; sndr < nprocs; sndr++){

        take = leftArray[sndr] - leftUsed[sndr];

        if (take == 0) continue;

        take = (take > need) ? need : take;

        this->DoTransfer(sndr + p1, recvr + p1, 
                         left[sndr] + leftUsed[sndr], left[recvr] + have, take);

        have += take;
        need -= take;

        leftUsed[sndr] += take;

        if (need == 0) break;
      }

      if (leftUsed[sndr] == leftArray[sndr]){
        nextLeftProc = sndr+1;
      }
      else{
        nextLeftProc = sndr;
      }

      leftRemaining -= have;
    }

    if (need == 0) continue;

    for (sndr = nextRightProc; sndr < nprocs; sndr++){
  
        take = rightArray[sndr] - rightUsed[sndr];

        if (take == 0) continue;
  
        take = (take > need) ? need : take;
  
        if ((sndr == Krank) && (rightUsed[sndr] == 0)){
  
          TLocation = left[recvr] + have;
        }

        if (FirstRight){

          FirstRightArrayElementLocation = left[recvr] + have;
    
          FirstRight = 0;
        }

        this->DoTransfer(sndr + p1, recvr + p1, 
                         left[sndr] + leftArray[sndr] + rightUsed[sndr], 
                         left[recvr] + have, take);

        have += take; 
        need -= take;
          
        rightUsed[sndr] += take;
          
        if (need == 0) break;
    }   

    if (rightUsed[sndr] == rightArray[sndr]){
      nextRightProc = sndr+1;
    }
    else{
      nextRightProc = sndr;
    }   
  }   

  this->SwitchDoubleBuffer();


  if (FirstRightArrayElementLocation != TLocation){

    this->ExchangeVals(FirstRightArrayElementLocation, TLocation);

    TLocation = FirstRightArrayElementLocation;
  }

  rootrank = this->SubGroup->getLocalRank(p1);

  this->SubGroup->Broadcast(&TLocation, 1, rootrank);

  return TLocation;
}

void vtkPKdTree::DoTransfer(int from, int to, int fromIndex, int toIndex, int count)
{
float *fromPt, *toPt;


  vtkCommunicator *comm = this->Controller->GetCommunicator();

  int nitems = count * 3;

  int me = this->MyId;

  int tag = this->SubGroup->tag;

  if ( (from==me) && (to==me)){

    fromPt = this->GetLocalVal(fromIndex);
    toPt = this->GetLocalValNext(toIndex);

    memcpy(toPt, fromPt, nitems * sizeof(float));
  }
  else if (from == me){

    fromPt = this->GetLocalVal(fromIndex);

    comm->Send(fromPt, nitems, to, tag);
  }
  else if (to == me){

    toPt = this->GetLocalValNext(toIndex);

    comm->Receive(toPt, nitems, from, tag);
  }

  return;
}

// Rearrange array elements [L:R] such that there is a J where all elements
// [L:J-1] are < T and all elements [J:R] are >= T.  If all elements are
// < T, let J = R+1.

int vtkPKdTree::PartitionAboutOtherValue(int L, int R, float T, int dim)
{
  float *pt, Lval, Rval;

  pt = this->GetLocalVal(L);
  Lval = pt[dim];

  pt = this->GetLocalVal(R);
  Rval = pt[dim];

  int I = L;
  int J = R;

  if ((Lval >= T) && (Rval >= T)){

    pt = this->GetLocalVal(J) + dim;

    while (J > I){
         J--;
         pt -= 3;
         if (*pt < T) break;
    }
  }
  else if ((Lval < T) && (Rval < T)){

    pt = this->GetLocalVal(I) + dim;

    while (I < J){
      I++;
      pt += 3;
      if (*pt >= T) break;
    }
  }
  else if ((Lval < T) && (Rval >= T)){

    this->ExchangeLocalVals(I, J);
  }
  else if ((Lval >= T) && (Rval < T)){

    // first loop will fix this
  }

  while (I < J){

    this->ExchangeLocalVals(I, J);

    pt = this->GetLocalVal(I) + dim;

    while (I < J){
      I++;
      pt += 3;
      if (*pt >= T) break;
    }

    pt = this->GetLocalVal(J) + dim;

    while (I < J){
      J--;
      pt -= 3;
      if (*pt < T) break;
    }
  }

  pt = this->GetLocalVal(R);

  if (pt[dim] < T) J = R + 1;

  return J;
} 

// My local array is [L:R] and L <= K <= R, and element K is T.  
// Rearrange the array so that there is a J satisfying all elements
// [L:J-1] are < T, all elements [J+1:R] >= T, and element J is T.

int vtkPKdTree::PartitionAboutMyValue(int L, int R, int K, int dim)
{ 
  float *pt;
  float T;
  int I, J;
  
  // Set up so after first exchange in the loop, we have either
  //   X[L] = T and X[R] >= T
  // or
  //   X[L] < T and X[R] = T
  //

  pt = this->GetLocalVal(K);
  
  T = pt[dim];
    
  this->ExchangeLocalVals(L, K);
    
  pt = this->GetLocalVal(R);
         
  if (pt[dim] >= T) this->ExchangeLocalVals(R, L);
    
  I = L;
  J = R;

  while (I < J){

    this->ExchangeLocalVals(I, J);

    pt = this->GetLocalVal(--J) + dim;
  
    while (J >= L){
  
      if (*pt < T) break;
    
      J--;
      pt -= 3;
    }
      
    pt = this->GetLocalVal(++I) + dim;
      
    while (I < J){
  
      if (*pt >= T) break;

      I++;
      pt += 3;
    }

  } 

  if (J < L){

    return L;    // X[L]=T , X[j] >=T for j > L
  }

  // J is location of the first value < T
      
  pt = this->GetLocalVal(L);
      
  float Lval = pt[dim];

  if (Lval == T){
    this->ExchangeLocalVals(L, J);
  } 
  else{
    this->ExchangeLocalVals(++J, R);
  }

  return J;
}

//--------------------------------------------------------------------
// Compute the bounds for the data in a region
//--------------------------------------------------------------------

void vtkPKdTree::GetLocalMinMax(int L, int R, int me, 
                                float *min, float *max)
{
  int i, d;
  int from = this->StartVal[me];
  int to   = this->EndVal[me];

  if (L > from){
    from = L;
  }
  if (R < to){
    to = R;
  }

  if (from <= to){

    from -= this->StartVal[me];
    to   -= this->StartVal[me];

    float *val = this->CurrentPtArray + from*3;

    for (d=0; d<3; d++){
      min[d] = max[d] = val[d];
    }

    for (i= from+1; i<=to; i++){

      val += 3;

      for (d=0; d<3; d++){

        if (val[d] < min[d]){
          min[d] = val[d];
        }
        else if (val[d] > max[d]){
          max[d] = val[d];
        }
      } 
    }
  } 
  else{

    // this guy has none of the data, but still must participate
    //   in ReduceMax and ReduceMin

    for (d=0; d<3; d++){
    
      min[d] = this->Top->Max[d];
      max[d] = this->Top->Min[d];
    }
  }
}
float *vtkPKdTree::DataBounds(int L, int K, int R) 
{
  float localMinLeft[3];    // Left region is L through K-1
  float localMaxLeft[3];
  float globalMinLeft[3];
  float globalMaxLeft[3];
  float localMinRight[3];   // Right region is K through R
  float localMaxRight[3];
  float globalMinRight[3];
  float globalMaxRight[3];
 
  float *globalBounds = new float [12];

  int fail = (globalBounds == NULL);

  if (this->AllCheckForFailure(fail, "DataBounds", "memory allocation")){
    return NULL;
  }

  this->GetLocalMinMax(L, K-1, this->MyId, localMinLeft, localMaxLeft);

  this->GetLocalMinMax(K, R, this->MyId, localMinRight, localMaxRight);

  this->SubGroup->ReduceMin(localMinLeft, globalMinLeft, 3, 0);
  this->SubGroup->Broadcast(globalMinLeft, 3, 0);

  this->SubGroup->ReduceMax(localMaxLeft, globalMaxLeft, 3, 0);
  this->SubGroup->Broadcast(globalMaxLeft, 3, 0);

  this->SubGroup->ReduceMin(localMinRight, globalMinRight, 3, 0);
  this->SubGroup->Broadcast(globalMinRight, 3, 0);

  this->SubGroup->ReduceMax(localMaxRight, globalMaxRight, 3, 0);
  this->SubGroup->Broadcast(globalMaxRight, 3, 0);

  float *left = globalBounds;
  float *right = globalBounds + 6;

  MinMaxToBounds(left, globalMinLeft, globalMaxLeft);

  MinMaxToBounds(right, globalMinRight, globalMaxRight);

  return globalBounds;
}

//--------------------------------------------------------------------
// Complete the tree - Different nodes of tree were computed by different
//   processors.  Now put it together.
//--------------------------------------------------------------------

int vtkPKdTree::CompleteTree()
{
  // calculate depth of entire tree

  int depth;
  int myDepth = vtkPKdTree::ComputeDepth(this->Top);

  this->SubGroup->ReduceMax(&myDepth, &depth, 1, 0); 
  this->SubGroup->Broadcast(&depth, 1, 0); 

  // fill out nodes of tree

  int fail = vtkPKdTree::FillOutTree(this->Top, depth);

  if (this->AllCheckForFailure(fail, "CompleteTree", "memory allocation")){
    return 1;
  }

  // For every node of the tree, processors with data for that
  //   node send to processors needing data for that node

  int *buf = new int [this->NumProcesses];

  fail = (buf == NULL);

  if (this->AllCheckForFailure(fail, "CompleteTree", "memory allocation")){
    if (buf) delete [] buf;
    return 1;
  }

  this->RetrieveData(this->Top, buf);

  delete [] buf;

  this->SetActualLevel();

  this->BuildRegionList();

  return 0;
}

void vtkPKdTree::PackData(vtkKdNode *kd, float *data)
{
  int i, v;

  data[0] = (float)kd->Dim;
  data[1] = (float)kd->Left->NumCells;
  data[2] = (float)kd->Right->NumCells;

  v = 3;
  for (i=0; i<3; i++){
    data[v++]    = (float)kd->Left->Min[i];
    data[v++]  = (float)kd->Left->Max[i];
    data[v++]  = (float)kd->Left->MinVal[i];
    data[v++]  = (float)kd->Left->MaxVal[i];
    data[v++] = (float)kd->Right->Min[i];
    data[v++] = (float)kd->Right->Max[i];
    data[v++] = (float)kd->Right->MinVal[i];
    data[v++] = (float)kd->Right->MaxVal[i];
  }
} 
void vtkPKdTree::UnpackData(vtkKdNode *kd, float *data)
{
  int i, v;

  kd->Dim             = (int)data[0];
  kd->Left->NumCells  = (int)data[1];
  kd->Right->NumCells = (int)data[2];

  v = 3;
  for (i=0; i<3; i++){
    kd->Left->Min[i]     = (float)data[v++];
    kd->Left->Max[i]     = (float)data[v++];
    kd->Left->MinVal[i]  = (float)data[v++];
    kd->Left->MaxVal[i]  = (float)data[v++];
    kd->Right->Min[i]    = (float)data[v++];
    kd->Right->Max[i]    = (float)data[v++];
    kd->Right->MinVal[i] = (float)data[v++];
    kd->Right->MaxVal[i] = (float)data[v++];
  }
} 
void vtkPKdTree::RetrieveData(vtkKdNode *kd, int *sources)
{
  int i;
  float data[27];

  if (kd->Left == NULL) return;

  int ihave = (kd->Dim < 3);

  this->SubGroup->Gather(&ihave, sources, 1, 0);
  this->SubGroup->Broadcast(sources, this->NumProcesses, 0);

  // a contiguous group of process IDs built this node, the first
  // in the group broadcasts the results to everyone else

  int root = -1;

  for (i=0; i<this->NumProcesses; i++){

    if (sources[i]){
      root = i;
      break;
    }
  }
  if (root == -1){

    // Normally BuildLocator will create a complete tree, but
    // it may refuse to divide a region if all the data is at
    // the same point along the axis it wishes to divide.  In
    // that case, this region was not divided, so just return.

    vtkKdTree::DeleteNodes(kd->Left);
    vtkKdTree::DeleteNodes(kd->Right);
    
    kd->Left = kd->Right = NULL;

    return;
  }

  if (root == this->MyId){

    vtkPKdTree::PackData(kd, data);
  }

  this->SubGroup->Broadcast(data, 27, root);

  if (!ihave){

    vtkPKdTree::UnpackData(kd, data);
  }

  this->RetrieveData(kd->Left, sources);

  this->RetrieveData(kd->Right, sources);

  return; 
}

int vtkPKdTree::FillOutTree(vtkKdNode *kd, int level)
{
  if (level == 0) return 0;

  if (kd->Left == NULL){

    kd->Left = new vtkKdNode;

    if (!kd->Left) goto doneError2;

    kd->Left->SetBounds(-1,-1,-1,-1,-1,-1);
    kd->Left->SetDataBounds(-1,-1,-1,-1,-1,-1);
    kd->Left->SetNumberOfCells(-1);

    kd->Left->Up = kd;
  }
  if (kd->Right == NULL){

    kd->Right = new vtkKdNode;

    if (!kd->Right) goto doneError2;

    kd->Right->SetBounds(-1,-1,-1,-1,-1,-1);
    kd->Right->SetDataBounds(-1,-1,-1,-1,-1,-1);
    kd->Right->SetNumberOfCells(-1);

    kd->Right->Up = kd;
  }

  if (vtkPKdTree::FillOutTree(kd->Left, level-1)) goto doneError2;

  if (vtkPKdTree::FillOutTree(kd->Right, level-1)) goto doneError2;

  return 0;

doneError2:

  return 1;
}

int vtkPKdTree::ComputeDepth(vtkKdNode *kd)
{
  int leftDepth = 0; 
  int rightDepth = 0;

  if ((kd->Left == NULL) && (kd->Right == NULL)) return 0;

  if (kd->Left){
    leftDepth = vtkPKdTree::ComputeDepth(kd->Left);
  }
  if (kd->Right){
    rightDepth = vtkPKdTree::ComputeDepth(kd->Right);
  }

  if (leftDepth > rightDepth) return leftDepth + 1;
  else return rightDepth + 1;
}

//--------------------------------------------------------------------
// lists, lists, lists
//--------------------------------------------------------------------

int vtkPKdTree::AllocateDoubleBuffer()
{
  this->FreeDoubleBuffer();

  this->PtArraySize = this->NumCells[this->MyId] * 3;

  this->PtArray2 = new float [this->PtArraySize];

  this->CurrentPtArray = this->PtArray;
  this->NextPtArray    = this->PtArray2;

  return (this->PtArray2 == NULL);
}
void vtkPKdTree::SwitchDoubleBuffer()
{
  float *temp = this->CurrentPtArray;

  this->CurrentPtArray = this->NextPtArray;
  this->NextPtArray = temp;
}
void vtkPKdTree::FreeDoubleBuffer()
{
  FreeList(this->PtArray2);
  this->CurrentPtArray = this->PtArray;
  this->NextPtArray = NULL;
}

int vtkPKdTree::AllocateSelectBuffer()
{
  this->FreeSelectBuffer();

  this->SelectBuffer = new int [this->NumProcesses * 7];

  return (this->SelectBuffer == NULL);
}
void vtkPKdTree::FreeSelectBuffer()
{
  if (this->SelectBuffer){
    delete [] this->SelectBuffer;
    this->SelectBuffer = NULL;
  }
}

#define FreeListOfLists(list, len) \
{                                  \
  int i;                           \
  if (list){                       \
    for (i=0; i<len; i++){            \
      if (list[i]) delete [] list[i]; \
    }                                 \
    delete [] list;                   \
    list = NULL;                      \
  }                                   \
}

#define MakeList(field, type, len) \
  {                                \
   field = new type [len];         \
   if (field) memset(field, 0, (len) * sizeof(type));  \
  }

// global index lists -----------------------------------------------

void vtkPKdTree::InitializeGlobalIndexLists()
{
  this->StartVal = NULL;
  this->EndVal   = NULL;
  this->NumCells = NULL;
}
int vtkPKdTree::AllocateAndZeroGlobalIndexLists()
{
  this->FreeGlobalIndexLists();

  MakeList(this->StartVal, int, this->NumProcesses);
  MakeList(this->EndVal, int, this->NumProcesses);
  MakeList(this->NumCells, int, this->NumProcesses);

  int defined = ((this->StartVal != NULL) && 
                 (this->EndVal != NULL) && 
                 (this->NumCells != NULL));

  if (!defined) this->FreeGlobalIndexLists();

  return !defined;
}
void vtkPKdTree::FreeGlobalIndexLists()
{
  FreeList(StartVal);
  FreeList(EndVal);
  FreeList(NumCells);
}
int vtkPKdTree::BuildGlobalIndexLists(int numMyCells)
{
  int fail = this->AllocateAndZeroGlobalIndexLists();

  if (this->AllCheckForFailure(fail, "BuildGlobalIndexLists", "memory allocation")){
    this->FreeGlobalIndexLists();
    return 1;
  }

  this->SubGroup->Gather(&numMyCells, this->NumCells, 1, 0);

  this->SubGroup->Broadcast(this->NumCells, this->NumProcesses, 0);

  this->StartVal[0] = 0;
  this->EndVal[0] = this->NumCells[0] - 1; 

  this->TotalNumCells = this->NumCells[0]; 

  for (int i=1; i<this->NumProcesses; i++){

    this->StartVal[i] = this->EndVal[i-1] + 1;
    this->EndVal[i]   = this->EndVal[i-1] + this->NumCells[i];

    this->TotalNumCells += this->NumCells[i];
  }

  return 0;
}

// Region assignment lists ---------------------------------------------

void vtkPKdTree::InitializeRegionAssignmentLists()
{
  this->RegionAssignmentMap = NULL;
  this->NumRegionsAssigned  = NULL;
}
int vtkPKdTree::AllocateAndZeroRegionAssignmentLists()
{
  this->FreeRegionAssignmentLists();

  this->RegionAssignmentMapLength = this->NumRegions;

  MakeList(this->RegionAssignmentMap, int , this->NumRegions);
  MakeList(this->NumRegionsAssigned, int, this->NumProcesses);

  int defined = ((this->RegionAssignmentMap != NULL) &&
                 (this->NumRegionsAssigned != NULL) ); 

  if (!defined) this->FreeRegionAssignmentLists();

  return !defined;
}
void vtkPKdTree::FreeRegionAssignmentLists()
{
  FreeList(this->RegionAssignmentMap);
  FreeList(this->NumRegionsAssigned);
  this->RegionAssignmentMapLength = 0;
}

// Process data tables ------------------------------------------------

void vtkPKdTree::InitializeProcessDataLists()
{
  this->DataLocationMap = NULL;

  this->NumProcessesInRegion = NULL;
  this->ProcessList = NULL;

  this->NumRegionsInProcess = NULL;
  this->RegionList = NULL;

  this->CellCountList = NULL;

  this->NumBlockIds = 0;
  this->BlockIdList = NULL;
  this->NumBlocksInRegion = NULL;
  this->CumulativeNumBlocks = NULL;
  this->BlockList = NULL;
  this->TempBlockCounts = NULL;
  this->TotalBlockCounts = NULL;
  this->ProcessRegionBlockCounts = NULL;
}
int vtkPKdTree::AllocateAndZeroProcessDataLists()
{
  int nRegions = this->NumRegions;
  int nProcesses = this->NumProcesses;

  this->FreeProcessDataLists();

  MakeList(this->DataLocationMap, char, nRegions * nProcesses);

  if (this->DataLocationMap == NULL) goto doneError3;

  MakeList(this->NumProcessesInRegion, int ,nRegions);

  if (this->NumProcessesInRegion == NULL) goto doneError3;

  MakeList(this->ProcessList, int * ,nRegions);

  if (this->ProcessList == NULL) goto doneError3;

  MakeList(this->NumRegionsInProcess, int ,nProcesses);

  if (this->NumRegionsInProcess == NULL) goto doneError3;

  MakeList(this->RegionList, int * ,nProcesses);

  if (this->RegionList == NULL) goto doneError3;

  MakeList(this->CellCountList, int * ,nRegions);

  if (this->CellCountList == NULL) goto doneError3;

  if (BlockData){

    MakeList(this->NumBlocksInRegion, int ,nRegions);
    MakeList(this->CumulativeNumBlocks, int ,nRegions);
    MakeList(this->BlockList, int * ,nRegions);
    MakeList(this->ProcessRegionBlockCounts, int * ,nProcesses);

    if ( (this->NumBlocksInRegion == NULL) ||
         (this->CumulativeNumBlocks == NULL) ||
         (this->BlockList == NULL) ||
         (this->ProcessRegionBlockCounts == NULL)){

      goto doneError3;
    }
  }

  return 0;

doneError3:
  this->FreeProcessDataLists();
  return 1;
}
void vtkPKdTree::FreeBlockDataLists()
{
  FreeList(this->BlockIdList);
  this->NumBlockIds = 0;

  FreeList(this->NumBlocksInRegion);

  FreeList(this->CumulativeNumBlocks);

  FreeListOfLists(this->BlockList, this->NumRegions); 

  FreeList(this->TempBlockCounts);
  FreeList(this->TotalBlockCounts);

  FreeListOfLists(this->ProcessRegionBlockCounts, this->NumProcesses);
}
void vtkPKdTree::FreeProcessDataLists()
{
  int nRegions = this->NumRegions;
  int nProcesses = this->NumProcesses;

  this->FreeBlockDataLists();

  FreeListOfLists(this->CellCountList, nRegions);

  FreeListOfLists(this->RegionList, nProcesses);

  FreeList(this->NumRegionsInProcess);

  FreeListOfLists(this->ProcessList, nRegions);

  FreeList(this->NumProcessesInRegion);

  FreeList(this->DataLocationMap);
}

void vtkPKdTree::ReleaseTables()
{
  this->FreeRegionAssignmentLists();
  this->FreeProcessDataLists();
  this->FreeCellCenters();
}

//--------------------------------------------------------------------
// Create tables indicating which processes have data for which
//  regions.  If block information is requested, also create tables
//  showing which blocks appear in which region, and breaking down
//  which processes have data for those blocks in those regions.
//--------------------------------------------------------------------

int vtkPKdTree::BuildRegionProcessTables()
{
  int proc, reg;
  int retval = 0;
  int *cellCounts = NULL;
  int *tempbuf;
  char *procData, *myData; 

  tempbuf = NULL;
  procData = myData = NULL;

  int fail = this->AllocateAndZeroProcessDataLists();

  if (this->AllCheckForFailure(fail, "BuildRegionProcessTables", "memory allocation")){
    this->FreeProcessDataLists();
    return 1;
  }

  // Get a global list of all block ids across the data set

  if (this->BlockData){
    this->BuildGlobalBlockIdList();
  }

  // Build table indicating which processes have data for which regions,
  //   broken down by block if BlockData on.

  cellCounts = this->CollectLocalRegionProcessData();

  fail = (cellCounts == NULL);

  if (this->AllCheckForFailure(fail,"BuildRegionProcessTables","error")){
    goto doneError4;
  }

  myData = this->DataLocationMap + (this->MyId * this->NumRegions);

  for (reg=0; reg < this->NumRegions; reg++){
    if (cellCounts[reg] > 0) myData[reg] = 1;
  }

  this->SubGroup->Gather(myData, this->DataLocationMap,
                         this->NumRegions, 0);

  this->SubGroup->Broadcast(this->DataLocationMap, 
                    this->NumRegions * this->NumProcesses, 0);

  // Other helpful tables - not the fastest way to create this
  //   information, but it uses the least memory

  procData = this->DataLocationMap;

  for (proc=0; proc<this->NumProcesses; proc++){

    for (reg=0; reg < this->NumRegions; reg++){

      if (*procData) {
        this->NumProcessesInRegion[reg]++;
        this->NumRegionsInProcess[proc]++;
      }
      procData++;
    }
  }
  for (reg=0; reg < this->NumRegions; reg++){

    int nprocs = this->NumProcessesInRegion[reg];

    if (nprocs > 0){
      this->ProcessList[reg] = new int [nprocs];
      this->ProcessList[reg][0] = -1;
      this->CellCountList[reg] = new int [nprocs];
      this->CellCountList[reg][0] = -1;
    }
  }
  for (proc=0; proc < this->NumProcesses; proc++){

    int nregs = this->NumRegionsInProcess[proc];

    if (nregs > 0){
      this->RegionList[proc] = new int [nregs];
      this->RegionList[proc][0] = -1;
    }
  }

  procData = this->DataLocationMap;

  for (proc=0; proc<this->NumProcesses; proc++){

    for (reg=0; reg < this->NumRegions; reg++){

      if (*procData) {

        this->AddEntry(this->ProcessList[reg], 
                       this->NumProcessesInRegion[reg], proc);

        this->AddEntry(this->RegionList[proc], 
                       this->NumRegionsInProcess[proc], reg);
      }
      procData++;
    }
  }

  // Cell counts per process per region

  tempbuf = new int [this->NumRegions * this->NumProcesses];

  fail = (tempbuf == NULL);

  if (this->AllCheckForFailure(fail,"BuildRegionProcessTables","memory allocation")){

    goto doneError4;
  }

  this->SubGroup->Gather(cellCounts, tempbuf, this->NumRegions, 0);
  this->SubGroup->Broadcast(tempbuf, this->NumProcesses*this->NumRegions, 0);

  for (proc=0; proc<this->NumProcesses; proc++){

    int *procCount = tempbuf + (proc * this->NumRegions);

    for (reg=0; reg < this->NumRegions; reg++){

       if (procCount[reg]> 0){
         this->AddEntry(this->CellCountList[reg], 
                          this->NumProcessesInRegion[reg],
                          procCount[reg]);
       }
    }
  }

  if (this->NumBlockIds > 0){
    this->BuildBlockCountTables();
  }

  goto done4;

doneError4:
  
  this->FreeProcessDataLists();
  retval = 1;

done4:

  FreeList(cellCounts);
  FreeList(tempbuf);
  
  return retval;
}
int vtkPKdTree::SameBlockIdList(vtkDataArray *blockIds)
{
  if (this->NumProcesses == 1) return 1;

  float range[2];
  blockIds->GetRange(range);

  float *allrange = new float [2 * this->NumProcesses];

  this->SubGroup->Gather(range, allrange, 2, 0);
  this->SubGroup->Broadcast(allrange, 2*this->NumProcesses, 0);

  int agree = 1;

  for (int p=1; p<this->NumProcesses; p++){

    if ( (allrange[2*p]   != allrange[0]) ||
         (allrange[2*p+1] != allrange[1])){

       agree = 0;
       break;
     }
  }

  delete [] allrange;

  if (!agree) return 0;

  if ((range[0] == range[1]) ||
      (range[0] == range[1] - 1)){

    return 1;
  }

  int *ids = (int *)blockIds->GetVoidPointer(0);
  int nids = blockIds->GetNumberOfTuples();

  int *idVals;

  int numIdVals = this->MakeSortedUnique(ids, nids, &idVals);

  delete [] idVals;

  int *allNumIdVals = new int [this->NumProcesses];

  this->SubGroup->Gather(&numIdVals, allNumIdVals, 1, 0);
  this->SubGroup->Broadcast(allNumIdVals, this->NumProcesses, 0);

  agree = 0;

  if (numIdVals == range[1] - range[0] + 1){

    agree = 1;

    for (int p=1; p<this->NumProcesses; p++){

      if (allNumIdVals[p] != allNumIdVals[0]){
         agree = 0;
         break;
      }
    }
  }
  delete [] allNumIdVals;
   
  return agree;
}
int vtkPKdTree::BuildGlobalBlockIdList()
{
  vtkDataArray *blockIds=NULL;
  int fail = 0;

  // Block data is computed only for data set 0, which is all there is
  // 99.999% of the time.  Block ids across different data sets are meaningless.

  vtkUnstructuredGrid *ugrid = 
    vtkUnstructuredGrid::SafeDownCast(this->DataSets[0]);

  if (!ugrid){

    VTKWARNING("block data not computed, data set is not an unstructured grid");
    fail = 1;

  } else{

    blockIds = ugrid->GetCellData()->GetScalars("BlockId");

    if (!blockIds){
      VTKWARNING("block data not computed, block ids not available");
      fail = 1;
    }
  }
  if (this->AllCheckForFailure(fail, "BuildGlobalBlockIdList", "no block data")){
    this->FreeBlockDataLists();
    return 0;
  }

  // get a list of all block ids found on all processors

  if (blockIds->GetDataTypeSize() != sizeof(int)){

    VTKWARNING("block data not computed, block Id data size changed");
    this->FreeBlockDataLists();
    return 0;
  }

  // simple case - same contiguous list of block Ids on all processors

  int simpleCase = this->SameBlockIdList(blockIds);

  if (!simpleCase){

    int *allIds;

    int numAllIds = this->SubGroup->AllReduceUniqueList(
                         static_cast<int *>(blockIds->GetVoidPointer(0)),
                         blockIds->GetNumberOfTuples(), &allIds);

    this->NumBlockIds = numAllIds;
    this->FirstBlockId = allIds[0];
    this->LastBlockId =  allIds[numAllIds-1];

    if ((this->LastBlockId - this->FirstBlockId + 1) == this->NumBlockIds){
      this->BlockIdsContiguous = 1;
      this->BlockIdList = NULL;
      delete [] allIds;
    }
    else{
      this->BlockIdsContiguous = 0;
      this->BlockIdList = allIds;
    }
  }
  else{
    float range[2];
    blockIds->GetRange(range);

    this->NumBlockIds        = (int)(range[1] - range[0] + 1);
    this->FirstBlockId       = (int)range[0];
    this->LastBlockId        = (int)range[1];
    this->BlockIdsContiguous = 1;
    this->BlockIdList        = NULL;
  }

  MakeList(this->TempBlockCounts, int, this->NumRegions * this->NumBlockIds);

  fail = (this->TempBlockCounts == NULL);

  if (this->AllCheckForFailure(fail, "BuildGlobalBlockIdList", "memory allocation")){
    this->FreeBlockDataLists();
    return 0;
  }

  return 0;
}
void vtkPKdTree::BuildBlockCountTables()
{
  int index,block,reg,proc,fail;
  int *mylist, *myptr, *globallist, *globalptr;
  int totalRegionBlocks;
  int numb = this->NumBlockIds;
  int numr = this->NumRegions;
  int nump = this->NumProcesses;
  int size = numb * numr;

  int *tempbuf = new int [size];

  fail = (tempbuf == NULL);

  if (this->AllCheckForFailure(fail, "BuildBlockCountTables", "memory allocation")){
    goto doneError6;
  }
   
  memcpy(tempbuf, this->TempBlockCounts, sizeof(int) * size); // my totals

  this->SubGroup->ReduceSum(tempbuf, this->TempBlockCounts,  size, 0);

  this->SubGroup->Broadcast(this->TempBlockCounts, size, 0);  // global totals

  totalRegionBlocks = 0;

  globalptr = this->TempBlockCounts;

  // how many blocks occur in each spatial region

  for (reg=0; reg<numr; reg++){

    this->CumulativeNumBlocks[reg] = totalRegionBlocks;

    int numRegBlocks = 0;

    for (index=0; index < numb; index++){
      if (globalptr[index] > 0) numRegBlocks++;
    }

    this->NumBlocksInRegion[reg] = numRegBlocks;

    totalRegionBlocks += numRegBlocks;

    globalptr += numb;
  }

  // for each region, list the blocks found in that region

  for (reg=0, fail=0; reg<numr && !fail; reg++){

   if (this->NumBlocksInRegion[reg] > 0){
     this->BlockList[reg] = new int [this->NumBlocksInRegion[reg]];
     fail = (this->BlockList[reg] == NULL);
   }
   else{
     this->BlockList[reg] = NULL;
   }
  }

  if (!fail){
    this->TotalBlockCounts = new int [totalRegionBlocks];
    fail = (this->TotalBlockCounts == NULL);
  }

  for (proc=0; proc<nump && !fail; proc++){
    this->ProcessRegionBlockCounts[proc] = new int [totalRegionBlocks];
    fail = (this->ProcessRegionBlockCounts[proc] == NULL);
  }

  if (this->AllCheckForFailure(fail, "BuildBlockCountTables", "memory allocation")){
    goto doneError6;
  }

  // for each processor, compactly list the count of cells it's holding
  //   for each block found in each region

  globalptr = this->TempBlockCounts;

  for (reg=0; reg<numr; reg++, globalptr += numb){

    int nblocks = this->NumBlocksInRegion[reg];

    if (nblocks > 0){

      int i=0;

      for (index=0; index < numb; index++){
        if (globalptr[index] > 0){
          this->BlockList[reg][i++] = index;
          if (i == nblocks) break;
        }
      }
    }
  }

  mylist = this->ProcessRegionBlockCounts[this->MyId];
  myptr = tempbuf;

  globallist = this->TotalBlockCounts;
  globalptr = this->TempBlockCounts;

  for (reg=0; reg<numr; reg++){

    int nblocks = this->NumBlocksInRegion[reg];

    for (index=0; index < nblocks; index++){

      block = this->BlockList[reg][index];

      mylist[index] = myptr[block];

      globallist[index] = globalptr[block];
    }

    mylist += nblocks;
    myptr += numb;
    globallist += nblocks;
    globalptr += numb;
  }

  for (proc=0; proc<nump; proc++){
      this->SubGroup->Broadcast(this->ProcessRegionBlockCounts[proc],
                                totalRegionBlocks, proc);
  }

  goto done6;

doneError6:

  this->FreeBlockDataLists();

done6:

  FreeList(this->TempBlockCounts);
  FreeList(tempbuf);

  return;
}
int vtkPKdTree::FindBlockId(int index)
{
  if (this->NumBlockIds == 0) return -1;

  if ( (index < 0) || (index >= this->NumBlockIds)) return -1;

  if (this->BlockIdsContiguous){
    return this->FirstBlockId + index;
  }

  return this->BlockIdList[index];
}
int vtkPKdTree::FindBlockIdIndex(int block)
{
  if (this->BlockIdsContiguous){

    int index = block - this->FirstBlockId;

    if ( (index < 0) || (index >= this->NumBlockIds)) return -1;

    return index;
  }
  return this->BinarySearch(this->BlockIdList, this->NumBlockIds, block);
}
int vtkPKdTree::FindRegionBlockIndex(int regionId, int blockIndex)
{
  if ( (regionId < 0) || (regionId >= this->NumRegions)) return -1;

  if ( (blockIndex < 0) || (blockIndex >= this->NumBlockIds)) return -1;

  int index = this->CumulativeNumBlocks[regionId];

  int regionIndex = -1;

  for (int i=0; i<this->NumBlocksInRegion[regionId]; i++){

    if (this->BlockList[regionId][i] == blockIndex){
      regionIndex = i;
      break;
    }
  }

  if (regionIndex < 0) return -1;

  return index + regionIndex;
}
int *vtkPKdTree::CollectLocalRegionProcessData()
{
  vtkUnstructuredGrid *ugrid=NULL;
  vtkDataArray *blockIds=NULL;
  int *Ids=NULL;
  int numIds=0;
  int *cellCounts;

  int numMyCells = this->NumCells[this->MyId];

  if (this->GetNumberOfCellCenters() < numMyCells) return NULL;

  int numRegions = this->NumRegions;

  MakeList(cellCounts, int, numRegions);

  if (!cellCounts){
     VTKERROR("CollectLocalRegionProcessData - memory allocation");
     return NULL;
  }

  if (this->NumBlockIds > 0){
    ugrid = vtkUnstructuredGrid::SafeDownCast(this->DataSets[0]);
    if (ugrid) blockIds = ugrid->GetCellData()->GetScalars("BlockId");

    if (!blockIds){
      this->FreeBlockDataLists();
      VTKWARNING("CollectLocalRegionProcessData - block id data unavailable");
      Ids = NULL;
    }
    else{
      Ids = (int *)blockIds->GetVoidPointer(0);
      numIds = blockIds->GetNumberOfTuples();
    }
  }

  float *pt = this->CurrentPtArray;

  for (int i=0; i<numMyCells; i++){

    int regionId = this->GetRegionContainingPoint(pt[0],pt[1],pt[2]);

    if ( (regionId < 0) || (regionId >= numRegions)){

      VTKERROR("CollectLocalRegionProcessData - corrupt data");
      return NULL;
    }

    cellCounts[regionId]++;

    if (numIds > i){

      if ((Ids[i] >= this->FirstBlockId) && 
          (Ids[i] <= this->LastBlockId)){

        int index = this->FindBlockIdIndex(Ids[i]);
        if (index >= 0){
          this->TempBlockCounts[regionId * this->NumBlockIds + index]++;
        }
      }
    }

    pt += 3;
  }

  return cellCounts;
}
void vtkPKdTree::AddEntry(int *list, int len, int id)
{
  int i=0;

  while ((i < len) && (list[i] != -1)) i++;

  if (i == len) return;  // error

  list[i++] = id;

  if (i < len) list[i] = -1;
}
int vtkPKdTree::BinarySearch(int *list, int len, int which)
{
int mid, left, right;

  mid = -1;

  if (len <= 3){
    for (int i=0; i<len; i++){
      if (list[i] == which){
        mid=i;
        break;
      }
    }
  }
  else{

    mid = len >> 1;
    left = 0;
    right = len-1;

    while (list[mid] != which){

      if (list[mid] < which){
        left = mid+1;
      }
      else{
        right = mid-1;
      }

      if (right > left+1){
        mid = (left + right) >> 1;
      }
      else{
        if (list[left] == which) mid = left;
        else if (list[right] == which) mid = right;
        else mid = -1;
        break;
      }
    }
  }
  return mid;
}
//--------------------------------------------------------------------
// Assign responsibility for each spatial region to one process
//--------------------------------------------------------------------

int vtkPKdTree::UpdateRegionAssignment()
{
  int returnVal = 0;

  if (this->RegionAssignment== ContiguousAssignment){
    returnVal = this->AssignRegionsContiguous();
  }
  else if (this->RegionAssignment== RoundRobinAssignment){
    returnVal = this->AssignRegionsRoundRobin();
  }
  else if (this->RegionAssignment== MinimizeDataMovementAssignment){
    returnVal = this->AssignRegionsToMinimizeDataMovement();
  }

  return returnVal;
}
int vtkPKdTree::AssignRegionsToMinimizeDataMovement()
{
  VTKWARNING("Sorry, vtkPKdTree::AssignRegionsToMinimizeDataMovement is not implemented.  Would be nice though.");

  return this->AssignRegionsContiguous();

  this->RegionAssignment = MinimizeDataMovementAssignment;

  if ((this->Top == NULL) || (this->CellCountList == NULL)){
    return 0;
  }

  // insert code here...

  //this->UpdateUserMTimes();

  return 0;
}
int vtkPKdTree::AssignRegionsRoundRobin()
{
  this->RegionAssignment = RoundRobinAssignment;

  if (this->Top == NULL){
    return 0;
  }

  int nProcesses = this->NumProcesses;
  int nRegions = this->NumRegions;

  int fail = this->AllocateAndZeroRegionAssignmentLists();

  if (fail){
    return 1;
  }

  for (int i=0, procID=0; i<nRegions; i++){

    this->RegionAssignmentMap[i] = procID;
    this->NumRegionsAssigned[procID]++;

    procID = ( (procID == nProcesses-1) ? 0 : procID+1);
  }
  //this->UpdateUserMTimes();

  return 0;
}
int vtkPKdTree::AssignRegions(int *map, int len)
{
  this->FreeRegionAssignmentLists();

  MakeList(this->RegionAssignmentMap, int , len);
  MakeList(this->NumRegionsAssigned, int, this->NumProcesses);

  int defined = ((this->RegionAssignmentMap != NULL) &&
                 (this->NumRegionsAssigned != NULL) );

  if (!defined){
    this->FreeRegionAssignmentLists();
    VTKERROR("AssignRegions - memory allocation");
    return 1;
  }

  this->RegionAssignmentMapLength = len;

  this->RegionAssignment = UserDefinedAssignment;

  for (int i=0; i<len; i++){

    if ( (map[i] < 0) || (map[i] >= this->NumProcesses)){
      this->FreeRegionAssignmentLists();
      VTKERROR("AssignRegions - invalid process id in map");
      return 1;
    }

    this->RegionAssignmentMap[i] = map[i];
    this->NumRegionsAssigned[map[i]]++;
  }
  //this->UpdateUserMTimes();

  return 0;
}
void vtkPKdTree::AddProcessRegions(int procId, vtkKdNode *kd)
{
  vtkIdList *leafNodeIds = vtkIdList::New();

  vtkKdTree::GetLeafNodeIds(kd, leafNodeIds);

  int nLeafNodes = leafNodeIds->GetNumberOfIds();

  for (int n=0; n<nLeafNodes; n++){
    this->RegionAssignmentMap[leafNodeIds->GetId(n)] = procId;
    this->NumRegionsAssigned[procId]++;
  }

  leafNodeIds->Delete();
}

int vtkPKdTree::AssignRegionsContiguous()
{
  int p;

  this->RegionAssignment = ContiguousAssignment;

  if (this->Top == NULL){
    return 0;
  }

  int nProcesses = this->NumProcesses;
  int nRegions = this->NumRegions;

  if (nRegions <= nProcesses){
    this->AssignRegionsRoundRobin();
    return 0;
  }

  int fail = this->AllocateAndZeroRegionAssignmentLists();

  if (fail){
    return 1;
  }

  int floorLogP, ceilLogP;

  for (floorLogP = 0; (nProcesses >> floorLogP) > 0; floorLogP++);
  floorLogP--;

  int P = 1 << floorLogP;

  if (nProcesses == P) ceilLogP = floorLogP;
  else                 ceilLogP = floorLogP + 1;

  vtkKdNode **nodes = new vtkKdNode* [P];

  this->GetRegionsAtLevel(floorLogP, nodes);

  if (floorLogP == ceilLogP){

    for (p=0; p<nProcesses; p++){
      this->AddProcessRegions(p, nodes[p]);
    }
  }
  else{

    int nodesLeft = 1 << ceilLogP;
    int procsLeft = nProcesses;
    int procId = 0;

    for (int i=0; i<P; i++){

      if (nodesLeft > procsLeft){

        this->AddProcessRegions(procId, nodes[i]);

        procsLeft -= 1;
        procId    += 1;
      }
      else{
        this->AddProcessRegions(procId, nodes[i]->Left);
        this->AddProcessRegions(procId+1, nodes[i]->Right);

        procsLeft -= 2;
        procId    += 2;
      }
      nodesLeft -= 2;
    }
  }

  delete [] nodes;

  //this->UpdateUserMTimes();

  return 0;
}

//--------------------------------------------------------------------
// Queries
//--------------------------------------------------------------------

int vtkPKdTree::GetRegionAssignmentList(int procId, vtkIdList *list)
{
  if ( (procId < 0) || (procId >= this->NumProcesses)){
    VTKERROR("GetRegionAssignmentList - invalid process id");
    return 0;
  }

  if (!this->RegionAssignmentMap){
    this->UpdateRegionAssignment();

    if (!this->RegionAssignmentMap){
      return 0;
    }
  }

  int nregions = this->NumRegionsAssigned[procId];

  list->Initialize();
  list->SetNumberOfIds(nregions);

  int nextId = 0;

  for (int i=0; i<this->RegionAssignmentMapLength; i++){

    if (this->RegionAssignmentMap[i] == procId){

      list->SetId(nextId++, i);

      if (nextId == nregions) break;
    }
  }

  return nregions;
}
int vtkPKdTree::HasData(int processId, int regionId)
{
  if ((!this->DataLocationMap) ||
      (processId < 0) || (processId >= this->NumProcesses) ||
      (regionId < 0) || (regionId >= this->NumRegions)   ) {
 
    VTKERROR("HasData - invalid request");
    return 0;
  }

  int where = this->NumRegions * processId + regionId;

  return this->DataLocationMap[where];
}

int vtkPKdTree::GetTotalProcessesInRegion(int regionId)
{
  if (!this->NumProcessesInRegion ||
      (regionId < 0) || (regionId >= this->NumRegions)   ) {
 
    VTKERROR("GetTotalProcessesInRegion - invalid request");
    return 0;
  }

  return this->NumProcessesInRegion[regionId];
}

int vtkPKdTree::GetProcessListForRegion(int regionId, vtkIdList *processes)
{
  if (!this->ProcessList ||
      (regionId < 0) || (regionId >= this->NumRegions)   ) {
 
    VTKERROR("GetProcessListForRegion - invalid request");
    return 0;
  }

  int nProcesses = this->NumProcessesInRegion[regionId];

  for (int i=0; i<nProcesses; i++){

    processes->InsertNextId(this->ProcessList[regionId][i]);
  }

  return nProcesses;
}

int vtkPKdTree::GetProcessesCellCountForRegion(int regionId, int *count, int len)
{
  if (!this->CellCountList ||
      (regionId < 0) || (regionId >= this->NumRegions)   ) {
 
    VTKERROR("GetProcessesCellCountForRegion - invalid request");
    return 0;
  }

  int nProcesses = this->NumProcessesInRegion[regionId];

  nProcesses = (len < nProcesses) ? len : nProcesses;

  for (int i=0; i<nProcesses; i++){
    count[i] = this->CellCountList[regionId][i];
  }

  return nProcesses;
}

int vtkPKdTree::GetProcessCellCountForRegion(int processId, int regionId)
{
  int nCells;

  if (!this->CellCountList ||
      (regionId < 0) || (regionId >= this->NumRegions) ||
      (processId < 0) || (processId >= this->NumProcesses)){ 
 
    VTKERROR("GetProcessCellCountForRegion - invalid request");
    return 0;
  }

  int nProcesses = this->NumProcessesInRegion[regionId];

  int which = -1;

  for (int i=0; i<nProcesses; i++){

    if (this->ProcessList[regionId][i] == processId){
      which = i;
      break;
    }
  }

  if (which == -1) nCells = 0;
  else             nCells = this->CellCountList[regionId][which];

  return nCells;
}

int vtkPKdTree::GetTotalRegionsForProcess(int processId)
{
  if ((!this->NumRegionsInProcess) ||
      (processId < 0) || (processId >= this->NumProcesses)){ 
 
    VTKERROR("GetTotalRegionsForProcess - invalid request");
    return 0;
  }

  return this->NumRegionsInProcess[processId];
}

int vtkPKdTree::GetRegionListForProcess(int processId, vtkIdList *regions)
{
  if (!this->RegionList ||
      (processId < 0) || (processId >= this->NumProcesses)){ 
 
    VTKERROR("GetRegionListForProcess - invalid request");
    return 0;
  }

  int nRegions = this->NumRegionsInProcess[processId];

  for (int i=0; i<nRegions; i++){

    regions->InsertNextId(this->RegionList[processId][i]);
  }

  return nRegions;
}
int vtkPKdTree::GetRegionsCellCountForProcess(int processId, int *count, int len)
{
  if (!this->CellCountList ||
      (processId < 0) || (processId >= this->NumProcesses)){ 
 
    VTKERROR("GetRegionsCellCountForProcess - invalid request");
    return 0;
  }

  int nRegions = this->NumRegionsInProcess[processId];

  nRegions = (len < nRegions) ? len : nRegions;

  for (int i=0; i<nRegions; i++){

    int regionId = this->RegionList[processId][i];
    int iam;

    for (iam = 0; iam < this->NumProcessesInRegion[regionId]; iam++){
      if (this->ProcessList[regionId][iam] == processId) break;
    }

    count[i] = this->CellCountList[regionId][iam];
  }
  return nRegions;
}
int vtkPKdTree::GetNumberOfBlockIds()
{
  return this->NumBlockIds;
}
int vtkPKdTree::GetListOfBlockIds(vtkIdList *list)
{
  if (this->NumBlockIds == 0) return 0;

  if (this->BlockIdsContiguous){

    for (int b=this->FirstBlockId; b <= this->LastBlockId; b++){

      list->InsertNextId(b);
    }
  }
  else{
    for (int b=0; b<this->NumBlockIds; b++){
      list->InsertNextId(this->BlockIdList[b]);
    }
  }

  return this->NumBlockIds;
}
int vtkPKdTree::GetBlockIdsInRegion(int regionId, vtkIdList *blockIds)
{
  if (!this->NumBlocksInRegion ||
      (regionId < 0) ||
      (regionId >= this->NumRegions)){

    return 0;
  }

  int nblocks = this->NumBlocksInRegion[regionId];

  for (int i=0; i<nblocks; i++){

    int Id = this->FindBlockId(this->BlockList[regionId][i]);

    blockIds->InsertNextId(Id);
  }

  return nblocks;
}
int vtkPKdTree::GetRegionIdsContainingBlock(int blockID, vtkIdList *regionIds)
{
  if (!this->NumBlocksInRegion || !this->BlockList) return 0;

  int index = this->FindBlockIdIndex(blockID);

  if (index < 0) return 0;

  int nregions = 0;

  for (int reg = 0; reg < this->NumRegions; reg++){

    int nblocks = this->NumBlocksInRegion[reg];

    int found = this->BinarySearch(this->BlockList[reg], nblocks, index);

    if (found >= 0){
      regionIds->InsertNextId(reg);
      nregions++;
    }
  }

  return nregions;
}
int vtkPKdTree::GetBlockCellCountsInRegion(int regionId, int *cellCounts, int len)
{
  if (!this->TotalBlockCounts ||
      (regionId < 0) ||
      (regionId >= this->NumRegions)){

    return 0;
  }
  int nblocks = this->NumBlocksInRegion[regionId];

  if (len < nblocks) nblocks = len;

  int *counts = this->TotalBlockCounts + this->CumulativeNumBlocks[regionId];

  for (int i=0; i<nblocks; i++){
    cellCounts[i] = counts[i];
  }

  return nblocks;
}
int vtkPKdTree::GetProcessBlockCellCountInRegion(int processId, int regionId, int blockId)
{
  if ((regionId < 0) ||
      (regionId >= this->NumRegions) ||
      (processId < 0) ||
      (processId >= this->NumProcesses)){

    return 0;
  }

  if ((this->ProcessRegionBlockCounts == NULL) ||
      (this->ProcessRegionBlockCounts[processId] == NULL)){

    return 0;
  } 

  int index = this->FindBlockIdIndex(blockId);

  if (index < 0) return 0;

  int regionBlockIndex = this->FindRegionBlockIndex(regionId, index);

  if (regionBlockIndex < 0) return 0;
  
  return this->ProcessRegionBlockCounts[processId][regionBlockIndex];
}

void vtkPKdTree::PrintTiming(ostream& os, vtkIndent indent)
{
  os << indent << "Total cells in distributed data: " << this->TotalNumCells << endl;

  if (this->NumProcesses){
    os << indent << "Average cells per processor: " ;
            os << this->TotalNumCells / this->NumProcesses << endl;
  }
  vtkTimerLog::DumpLogWithIndents(&os, (float)0.0);
}
void vtkPKdTree::PrintTables(ostream & os, vtkIndent indent)
{
  int nregions = this->NumRegions;
  int nprocs =this->NumProcesses;
  int r,p,b,n,m;
  int *blockList, *count;

  if (this->RegionAssignmentMap){

    int *map = this->RegionAssignmentMap; 
    int *num = this->NumRegionsAssigned;
    int halfr = this->RegionAssignmentMapLength/2;
    int halfp = nprocs/2;

    os << indent << "Region assignments:" << endl;
    for (r=0; r < halfr; r++){
      os << indent << "  region " << r << " to process " << map[r] ;
      os << "    region " << r+halfr << " to process " << map[r+halfr] ;
      os << endl;
    }
    for (p=0; p < halfp; p++){
      os << indent << "  " << num[p] << " regions to process " << p ;
      os << "    " << num[p+halfp] << " regions to process " << p+ halfp ;
      os << endl;
    }
    if (nprocs > halfp*2){
      os << indent << "  " << num[nprocs-1];
      os << " regions to process " << nprocs-1 << endl ;
    }
  }

  if (this->ProcessList){
    os << indent << "Processes holding data for each region:" << endl;
    for (r=0; r<nregions; r++){

      n = this->NumProcessesInRegion[r];

      os << indent << " region " << r << " (" << n << " processes): ";

      for (p=0; p<n; p++){
        if (p && (p%10==0)) os << endl << indent << "   "; 
        os << this->ProcessList[r][p] << " " ;
      }
      os << endl;
    }
  }
  if (this->RegionList){
    os << indent << "Regions held by each process:" << endl;
    for (p=0; p<nprocs; p++){

      n = this->NumRegionsInProcess[p];

      os << indent << " process " << p << " (" << n << " regions): ";

      for (r=0; r<n; r++){
        if (r && (r%10==0)) os << endl << indent << "   "; 
        os << this->RegionList[p][r] << " " ;
      }
      os << endl;
    }
  }
  if (this->CellCountList){
    os << indent << "Number of cells per process per region:" << endl;
    for (r=0; r<nregions; r++){

      n = this->NumProcessesInRegion[r];

      os << indent << " region: " << r << "  ";
      for (p=0; p<n; p++){
        if (p && (p%5==0)) os << endl << indent << "   "; 
        os << this->ProcessList[r][p] << " - ";
        os << this->CellCountList[r][p] << " cells, ";
      }
      os << endl;
    }
  }

  if (this->BlockIdList){
    os << indent << this->NumBlockIds << " block Ids: " ;

    for (b=0; b<this->NumBlockIds; b++){
        if (b && (b%10==0)) os << endl << indent << "   "; 
        os << this->BlockIdList[b] << " ";
    }
    os << endl;
  }
  else if (this->NumBlockIds > 0){
    os << endl << indent << this->NumBlockIds << " blocks in the data set" << endl;
    if (this->BlockIdsContiguous){
      os << indent << "They range from " << this->FirstBlockId;
      os << " to " << this->LastBlockId << endl;
    }
  }
  if (this->NumBlocksInRegion){

    os << indent << "\nBlocks found in each spatial region: " << endl;

    for (r=0; r < nregions; r++){

      n = this->NumBlocksInRegion[r];
      m = this->CumulativeNumBlocks[r];

      blockList = this->BlockList[r];

      if (!blockList) continue;

      os << indent << "region " << r << " (" << n;
      os << " blocks, " << m << " cumulative): ";

      for (b = 0; b < n ; b++){
        if (b && (b%10==0)) os << endl << indent << "   "; 
        os << this->FindBlockId(blockList[b]) << " ";
      }
      os << endl;
    }
  }

  if (this->TotalBlockCounts){

    os << indent << "\nCount of cells by spatial region, by block ID:" << endl;

    count = this->TotalBlockCounts;

    for (r=0; r<nregions; r++){

      n = this->NumBlocksInRegion[r];

      int *blockList = this->BlockList[r];

      if (!blockList) continue;

      os << indent << "  region " << r << ": ";
     
      for (b=0; b<n; b++){

        if (b && (b%5==0)) os << endl << indent << "           "; 

        os << this->FindBlockId(blockList[b]) << " - " << *count++ << ", ";
      }
      os << endl;
    }
  }

  if (this->ProcessRegionBlockCounts){

    os << indent << "\nCount of cells by process, by region, by block ID:" << endl;

    for (p=0; p<nprocs; p++){

      count = this->ProcessRegionBlockCounts[p];

      if (!count) continue;

      os << indent << "  process " << p << ":" << endl;

      for (r=0; r<nregions; r++){

        n = this->NumBlocksInRegion[r];

        if (!this->HasData(p,r)){
           count += n;
           continue;
        }

        blockList = this->BlockList[r];

        os << indent << "    region " << r << ": ";

        for (b=0; b<n; b++){

          if (b && (b%5==0)) os << endl << indent << "            "; 

          os << this->FindBlockId(blockList[b]) << " - " << *count++ << ", ";
        }
        os << endl;
      }
    }
  }
}
void vtkPKdTree::PrintSelf(ostream& os, vtkIndent indent)
{  
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumRegionsOrLess: " << this->NumRegionsOrLess << endl;
  os << indent << "NumRegionsOrMore: " << this->NumRegionsOrMore << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "BlockData: " << this->BlockData << endl;
  os << indent << "ProcessCellCountData: " << this->ProcessCellCountData << endl;
  os << indent << "RegionAssignment: " << this->RegionAssignment << endl;

  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "SubGroup: " << this->SubGroup<< endl;
  os << indent << "NumProcesses: " << this->NumProcesses << endl;
  os << indent << "MyId: " << this->MyId << endl;

  os << indent << "RegionAssignmentMap: " << this->RegionAssignmentMap << endl;
  os << indent << "NumRegionsAssigned: " << this->NumRegionsAssigned << endl;
  os << indent << "NumProcessesInRegion: " << this->NumProcessesInRegion << endl;
  os << indent << "ProcessList: " << this->ProcessList << endl;
  os << indent << "NumRegionsInProcess: " << this->NumRegionsInProcess << endl;
  os << indent << "RegionList: " << this->RegionList << endl;
  os << indent << "CellCountList: " << this->CellCountList << endl;

  os << indent << "NumBlockIds: " << this->NumBlockIds << endl;
  os << indent << "BlockIdList: " << this->BlockIdList << endl;
  os << indent << "FirstBlockId: " << this->FirstBlockId << endl;
  os << indent << "LastBlockId: " << this->LastBlockId << endl;
  os << indent << "BlockIdsContiguous: " << this->BlockIdsContiguous << endl;
  os << indent << "NumBlocksInRegion: " << this->NumBlocksInRegion << endl;
  os << indent << "CumulativeNumBlocks: " << this->CumulativeNumBlocks << endl;
  os << indent << "BlockList: " << this->BlockList << endl;
  os << indent << "TempBlockCounts: " << this->TempBlockCounts << endl;
  os << indent << "TotalBlockCounts: " << this->TotalBlockCounts << endl;
  os << indent << "ProcessRegionBlockCounts: " << this->ProcessRegionBlockCounts << endl;

  os << indent << "StartVal: " << this->StartVal << endl;
  os << indent << "EndVal: " << this->EndVal << endl;
  os << indent << "NumCells: " << this->NumCells << endl;
  os << indent << "TotalNumCells: " << this->TotalNumCells << endl;

  os << indent << "PtArray: " << this->PtArray << endl;
}

//
// sub group operations - we need reduce min, reduce max, reduce sum,
//    gather, and broadcast.
//    A process can only have one of these groups at a time, different groups
//    across the application must have unique tags.  A group must be a
//    contiguous set of process Ids.
//

vtkSubGroup::vtkSubGroup(int p0, int p1, int me, int tag, vtkCommunicator *c)
{
  int i, ii;
  this->nmembers = p1 - p0 + 1;
  this->tag = tag;
  this->comm = c;

  this->members = new int [this->nmembers];

  this->myLocalRank = -1;

  for (i=p0, ii=0; i<=p1; i++){
    if (i == me) this->myLocalRank = ii;
    this->members[ii++] = i;
  }

  if (this->myLocalRank == -1){
    FreeList(this->members);
    return;
  }

  this->gatherRoot = this->gatherLength = -1;

  this->computeFanInTargets();
}

int vtkSubGroup::computeFanInTargets()
{
  int i;
  this->nTo = 0;
  this->nFrom = 0;

  for (i = 1; i < this->nmembers; i <<= 1){
  
    int other = this->myLocalRank ^ i;

    if (other >= this->nmembers) continue;

    if (this->myLocalRank > other){
      this->fanInTo = other;

      this->nTo++;   /* one at most */

      break;
    }
    else{
      this->fanInFrom[this->nFrom] = other;
      this->nFrom++;
    }
  }
  return 0;
}
void vtkSubGroup::moveRoot(int root)
{
  int tmproot = this->members[root];
  this->members[root] = this->members[0];
  this->members[0] = tmproot;

  return;
}
void vtkSubGroup::restoreRoot(int root)
{
  if (root == 0) return;

  this->moveRoot(root);

  if (this->myLocalRank == root){
    this->myLocalRank = 0;
    this->computeFanInTargets();
  }
  else if (this->myLocalRank == 0){
    this->myLocalRank = root;
    this->computeFanInTargets();
  }

  return;
}
void vtkSubGroup::setUpRoot(int root)
{
  if (root == 0) return;

  this->moveRoot(root);

  if (this->myLocalRank == root){

    this->myLocalRank = 0;
    this->computeFanInTargets();
  }
  else if (this->myLocalRank == 0){

    this->myLocalRank = root;
    this->computeFanInTargets();
  }

  return;
}

vtkSubGroup::~vtkSubGroup()
{
  FreeList(this->members);
}
void vtkSubGroup::setGatherPattern(int root, int length)
{
  int i;

  if ( (root == this->gatherRoot) && (length == this->gatherLength)){
    return;
  }

  this->gatherRoot   = root;
  this->gatherLength = length;

  int clogn; // ceiling(log2(this->nmembers))
  for (clogn=0; 1<<clogn < this->nmembers; clogn++);

  int left = 0;
  int right = this->nmembers - 1;
  int iroot = root;

  this->nSend = 0;
  this->nRecv = 0;

  for (i=0; i<clogn; i++){

    int src, offset, len;

    int mid = (left + right) / 2;

    if (iroot <= mid) {
      src = (iroot == left ? mid + 1 : right);
    } else {
      src = (iroot == right ? mid : left);
    }
    if (src <= mid) {                   /* left ... mid */
      offset = left * length;
      len =  (mid - left + 1) * length;
    } else {                            /* mid+1 ... right */
      offset = (mid + 1) * length;
      len    = (right - mid) * length;
    }
    if (this->myLocalRank == iroot) {

      this->recvId[this->nRecv] = this->members[src];
      this->recvOffset[this->nRecv] = offset;
      this->recvLength[this->nRecv] = len;
            
      this->nRecv++;
        
    } else if (this->myLocalRank == src) {

      this->sendId = this->members[iroot];
      this->sendOffset = offset;
      this->sendLength = len;
            
      this->nSend++;
    }
    if (this->myLocalRank <= mid) {
      if (iroot > mid) {
        iroot = src;
      }
      right = mid;
    } else {
      if (iroot <= mid) {
        iroot = src;
      }
      left = mid + 1;
    }
    if (left == right) break;
  }
  return;
}

int vtkSubGroup::getLocalRank(int processId)
{
  int localRank = processId - this->members[0];

  if ( (localRank < 0) || (localRank >= this->nmembers)) return -1;
  else return localRank;
}
#define MINOP  if (tempbuf[p] < buf[p]) buf[p] = tempbuf[p];
#define MAXOP  if (tempbuf[p] > buf[p]) buf[p] = tempbuf[p];
#define SUMOP  buf[p] += tempbuf[p];

#define REDUCE(Type, name, op) \
int vtkSubGroup::Reduce##name(Type *data, Type *to, int size, int root) \
{ \
  int i, p;\
  if (this->nmembers == 1){\
    for (i=0; i<size; i++) to[i] = data[i];\
    return 0;\
  }\
  if ( (root < 0) || (root >= this->nmembers)) return 1;\
  if (root != 0) this->setUpRoot(root); \
  Type *buf, *tempbuf; \
  tempbuf = new Type [size]; \
  if (this->nTo > 0){\
    buf = new Type [size];\
  } \
  else{ \
    buf = to;\
  } \
  if (buf != data) memcpy(buf, data, size * sizeof(Type));\
  for (i=0; i < this->nFrom; i++){ \
    this->comm->Receive(tempbuf, size,\
                      this->members[this->fanInFrom[i]], this->tag);\
    for (p=0; p<size; p++){ op }\
  }\
  delete [] tempbuf;\
  if (this->nTo > 0){\
    this->comm->Send(buf, size, this->members[this->fanInTo], this->tag);\
    delete [] buf;\
  }\
  if (root != 0) this->restoreRoot(root);\
  return 0; \
}

REDUCE(int, Min, MINOP)
REDUCE(float, Min, MINOP)
REDUCE(int, Max, MAXOP)
REDUCE(float, Max, MAXOP)
REDUCE(int, Sum, SUMOP)

#define BROADCAST(Type) \
int vtkSubGroup::Broadcast(Type *data, int length, int root) \
{ \
  int i;\
  if (this->nmembers == 1) return 0;\
  if ( (root < 0) || (root >= this->nmembers)) return 1;\
  if (root != 0) this->setUpRoot(root); \
  if (this->nTo > 0){ \
    this->comm->Receive(data, length, this->members[this->fanInTo], this->tag);\
  } \
  for (i = this->nFrom-1 ; i >= 0; i--){ \
    this->comm->Send(data, length, this->members[this->fanInFrom[i]], this->tag); \
  } \
  if (root != 0) this->restoreRoot(root); \
  return 0; \
}

BROADCAST(char)
BROADCAST(int)
BROADCAST(float)

#define GATHER(Type)\
int vtkSubGroup::Gather(Type *data, Type *to, int length, int root)\
{ \
  int i;\
  Type *recvBuf;\
  if (this->nmembers == 1){\
    for (i=0; i<length; i++) to[i] = data[i];\
    return 0;\
  }\
  if ( (root < 0) || (root >= this->nmembers)) return 1;\
  this->setGatherPattern(root, length);\
  if (this->nSend > 0){\
    recvBuf = new Type [length * this->nmembers];\
  }\
  else{\
    recvBuf = to;\
  }\
  for (i=0; i<this->nRecv; i++){ \
    this->comm->Receive(recvBuf + this->recvOffset[i], \
              this->recvLength[i], this->recvId[i], this->tag);\
  }\
  memcpy(recvBuf + (length * this->myLocalRank), data,\
         length * sizeof(Type));\
  if (this->nSend > 0){ \
    this->comm->Send(recvBuf + this->sendOffset,\
                     this->sendLength, this->sendId, this->tag);\
    delete [] recvBuf;\
  }\
  return 0; \
}

GATHER(int)
GATHER(char)
GATHER(float)

int vtkSubGroup::AllReduceUniqueList(int *list, int len, int **newList)
{
  int transferLen, myListLen, lastListLen, nextListLen;
  int *myList, *lastList, *nextList;

  myListLen = vtkPKdTree::MakeSortedUnique(list, len, &myList);

  if (this->nmembers == 1){ 
    *newList = myList;
    return myListLen;
  }

  lastList = myList;
  lastListLen = myListLen;

  for (int i=0; i < this->nFrom; i++){ 

    this->comm->Receive(&transferLen, 1,
                      this->members[this->fanInFrom[i]], this->tag); 

    int *buf = new int [transferLen];

    this->comm->Receive(buf, transferLen,
                      this->members[this->fanInFrom[i]], this->tag+1); 

    nextListLen = vtkSubGroup::MergeSortedUnique(lastList, lastListLen, 
                                           buf, transferLen, &nextList);

    delete [] buf;
    delete [] lastList;

    lastList = nextList;
    lastListLen = nextListLen;
  }                                                     

  if (this->nTo > 0){ 

    this->comm->Send(&lastListLen, 1, this->members[this->fanInTo], this->tag);

    this->comm->Send(lastList, lastListLen, 
                     this->members[this->fanInTo], this->tag+1); 
  }                


  this->Broadcast(&lastListLen, 1, 0);

  if (this->myLocalRank > 0){

    delete [] lastList;
    lastList = new int [lastListLen];
  }

  this->Broadcast(lastList, lastListLen, 0);

  *newList = lastList;

  return lastListLen; 
}
int vtkSubGroup::MergeSortedUnique(int *list1, int len1, int *list2, int len2,
                                   int **newList)
{
  int newLen = 0;
  int i1=0;
  int i2=0;

  int *newl = new int [len1 + len2];

  if (newl == NULL) return 0;

  while ((i1 < len1) || (i2 < len2)){

    if (i2 == len2){
      newl[newLen++] = list1[i1++];
    }
    else if (i1 == len1){
      newl[newLen++] = list2[i2++];
    }
    else if (list1[i1] < list2[i2]){
      newl[newLen++] = list1[i1++];
    }
    else if (list1[i1] > list2[i2]){
      newl[newLen++] = list2[i2++];
    }
    else{
      newl[newLen++] = list1[i1++];
      i2++;
    }
  }

  *newList = newl;

  return newLen;
}
int vtkPKdTree::MakeSortedUnique(int *list, int len, int **newList)
{
  int i, newlen;
  int *newl;

  newl = new int [len];
  if (newl == NULL) return 0;

  memcpy(newl, list, len * sizeof(int));
  vtkstd::sort(newl, newl + len);

  for (i=1, newlen=1; i<len; i++){

    if (newl[i] == newl[newlen-1]) continue;

    newl[newlen++] = newl[i];
  }
  
  *newList = newl;

  return newlen;
}

int vtkSubGroup::Barrier()
{
  float token = 0;
  float result;

  this->ReduceMin(&token, &result, 1, 0);
  this->Broadcast(&token, 1, 0);
  return 0;
}

void vtkSubGroup::PrintSubGroup() const
{
  int i;
  cout << "(Fan In setup ) nFrom: " << this->nFrom << ", nTo: " << this->nTo << endl;
  if (this->nFrom > 0){
    for (i=0; i<nFrom; i++){
      cout << "fanInFrom[" << i << "] = " << this->fanInFrom[i] << endl;
    }
  }
  if (this->nTo > 0) cout << "fanInTo = " << this->fanInTo << endl;

  cout << "(Gather setup ) nRecv: " << this->nRecv << ", nSend: " << this->nSend << endl;
  if (this->nRecv > 0){
    for (i=0; i<nRecv; i++){
      cout << "recvId[" << i << "] = " << this->recvId[i];
      cout << ", recvOffset[" << i << "] = " << this->recvOffset[i]; 
      cout << ", recvLength[" << i << "] = " << this->recvLength[i] << endl;
    }
  }
  if (nSend > 0){
    cout << "sendId = " << this->sendId;
    cout << ", sendOffset = " << this->sendOffset;
    cout << ", sendLength = " << this->sendLength << endl;
  }
  cout << "gatherRoot " << this->gatherRoot ;
  cout << ", gatherLength " << this->gatherLength << endl;

  cout << "nmembers: " << this->nmembers << endl;
  cout << "myLocalRank: " << this->myLocalRank << endl;
  for (i=0; i<this->nmembers; i++){
    cout << "  " << this->members[i];
    if (i && (i%20 == 0)) cout << endl;
  }
  cout << endl;
  cout << "comm: " << this->comm;
  cout << endl;
}
