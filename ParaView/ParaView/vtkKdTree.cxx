// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:  vtkKdTree.cxx
  Language:  C++
  Date:    $Date$
  Version:   $Revision$

=========================================================================*/

#include "vtkKdTree.h"
#include <algorithm>
#include <vtkObjectFactory.h>
#include <vtkDataSet.h>
#include <vtkCamera.h>
#include <vtkPolyData.h>
#include <vtkRenderWindow.h>
#include <vtkFloatArray.h>
#include <vtkMath.h>

vtkCxxRevisionMacro(vtkKdTree, "1.3");

// methods for vtkKdNode -------------------------------------------

const char *vtkKdNode::LevelMarker[20]={
"",
" ",
"  ",
"   ",
"    ",
"     ",
"      ",
"       ",
"        ",
"         ",
"          ",
"           ",
"            ",
"             ",
"              ",
"               ",
"                ",
"                 ",
"                  ",
"                   "
};

vtkKdNode::vtkKdNode()
{  
  this->Up = this->Left = this->Right = NULL;
  this->Dim = 3;
  this->Id = -1;
  this->NumCells = 0;
}
vtkKdNode::~vtkKdNode()
{
}
void vtkKdNode::SetBounds(double x1,double x2,double y1,double y2,double z1,double z2)
{
   this->Min[0] = x1; this->Max[0] = x2;
   this->Min[1] = y1; this->Max[1] = y2;
   this->Min[2] = z1; this->Max[2] = z2;
}
void vtkKdNode::GetBounds(double *b) const
{
   b[0] = this->Min[0]; b[1] = this->Max[0];
   b[2] = this->Min[1]; b[3] = this->Max[1];
   b[4] = this->Min[2]; b[5] = this->Max[2];
}
void vtkKdNode::SetDataBounds(double x1,double x2,double y1,double y2,double z1,double z2)
{
   this->MinVal[0] = x1; this->MaxVal[0] = x2;
   this->MinVal[1] = y1; this->MaxVal[1] = y2;
   this->MinVal[2] = z1; this->MaxVal[2] = z2;
}
void vtkKdNode::SetDataBounds(float *v)
{
  int x;
  double newbounds[6];

  int numCells = this->GetNumberOfCells();

  int i;

  if (this->Up){

    double bounds[6];

    this->Up->GetDataBounds(bounds);
   
    int dim = this->Up->GetDim();
   
    for (i=0; i<3; i++){
      if (i == dim) continue;

      newbounds[i*2]  = bounds[i*2];
      newbounds[i*2+1] = bounds[i*2+1];
    } 

    newbounds[dim*2] = newbounds[dim*2+1] = (double)v[dim];

    for (i = dim+3; i< numCells*3; i+=3){
   
      if (v[i] < newbounds[dim*2]) newbounds[dim*2] = (double)v[i];
      else if (v[i] > newbounds[dim*2+1]) newbounds[dim*2+1] = (double)v[i];
    }
  }
  else{
    for (i=0; i<3; i++){
      newbounds[i*2] = newbounds[i*2+1] = (double)v[i];
    }

    for (x = 3; x< numCells*3; x+=3){
      int y=x+1;
      int z=x+2;

      if (v[x] < newbounds[0]) newbounds[0] = (double)v[x];
      else if (v[x] > newbounds[1]) newbounds[1] = (double)v[x];

      if (v[y] < newbounds[2]) newbounds[2] = (double)v[y];
      else if (v[y] > newbounds[3]) newbounds[3] = (double)v[y];

      if (v[z] < newbounds[4]) newbounds[4] = (double)v[z];
      else if (v[z] > newbounds[5]) newbounds[5] = (double)v[z];
    }
  }

  this->SetDataBounds(newbounds[0], newbounds[1], newbounds[2],
            newbounds[3], newbounds[4], newbounds[5]);
}
void vtkKdNode::GetDataBounds(double *b) const
{
   b[0] = this->MinVal[0]; b[1] = this->MaxVal[0];
   b[2] = this->MinVal[1]; b[3] = this->MaxVal[1];
   b[4] = this->MinVal[2]; b[5] = this->MaxVal[2];
}
void vtkKdNode::AddChildNodes(vtkKdNode *left, vtkKdNode *right)
{     
  this->Left = left;
  this->Right = right;
      
  right->Up = this;
  left->Up  = this;
}       
int vtkKdNode::IntersectsBox(float x0, float x1, float y0, float y1,
                         float z0, float z1)
{
  return this->IntersectsBox((double)x0, (double)x1, (double)y0, (double)y1,
                         (double)z0, (double)z1);
}
int vtkKdNode::IntersectsBox(double x0, double x1, double y0, double y1,
                         double z0, double z1)
{
  if ( (this->MinVal[0] >= x1) ||
       (this->MaxVal[0] <= x0) ||
       (this->MinVal[1] >= y1) ||
       (this->MaxVal[1] <= y0) ||
       (this->MinVal[2] >= z1) ||
       (this->MaxVal[2] <= z0)){

    return 0;
  }
  else{
    return 1;
  }
}
int vtkKdNode::IntersectsRegion(vtkPlanesIntersection *pi)
{
  double x0, x1, y0, y1, z0, z1;
  vtkPoints *box = vtkPoints::New();

  box->SetNumberOfPoints(8);

  x0 = this->MinVal[0]; x1 = this->MaxVal[0];
  y0 = this->MinVal[1]; y1 = this->MaxVal[1];
  z0 = this->MinVal[2]; z1 = this->MaxVal[2];

  box->SetPoint(0, x1, y0, z1);
  box->SetPoint(1, x1, y0, z0);
  box->SetPoint(2, x1, y1, z0);
  box->SetPoint(3, x1, y1, z1);
  box->SetPoint(4, x0, y0, z1);
  box->SetPoint(5, x0, y0, z0);
  box->SetPoint(6, x0, y1, z0);
  box->SetPoint(7, x0, y1, z1);

  int intersects = pi->IntersectsRegion(box);

  box->Delete();

  return intersects;
}

void vtkKdNode::PrintNode(int depth)
{
  if ( (depth < 0) || (depth > 19)) depth = 19;
      
  printf("%s x (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f) - %d cells, #%d %s\n",
      vtkKdNode::LevelMarker[depth],
      this->Min[0], this->Max[0],
      this->Min[1], this->Max[1],
      this->Min[2], this->Max[2],
      this->NumCells, this->Id, this->Left ? "" : "(leaf node)" );
}
void vtkKdNode::PrintVerboseNode(int depth)
{
  if ( (depth < 0) || (depth > 19)) depth = 19;
      
  printf("%so Space (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f)\n",
      vtkKdNode::LevelMarker[depth],
      this->Min[0], this->Max[0],
      this->Min[1], this->Max[1],
      this->Min[2], this->Max[2]);
  printf("%s Data (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f)\n",
      vtkKdNode::LevelMarker[depth],
      this->MinVal[0], this->MaxVal[0],
      this->MinVal[1], this->MaxVal[1],
      this->MinVal[2], this->MaxVal[2]);
  printf("%s %d cells, id %d, cut next along %d, left %p, right %p, up %p\n",
      vtkKdNode::LevelMarker[depth],
      this->NumCells, this->Id, this->Dim, this->Left, this->Right, this->Up);
  printf("%s dim: %p\n\n",vtkKdNode::LevelMarker[depth],&(this->Dim));
}
// end of vtkKdNode -------------------------------------------


#define TIMER(s) if (this->Timing){ this->TimerLog->MarkStartEvent(s); }
#define TIMERDONE(s) if (this->Timing){ this->TimerLog->MarkEndEvent(s); }

vtkStandardNewMacro(vtkKdTree);

const int vtkKdTree::xdim  = 0;  // don't change these values
const int vtkKdTree::ydim  = 1;
const int vtkKdTree::zdim  = 2;

makeCompareFunc(X, 0);  // for sorting centroids
makeCompareFunc(Y, 1);
makeCompareFunc(Z, 2);

static int (*compareFuncs[3])(const void *, const void *)={
compareFuncX, compareFuncY, compareFuncZ
};

vtkKdTree::vtkKdTree()
{
  this->DataSets = NULL;
  this->NumDataSets = 0;
  this->NumDataSetsAllocated = 0;
  this->ValidDirections =
  (1 << vtkKdTree::xdim) | (1 << vtkKdTree::ydim) | (1 << vtkKdTree::zdim);

  this->RetainCellLocations = 0;
  this->CellCenters = NULL;
  this->CellCentersSize = 0;

  this->CellList = NULL;
  this->NumCellLists = 0;
  this->NumCellListsAllocated = 0;

  this->Top      = NULL;
  this->MaxLevel = 20;
  this->MinCells = 100;
  this->Level    = 0;
  this->NumRegions     = 0;
  this->RegionList   = NULL;

  this->Timing = 0;
  this->TimerLog = NULL;

  //this->NumberOfUsers = 0;
  //this->Users = NULL;
}
void vtkKdTree::DeleteNodes(vtkKdNode *nd)
{   
  if (nd->Left){
     vtkKdTree::DeleteNodes(nd->Left);
     delete nd->Left;
     nd->Left = NULL;
  }
  if (nd->Right){
     vtkKdTree::DeleteNodes(nd->Right);
     delete nd->Right;
     nd->Right = NULL;
  }
  return;
}
void vtkKdTree::DeleteAllCellLists(vtkKdTree::_cellList *list)
{
  int i;

  if (list->regionIds){
    free(list->regionIds); 
    list->regionIds = NULL;
  }

  if (list->cells){
    for (i=0; i<list->nRegions; i++){
      list->cells[i]->Delete();
    }

    free(list->cells);
    list->cells = NULL;
  }
  return;
}
int vtkKdTree::OnList(int *list, int size, int n)
{
  int i;
  for (i=0; i<size; i++){
    if (list[i] == n) return 1;
  }
  return 0;
}
void vtkKdTree::DeleteSomeCellLists(vtkKdTree::_cellList *list, 
               int ndelete, int *rlist, int listsize)
{
  int i, j;

  int nkeep = list->nRegions - ndelete;

  if (nkeep >= list->nRegions) return;

  int *regionList = (int *)malloc(sizeof(int) * nkeep);

  if (!regionList){
    vtkErrorMacro(<<"vtkKdTree::DeleteSomeCellLists memory allocation");
    return;
  }

  vtkIdList **cellIdLists = (vtkIdList **)malloc(nkeep * sizeof(vtkIdList *));

  if (!cellIdLists){
    vtkErrorMacro(<<"vtkKdTree::DeleteSomeCellLists memory allocation");
    return;
  }

  for (i=0, j=0; i < list->nRegions; i++){

    if (vtkKdTree::OnList(rlist, listsize, list->regionIds[i])){

      list->cells[i]->Delete();
    }
    else{

      regionList[j] = list->regionIds[i];
      cellIdLists[j] = list->cells[i];
      j++;
    }
  }

  free(list->regionIds);
  free(list->cells);

  list->regionIds = regionList;
  list->cells = cellIdLists;
  list->nRegions = nkeep;

  return;

}
vtkKdTree::~vtkKdTree()
{
  if (this->DataSets) free (this->DataSets);

  this->FreeCellCenters();

  this->FreeSearchStructure();

  if (this->TimerLog){
    this->TimerLog->Delete();
  }
  //if (this->Users)
  //{
  //  delete [] this->Users;
  //}

  return;
}

void vtkKdTree::Modified()
{
  //this->UpdateUserMTimes();  // change that may not require rebuilding tree
  vtkLocator::Modified();    // k-d tree must be re-built
}

void vtkKdTree::SetDataSet(vtkDataSet *set)
{
  int i;
  for (i=0; i<this->NumDataSets; i++){
    if (this->DataSets[i] == set) return;
  }    

  if (this->NumDataSetsAllocated == 0){
    this->DataSets = (vtkDataSet **)malloc(sizeof(vtkDataSet *));

    if (!this->DataSets){
      vtkErrorMacro(<<"vtkKdTree::SetDataSet memory allocation");
      return;
    }

    vtkLocator::SetDataSet(set);

    this->NumDataSetsAllocated = 1;
    this->NumDataSets          = 0;
  }
  else if (this->NumDataSetsAllocated - this->NumDataSets <= 0){

    this->NumDataSetsAllocated += 10;

    this->DataSets = (vtkDataSet **)
        realloc(this->DataSets, this->NumDataSetsAllocated * sizeof(vtkDataSet *));

    if (!this->DataSets){
      vtkErrorMacro(<<"vtkKdTree::SetDataSet memory allocation");
      return;
    }

  }
  this->DataSets[this->NumDataSets++] = set;

  this->Modified();
}
void vtkKdTree::RemoveDataSet(vtkDataSet *set)
{
  int i;
  int removeSet = -1;
  for (i=0; i<this->NumDataSets; i++){
    if (this->DataSets[i] == set){
       removeSet = i;
       break;
    }
  }
  if (removeSet >= 0){
    this->RemoveDataSet(removeSet);
  }
  else{
    vtkErrorMacro( << "vtkKdTree::RemoveDataSet not a valid data set");
  }
}
void vtkKdTree::RemoveDataSet(int which)
{
  int i;

  if ( (which < 0) || (which >= this->NumDataSets)){
    vtkErrorMacro( << "vtkKdTree::RemoveDataSet not a valid data set");
    return;
  }

  this->DeleteCellList(which, NULL, 0);

  for (i=which; i<this->NumDataSets-1; i++){
    this->DataSets[i] = this->DataSets[i+1];
  }
  this->NumDataSets--;
  this->Modified();
}
int vtkKdTree::GetDataSet(vtkDataSet *set)
{
  int i;
  int whichSet = -1;

  for (i=0; i<this->NumDataSets; i++){
    if (this->DataSets[i] == set){
      whichSet = i;
      break;
    } 
    
  }
  return whichSet;
}
void vtkKdTree::GetRegionBounds(int regionID, float bounds[6])
{
  if ( (regionID < 0) || (regionID >= this->NumRegions)){
    vtkErrorMacro( << "vtkKdTree::GetRegionBounds invalid region");
    return;
  }

  vtkKdNode *node = this->RegionList[regionID];

  bounds[0] = node->Min[0];
  bounds[2] = node->Min[1];
  bounds[4] = node->Min[2];
  bounds[1] = node->Max[0];
  bounds[3] = node->Max[1];
  bounds[5] = node->Max[2];
}
void vtkKdTree::GetRegionDataBounds(int regionID, float bounds[6])
{
  if ( (regionID < 0) || (regionID >= this->NumRegions)){
    vtkErrorMacro( << "vtkKdTree::GetRegionDataBounds invalid region");
    return;
  }

  vtkKdNode *node = this->RegionList[regionID];

  bounds[0] = node->MinVal[0];
  bounds[2] = node->MinVal[1];
  bounds[4] = node->MinVal[2];
  bounds[1] = node->MaxVal[0];
  bounds[3] = node->MaxVal[1];
  bounds[5] = node->MaxVal[2];
}
vtkKdNode **vtkKdTree::_GetRegionsAtLevel(int level, vtkKdNode **nodes, vtkKdNode *kd)
{
  if (level > 0){
    vtkKdNode **nodes0 = _GetRegionsAtLevel(level-1, nodes, kd->Left);
    vtkKdNode **nodes1 = _GetRegionsAtLevel(level-1, nodes0, kd->Right);

    return nodes1;

  }
  else{
    nodes[0] = kd;
    return nodes+1;
  }
}
void vtkKdTree::GetRegionsAtLevel(int level, vtkKdNode **nodes)
{
  if ( (level < 0) || (level > this->Level)) return;

  vtkKdTree::_GetRegionsAtLevel(level, nodes, this->Top);

  return;
}
void vtkKdTree::GetLeafNodeIds(vtkKdNode *node, vtkIdList *ids)
{
  if (node->Id < 0){
    vtkKdTree::GetLeafNodeIds(node->Left, ids);
    vtkKdTree::GetLeafNodeIds(node->Right, ids);
  }
  else{
    ids->InsertNextId(node->Id);
  }
  return;
}

int vtkKdTree::ComputeCellCenters()
{
  int i;
  float *center, pcoords[3];

  this->FreeCellCenters();

  int totalCells = 0;

  for (i=0; i<this->NumDataSets; i++){

    vtkDataSet *set = this->DataSets[i];    

    totalCells += set->GetNumberOfCells();
  }

  this->CellCenters = new float [3 * totalCells];

  if (!this->CellCenters){

    return 1;
  }

  this->CellCentersSize = totalCells;

  center = this->CellCenters;


  for (i=0; i<this->NumDataSets; i++){

    vtkDataSet *set = this->DataSets[i];    

    float *weights = new float [set->GetMaxCellSize()];

    int nCells = set->GetNumberOfCells();

    for (int j=0; j<nCells; j++){

      vtkCell *cell = set->GetCell(j);
      int subId = cell->GetParametricCenter(pcoords);
      cell->EvaluateLocation(subId, pcoords, center, weights);

      center += 3;
    }

    delete [] weights;
  }

  return 0;
}
void vtkKdTree::FreeCellCenters()
{

  if (this->CellCenters){
    delete [] this->CellCenters;
    this->CellCenters = NULL;
    this->CellCentersSize = 0;
  }
}
void vtkKdTree::ComputeCellCenter(vtkDataSet *set, int cellId, float *center)
{
  int setNum;

  if (set){
    setNum = this->GetDataSet(set);

    if ( setNum < 0){
      vtkErrorMacro(<<"vtkKdTree::ComputeCellCenter invalid data set");
      return;
    }
  }
  else{
    setNum = 0;
    set = this->DataSets[0];
  }

  if ( (cellId < 0) || (cellId >= set->GetNumberOfCells())){
    vtkErrorMacro(<<"vtkKdTree::ComputeCellCenter invalid cell ID");
    return;
  }

  if (!this->CellCenters){

    float pcoords[3];

    float *weights = new float [set->GetMaxCellSize()];

    vtkCell *cell = set->GetCell(cellId);

    int subId = cell->GetParametricCenter(pcoords);

    cell->EvaluateLocation(subId, pcoords, center, weights);

    delete [] weights;

    return;
  }

  int cellNum = 0;

  for (int i=0; i<setNum; i++){

    cellNum += this->DataSets[i]->GetNumberOfCells();
  }

  cellNum += cellId;

  int where = 3*cellNum;

  center[0] = this->CellCenters[where];
  center[1] = this->CellCenters[where+1];
  center[2] = this->CellCenters[where+2];

  return;
}

// Build the kdtree structure based on location of cell centroids.

void vtkKdTree::BuildLocator()
{
  int nCells=0;
  unsigned int maxTime=0;
  int i;

  for (i=0; i<this->NumDataSets; i++){
    maxTime = (this->DataSets[i]->GetMTime() > maxTime) ?
               this->DataSets[i]->GetMTime() : maxTime;
  }

  if ((this->Top != NULL) && (this->BuildTime > this->MTime) &&
       (this->BuildTime > maxTime))
  {
    return;
  }

  for (i=0; i<this->NumDataSets; i++){
    nCells += this->DataSets[i]->GetNumberOfCells();
  }

  if (nCells == 0)
  {
     vtkErrorMacro( << "vtkKdTree::BuildLocator - No cells to subdivide");
     return;
  }

  vtkDebugMacro( << "Creating Kdtree" );

  if (this->Timing){
    if (this->TimerLog == NULL) this->TimerLog = vtkTimerLog::New();
  }

  TIMER("Set up to build k-d tree");

  this->FreeSearchStructure();

  // volume bounds - push out a little if flat

  float setBounds[6], volBounds[6];

  for (i=0; i<this->NumDataSets; i++){
    if (i==0){
      this->DataSets[i]->GetBounds(volBounds);
    }
    else{
      this->DataSets[i]->GetBounds(setBounds);
      if (setBounds[0] < volBounds[0]) volBounds[0] = setBounds[0];
      if (setBounds[2] < volBounds[2]) volBounds[2] = setBounds[2];
      if (setBounds[4] < volBounds[4]) volBounds[4] = setBounds[4];
      if (setBounds[1] > volBounds[1]) volBounds[1] = setBounds[1];
      if (setBounds[3] > volBounds[3]) volBounds[3] = setBounds[3];
      if (setBounds[5] > volBounds[5]) volBounds[5] = setBounds[5];
    }
  }

  float diff[3], aLittle = 0.0;

  for (i=0; i<3; i++){
     diff[i] = volBounds[2*i+1] - volBounds[2*i];
     aLittle = (diff[i] > aLittle) ? diff[i] : aLittle;
  }
  if ((aLittle /= 100.0) <= 0.0){
     vtkErrorMacro( << "vtkKdTree::BuildLocator - degenerate volume");
     return;
  }
  for (i=0; i<3; i++){
     if (diff[i] <= 0){
        volBounds[2*i]   -= aLittle;
        volBounds[2*i+1] += aLittle;
     }
  }
  TIMERDONE("Set up to build k-d tree");
   
  // cell centers - basis of spacial decomposition

  TIMER("Create centroid list");

  int fail = this->ComputeCellCenters();

  TIMERDONE("Create centroid list");

  if (fail){
    vtkErrorMacro( << "vtkKdTree::BuildLocator - insufficient memory");
    return;
  }

  float *ptarray = this->GetCellCenters();
  int totalPts = this->GetNumberOfCellCenters();

  // create kd tree structure that balances cell centers

  vtkKdNode *kd = this->Top = new vtkKdNode();

  kd->SetBounds((double)volBounds[0], (double)volBounds[1], 
                (double)volBounds[2], (double)volBounds[3], 
                (double)volBounds[4], (double)volBounds[5]);

  kd->SetNumberOfCells(totalPts);

  kd->SetDataBounds((double)volBounds[0], (double)volBounds[1],
                (double)volBounds[2], (double)volBounds[3],
                (double)volBounds[4], (double)volBounds[5]); 

  TIMER("Build tree");

  this->DivideRegion(kd, ptarray, this->MaxLevel);

  TIMERDONE("Build tree");

  if (this->GetRetainCellLocations() == 0){
    this->FreeCellCenters();
  }


  this->SetActualLevel();
  this->BuildRegionList();

  this->BuildTime.Modified();

  return;
}
int vtkKdTree::ComputeLevel(vtkKdNode *kd)
{
  if (!kd) return 0;
  
  int iam = 1;

  if (kd->Left != NULL){

     int depth1 = vtkKdTree::ComputeLevel(kd->Left);
     int depth2 = vtkKdTree::ComputeLevel(kd->Right);

     if (depth1 > depth2) iam += depth1;
     else         iam += depth2;

  }
  return iam;
}
int vtkKdTree::SelectCutDirection(vtkKdNode *kd)
{
  int dim, i;

  int xdir = 1 << vtkKdTree::xdim;
  int ydir = 1 << vtkKdTree::ydim;
  int zdir = 1 << vtkKdTree::zdim;

  // determine direction in which to divide this region

  if (this->ValidDirections == xdir){
    dim = vtkKdTree::xdim;
  }
  else if (this->ValidDirections == ydir){
    dim = vtkKdTree::ydim;
  }
  else if (this->ValidDirections == zdir){
    dim = vtkKdTree::zdim;
  }
  else{
    // divide in the longest direction, for more compact regions

    double diff[3], dataBounds[6], maxdiff;
    kd->GetDataBounds(dataBounds);

    for (i=0; i<3; i++){ diff[i] = dataBounds[i*2+1] - dataBounds[i*2];}

    maxdiff = -1.0;

    if ((this->ValidDirections & xdir) && (diff[vtkKdTree::xdim] > maxdiff)){

      dim = vtkKdTree::xdim;
      maxdiff = diff[vtkKdTree::xdim];
    }

    if ((this->ValidDirections & ydir) && (diff[vtkKdTree::ydim] > maxdiff)){

      dim = vtkKdTree::ydim;
      maxdiff = diff[vtkKdTree::ydim];
    }

    if ((this->ValidDirections & zdir) && (diff[vtkKdTree::zdim] > maxdiff)){

      dim = vtkKdTree::zdim;
    }
  }
  return dim;
}
int vtkKdTree::DivideRegion(vtkKdNode *kd, float *c1, int nlevels)
{
  if (nlevels == 0) return 0;

  int minCells = this->GetMinCells();
  int numCells   = kd->GetNumberOfCells();

  if ((numCells < 2) || (minCells && (minCells > (numCells/2)))) return 0;

  int maxdim = this->SelectCutDirection(kd);

  kd->SetDim(maxdim);

  // find location such that cells are as evenly divided as possible
  //   cells 0 through "midpt"-1 fall on the left side,
  //   midpt through NumCells-1 on the right side.

  double coord;

#ifdef MEDIAN_SORT
  int midpt = vtkKdTree::MidValue(maxdim, c1, numCells, coord);
#else
  // usually much faster
  int midpt = vtkKdTree::Select(maxdim, c1, numCells, coord);
#endif

  vtkKdNode *left = new vtkKdNode();
  vtkKdNode *right = new vtkKdNode();

  kd->AddChildNodes(left, right);

  double bounds[6];
  kd->GetBounds(bounds);

  left->SetBounds(
     bounds[0], ((maxdim == xdim) ? coord : bounds[1]),
     bounds[2], ((maxdim == ydim) ? coord : bounds[3]),
     bounds[4], ((maxdim == zdim) ? coord : bounds[5]));
  
  left->SetNumberOfCells(midpt);
  
  right->SetBounds(
     ((maxdim == xdim) ? coord : bounds[0]), bounds[1],
     ((maxdim == ydim) ? coord : bounds[2]), bounds[3],
     ((maxdim == zdim) ? coord : bounds[4]), bounds[5]); 
  
  right->SetNumberOfCells(numCells - midpt);
  
  left->SetDataBounds(c1);
  right->SetDataBounds(c1 + midpt*3);
  
  this->DivideRegion(left, c1, nlevels - 1);
  
  this->DivideRegion(right, c1 + midpt*3, nlevels - 1);
  
  return 0;
}

// Sort list to find the median

int vtkKdTree::MidValue(int dim, float *c1, int nvals, double &coord)
{
  int mid   = nvals / 2;
  
  qsort(c1, nvals, sizeof(float) * 3, compareFuncs[dim]);
  
  float *divider = (c1 + (3 * mid) + dim);
  
  float rightcoord = *divider;
  float leftcoord  = *(divider-3);
  
  coord = (double)((leftcoord + rightcoord) / 2.0);
  
  return mid;
}

// Use Floyd & Rivest (1975) to find the median:
// Given an array X with element indices ranging from L to R, and
// a K such that L <= K <= R, rearrange the elements such that
// X[K] contains the ith sorted element,  where i = K - L + 1, and
// all the elements X[j], j < k satisfy X[j] <= X[K], and all the
// elements X[j], j > k satisfy X[j] >= X[K].

#define Exchange(array, x, y) {\
  float temp[3];                    \
  temp[0]        = array[3*x];      \
  temp[1]        = array[3*x + 1];  \
  temp[2]        = array[3*x + 2];  \
  array[3*x]     = array[3*y];      \
  array[3*x + 1] = array[3*y + 1];  \
  array[3*x + 2] = array[3*y + 2];  \
  array[3*y]     = temp[0];         \
  array[3*y + 1] = temp[1];         \
  array[3*y + 2] = temp[2];         \
}

#define sign(x) ((x<0) ? (-1) : (1))
#ifndef max
#define max(x,y) ((x>y) ? (x) : (y))
#endif
#ifndef min
#define min(x,y) ((x<y) ? (x) : (y))
#endif

int vtkKdTree::Select(int dim, float *c1, int nvals, double &coord)
{
  int left = 0;
  int mid = nvals / 2;
  int right = nvals -1;

  _Select(dim, c1, left, right, mid);

  float midValue = c1[mid*3 + dim];

  float leftMax = findMaxLeftHalf(dim, c1, mid);

  coord = ((double)midValue + (double)leftMax) / 2.0;

  return mid;
}
float vtkKdTree::findMaxLeftHalf(int dim, float *c1, int K)
{
  int i;

  float *Xcomponent = c1 + dim;
  float max = Xcomponent[0];

  for (i=3; i<K*3; i+=3){

    if (Xcomponent[i] > max) max = Xcomponent[i];
  }
  return max;
}
void vtkKdTree::_Select(int dim, float *X, int L, int R, int K)
{
  int N, I, J, S, SD, LL, RR;
  float Z, T;

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
      _Select(dim, X, LL, RR, K);
    }

    float *Xcomponent = X + dim;   // x, y or z component

    T = Xcomponent[K*3];

    // "the following code partitions X[L:R] about T."

    I = L;
    J = R;

    Exchange(X, L, K);

    if (Xcomponent[R*3] > T) Exchange(X, R, L);

    while (I < J){

      Exchange(X, I, J);

      while (Xcomponent[(++I)*3] < T);

      while (Xcomponent[(--J)*3] > T);
    }

    if (Xcomponent[L*3] == T){
      Exchange(X, L, J);
    }
    else{
      J++;
      Exchange(X, J, R);
    }

    // "now adjust L,R so they surround the subset containing
    // the (K-L+1)-th smallest element"

    if (J <= K) L = J + 1;
    if (K <= J) R = J -1;
  }
}

void vtkKdTree::SelfRegister(vtkKdNode *kd)
{
  if (kd->Left == NULL){
    this->RegionList[kd->Id] = kd; 
  }
  else{
    this->SelfRegister(kd->Left);
    this->SelfRegister(kd->Right);
  }

  return;
}
int vtkKdTree::SelfOrder(int startId, vtkKdNode *kd)
{
  int nextId;

  if (kd->Left == NULL){
    kd->Id = startId;
    nextId = startId + 1;
  }
  else{
    kd->Id = -1;
    nextId = vtkKdTree::SelfOrder(startId, kd->Left);
    nextId = vtkKdTree::SelfOrder(nextId, kd->Right);
  }

  return nextId;
}
void vtkKdTree::BuildRegionList()
{
  if (this->Top == NULL) return;

  this->NumRegions = vtkKdTree::SelfOrder(0, this->Top);
  
  this->RegionList = new vtkKdNode * [this->NumRegions];

  this->SelfRegister(this->Top);
}
void vtkKdTree::__printTree(vtkKdNode *kd, int depth, int v)
{
  if (v) kd->PrintVerboseNode(depth);
  else   kd->PrintNode(depth);

  if (kd->Left) vtkKdTree::__printTree(kd->Left, depth+1, v);
  if (kd->Right) vtkKdTree::__printTree(kd->Right, depth+1, v);
}
void vtkKdTree::_printTree(int v)
{
  vtkKdTree::__printTree(this->Top, 0, v);
}
void vtkKdTree::PrintTree()
{   
  _printTree(0);
}  
void vtkKdTree::PrintVerboseTree()
{      
  _printTree(1);
}          

void vtkKdTree::FreeSearchStructure()
{
  if (this->Top) {
    vtkKdTree::DeleteNodes(this->Top);
    delete this->Top;
    this->Top = NULL;
  }
  if (this->RegionList){
    delete [] this->RegionList;
    this->RegionList = NULL;
  } 

  this->NumRegions = 0;
  this->SetActualLevel();

  this->FreeCellLists();
}
void vtkKdTree::FreeCellLists()
{
  struct _cellList *list;
  int i;

  if ((list = this->CellList)){

    for (i=0; i<this->NumCellLists; i++){

      vtkKdTree::DeleteAllCellLists(list + i);

    }
    free(this->CellList);
    this->CellList = NULL;
  }
}

#define POLYGONAL_REPRESENTATION_PARTITIONS_WHOLE_SPACE
#ifdef POLYGONAL_REPRESENTATION_PARTITIONS_WHOLE_SPACE

// build PolyData representation of all spacial regions------------

void vtkKdTree::GenerateRepresentation(int level, vtkPolyData *pd)
{
  int i;

  vtkPoints *pts;
  vtkCellArray *polys;

  if ( this->Top == NULL )
  {
    vtkErrorMacro(<<"vtkKdTree::GenerateRepresentation empty tree");
    return;
  }

  if ((level < 0) || (level > this->Level)) level = this->Level;

  int npoints = 0;
  int npolys  = 0;

  for (i=0 ; i < level; i++){
    int levelPolys = 1 << (i-1);
    npoints += (4 * levelPolys);
    npolys += levelPolys;
  }

  pts = vtkPoints::New();
  pts->Allocate(npoints); 
  polys = vtkCellArray::New();
  polys->Allocate(npolys);

  // level 0 bounding box

  vtkIdType ids[8];
  vtkIdType idList[4];
  float     x[3];
  vtkKdNode    *kd    = this->Top;

  double *min = kd->Min;
  double *max = kd->Max;

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[0] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[1] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[2] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[3] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[4] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[5] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[6] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[7] = pts->InsertNextPoint(x);

  idList[0] = ids[0]; idList[1] = ids[1]; idList[2] = ids[2]; idList[3] = ids[3];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[1]; idList[1] = ids[5]; idList[2] = ids[6]; idList[3] = ids[2];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[5]; idList[1] = ids[4]; idList[2] = ids[7]; idList[3] = ids[6];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[4]; idList[1] = ids[0]; idList[2] = ids[3]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[3]; idList[1] = ids[2]; idList[2] = ids[6]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[1]; idList[1] = ids[0]; idList[2] = ids[4]; idList[3] = ids[5];
  polys->InsertNextCell(4, idList);

  if (kd->Left && (level > 0)){
      _generateRepresentation(kd, pts, polys, level-1);
  }

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}
void vtkKdTree::_generateRepresentation(vtkKdNode *kd, 
                                        vtkPoints *pts, 
                                        vtkCellArray *polys, 
                                        int level)
{
  int i;
  float p[4][3];
  vtkIdType ids[4];

  if ((level < 0) || (kd->Left == NULL)) return;

  double *min = kd->Min;
  double *max = kd->Max;  
  double *leftmax = kd->Left->Max;

  // splitting plane

  switch (kd->Dim){

    case xdim:

      p[0][0] = leftmax[0]; p[0][1] = max[1]; p[0][2] = max[2];
      p[1][0] = leftmax[0]; p[1][1] = max[1]; p[1][2] = min[2];
      p[2][0] = leftmax[0]; p[2][1] = min[1]; p[2][2] = min[2];
      p[3][0] = leftmax[0]; p[3][1] = min[1]; p[3][2] = max[2];

      break;

    case ydim:

      p[0][0] = min[0]; p[0][1] = leftmax[1]; p[0][2] = max[2];
      p[1][0] = min[0]; p[1][1] = leftmax[1]; p[1][2] = min[2];
      p[2][0] = max[0]; p[2][1] = leftmax[1]; p[2][2] = min[2];
      p[3][0] = max[0]; p[3][1] = leftmax[1]; p[3][2] = max[2];

      break;

    case zdim:

      p[0][0] = min[0]; p[0][1] = min[1]; p[0][2] = leftmax[2];
      p[1][0] = min[0]; p[1][1] = max[1]; p[1][2] = leftmax[2];
      p[2][0] = max[0]; p[2][1] = max[1]; p[2][2] = leftmax[2];
      p[3][0] = max[0]; p[3][1] = min[1]; p[3][2] = leftmax[2];

      break;
  }


  for (i=0; i<4; i++) ids[i] = pts->InsertNextPoint(p[i]);

  polys->InsertNextCell(4, ids);

  _generateRepresentation(kd->Left, pts, polys, level-1);
  _generateRepresentation(kd->Right, pts, polys, level-1);
}
#else

void vtkKdTree::GenerateRepresentation(int level, vtkPolyData *pd)
{
  int i;
  vtkPoints *pts;
  vtkCellArray *polys;

  if ( this->Top == NULL )
  {
    vtkErrorMacro(<<"vtkKdTree::GenerateRepresentation no tree");
    return;
  }

  if ((level < 0) || (level > this->Level)) level = this->Level;

  int npoints = 0;
  int npolys  = 0;

  for (i=0; i < level; i++){
    int levelBoxes= 1 << i;
    npoints += (8 * levelBoxes);
    npolys += (6 * levelBoxes);
  }

  pts = vtkPoints::New();
  pts->Allocate(npoints); 
  polys = vtkCellArray::New();
  polys->Allocate(npolys);

  _generateRepresentation(this->Top, pts, polys, level);

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}
void vtkKdTree::_generateRepresentation(vtkKdNode *kd, vtkPoints *pts,
                     vtkCellArray *polys, int level)
{
  vtkIdType ids[8];
  vtkIdType idList[4];
  float     x[3];

  if (level > 0){

      if (kd->Left){
           _generateRepresentation(kd->Left, pts, polys, level-1);
           _generateRepresentation(kd->Right, pts, polys, level-1);
      }

      return;
  }
  vtkKdTree::AddPolys(kd, pts, polys);
}
#endif

// PolyData rep. of all spacial regions, shrunk to data bounds-------

void vtkKdTree::AddPolys(vtkKdNode *kd, vtkPoints *pts, vtkCellArray *polys)
{
  vtkIdType ids[8];
  vtkIdType idList[4];
  float     x[3];

  double *min = kd->MinVal;
  double *max = kd->MaxVal;

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[0] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = min[2];
  ids[1] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[2] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = max[1]; x[2]  = max[2];
  ids[3] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[4] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = min[2];
  ids[5] = pts->InsertNextPoint(x);

  x[0]  = max[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[6] = pts->InsertNextPoint(x);

  x[0]  = min[0]; x[1]  = min[1]; x[2]  = max[2];
  ids[7] = pts->InsertNextPoint(x);

  idList[0] = ids[0]; idList[1] = ids[1]; idList[2] = ids[2]; idList[3] = ids[3];
  polys->InsertNextCell(4, idList);
  
  idList[0] = ids[1]; idList[1] = ids[5]; idList[2] = ids[6]; idList[3] = ids[2];
  polys->InsertNextCell(4, idList);
  
  idList[0] = ids[5]; idList[1] = ids[4]; idList[2] = ids[7]; idList[3] = ids[6];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[4]; idList[1] = ids[0]; idList[2] = ids[3]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);
  
  idList[0] = ids[3]; idList[1] = ids[2]; idList[2] = ids[6]; idList[3] = ids[7];
  polys->InsertNextCell(4, idList);

  idList[0] = ids[1]; idList[1] = ids[0]; idList[2] = ids[4]; idList[3] = ids[5];
  polys->InsertNextCell(4, idList);
}

// PolyData representation of a list of spacial regions------------

void vtkKdTree::GenerateRepresentation(int *regions, int len, vtkPolyData *pd)
{
  int i;
  vtkPoints *pts;
  vtkCellArray *polys;

  if ( this->Top == NULL )
  {
    vtkErrorMacro(<<"vtkKdTree::GenerateRepresentation no tree");
    return;
  }

  int npoints = 8 * len;
  int npolys  = 6 * len;

  pts = vtkPoints::New();
  pts->Allocate(npoints); 
  polys = vtkCellArray::New();
  polys->Allocate(npolys);

  for (i=0; i<len; i++){

    if ((regions[i] < 0) || (regions[i] >= this->NumRegions)) break;

    vtkKdTree::AddPolys(this->RegionList[regions[i]], pts, polys);

  }

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}
//  Cell ID lists ------------------------------------------------------
//  --------------------------------------------------------------------

#define SORTLIST(l, lsize) vtkstd::sort(l, l + lsize)

#define REMOVEDUPLICATES(l, lsize, newsize) \
{                                  \
int i,j;                           \
for (i=0, j=0; i<lsize; i++){    \
  if ((i > 0) && (l[i] == l[j-1])) continue; \
  if (j != i) l[j] = l[i];       \
  j++;                           \
}                                \
newsize = j;                     \
}
#define REMOVEFOUNDIN(l1, l1length, l2, l2length, newsize) \
{                                             \
int i,j,k;                                    \
j = k = 0;                                    \
for (i=0; i<l2length; i++){                     \
  while((j < l1length) && (l1[j] < l2[i])) j++; \
  if (l2[i] == l1[j]){                        \
    j++;                                      \
    continue;                                 \
  }                                           \
  if (i != k) l2[k] = l2[i];                  \
  k++;                                        \
}                                             \
newsize = k;                                  \
}
int vtkKdTree::findRegion(vtkKdNode *node, float x, float y, float z)
{
  int regionId;

  if  ( (x < node->Min[0]) || (x > node->Max[0]) ||
        (y < node->Min[1]) || (y > node->Max[1]) ||
        (z < node->Min[2]) || (z > node->Max[2]) ){

    return -1;
  }

  if (node->Left == NULL){
    regionId = node->Id;
  }
  else{
    regionId = vtkKdTree::findRegion(node->Left, x, y, z);

    if (regionId < 0) regionId = vtkKdTree::findRegion(node->Right, x, y, z);
  }

  return regionId;
}
void vtkKdTree::CreateCellList()
{
  this->CreateCellList(this->DataSets[0], (int *)NULL, 0);
  return;
}
void vtkKdTree::CreateCellList(vtkIdType *regionList, int listSize)
{
  this->CreateCellList(this->DataSets[0], regionList, listSize);
  return;
}
void vtkKdTree::CreateCellList(int dataSet, int *regionList, int listSize)
{
  if ((dataSet < 0) || (dataSet >= NumDataSets)){
    vtkErrorMacro(<<"vtkKdTree::CreateCellList invalid data set");
    return;
  }

  this->CreateCellList(this->DataSets[dataSet], regionList, listSize);
  return;
}
void vtkKdTree::CreateCellList(vtkDataSet *set, int *regionList, int listSize)
{
  int i, AllRegions, regionId;
  struct _cellList *list;

  if ( this->GetDataSet(set) < 0){
    vtkErrorMacro(<<"vtkKdTree::CreateCellList invalid data set");
    return;
  }

  if ((regionList == NULL) || (listSize == 0)) AllRegions = 1;
  else                                         AllRegions = 0;

  list = NULL;

  if (AllRegions && !this->GetCellCenters()){
    int fail = this->ComputeCellCenters();

    if (fail){
      vtkErrorMacro(<<"vtkKdTree::CreateCellList memory allocation");
      return;
    }
  }

  // the following ugly code is here to keep down the memory used
  // by the cell lists

  if (this->CellList == NULL){
    this->CellList = (struct _cellList *)malloc(sizeof(struct _cellList));

    if (!this->CellList){
      vtkErrorMacro(<<"vtkKdTree::CreateCellList memory allocation");
      return;
    }

    this->NumCellListsAllocated = 1;
    this->NumCellLists          = 1;

    list = this->CellList;

    list->cells = NULL;
  }
  else{
    for (i=0; i<this->NumCellLists; i++){

      if (this->CellList[i].dataSet == set){
        list = this->CellList + i;
        break;
      }
    }

    if (list == NULL){
      if (this->NumCellListsAllocated - this->NumCellLists <= 0){

        this->NumCellListsAllocated += 10;

        this->CellList = (struct _cellList *)realloc(this->CellList,
                       this->NumCellListsAllocated * sizeof(struct _cellList));

        if (!this->CellList){
          vtkErrorMacro(<<"vtkKdTree::CreateCellList memory allocation");
          return;
        }

      }
      list = this->CellList + this->NumCellLists;

      list->cells = NULL;
      this->NumCellLists++;
    }
  }

  if (list->cells == NULL){
    list->dataSet = set;
    list->regionIds = NULL;
    list->nRegions = 0;
  }

  int nHave = list->nRegions;
  int nWant = (AllRegions) ? this->NumRegions : listSize;
  int nNeed = 0;
  int *wantList = (int *)malloc(sizeof(int) * nWant);

  if (!wantList){
    vtkErrorMacro(<<"vtkKdTree::CreateCellList memory allocation");
    return;
  }

  if (AllRegions){
    for (i=0; i<this->NumRegions; i++) wantList[i] = i;
  }
  else{
    memcpy(wantList, regionList, sizeof(int) * nWant);
  }

  if (nHave > 0){  // remove regions already computed from list

    int *haveList = new int [nHave];
    memcpy(haveList, list->regionIds, sizeof(int) * nHave);

    SORTLIST(haveList, nHave);
    if (!AllRegions){
      SORTLIST(wantList, nWant);
      REMOVEDUPLICATES(wantList, nWant, nWant);
    }

    if (!((haveList[nHave-1] < wantList[0]) || 
          (haveList[0] > wantList[nWant-1]))){

      REMOVEFOUNDIN(haveList, nHave, wantList, nWant, nNeed);
    }
    else{
      nNeed = nWant;
    }

    delete [] haveList;
  }
  else{
    if (!AllRegions){
      SORTLIST(wantList, nWant);
      REMOVEDUPLICATES(wantList, nWant, nNeed);
    }
    else{
      nNeed = nWant;
    }
  }

  if (nNeed == 0){
     free(wantList);
     return;
  }

  if (nHave > 0){
    list->regionIds = (int *)realloc(list->regionIds, 
                                 (nHave+nNeed) * sizeof(int));

    if (!list->regionIds){
      vtkErrorMacro(<<"vtkKdTree::CreateCellList memory allocation");
      return;
    }
    memcpy(list->regionIds + nHave, wantList, sizeof(int) * nNeed);

    list->cells = (vtkIdList **)realloc(list->cells, 
                                 (nHave+nNeed) * sizeof(vtkIdList *));
    if (!list->cells){
      vtkErrorMacro(<<"vtkKdTree::CreateCellList memory allocation");
      return;
    }
    list->nRegions += nNeed;

    free(wantList);
  }
  else{
    list->regionIds = wantList;
    list->cells = (vtkIdList **)malloc(nNeed * sizeof(vtkIdList *));

    if (!list->cells){
      vtkErrorMacro(<<"vtkKdTree::CreateCellList memory allocation");
      return;
    }

    list->nRegions = nNeed;
  }

  vtkIdList **listptr = new vtkIdList * [this->NumRegions];

  if (!listptr){
    vtkErrorMacro(<<"vtkKdTree::CreateCellList memory allocation");
    return;
  }
  memset(listptr, 0, this->NumRegions * sizeof(vtkIdList *));

  for (i = nHave; i < list->nRegions; i++){

    list->cells[i] = vtkIdList::New();
    listptr[list->regionIds[i]] = list->cells[i];
  }

  int nCells = set->GetNumberOfCells();
  float center[3];

  for (i=0; i<nCells; i++){

    this->ComputeCellCenter(set, i, center);

    regionId = this->findRegion(this->Top, center[0], center[1], center[2]);

    if (regionId < 0){
      continue;   // should never happen
    }

    if (listptr[regionId] != NULL){
      listptr[regionId]->InsertNextId(i);
    }
  }

  delete [] listptr;
}
void vtkKdTree::DeleteCellList()
{
  this->DeleteCellList(this->DataSets[0], (int *)NULL, 0);
}
void vtkKdTree::DeleteCellList(int *regionList, int listSize)
{
  this->DeleteCellList(this->DataSets[0], regionList, listSize);
}
void vtkKdTree::DeleteCellList(int dataSet, int *regionList, int listSize)
{
  if ( (dataSet < 0) || (dataSet >= this->NumDataSets)){
    vtkErrorMacro(<< "vtkKdTree::DeleteCellList invalid data set");
    return;
  }

  this->DeleteCellList(this->DataSets[dataSet], regionList, listSize);
}
void vtkKdTree::DeleteCellList(vtkDataSet *set, int *regionList, int listSize)
{
  struct _cellList *list;
  int *rlist=NULL;
  int setNum;
  int ndelete;
  int i;

  if ( (setNum = this->GetDataSet(set)) < 0){
    vtkErrorMacro(<< "vtkKdTree::DeleteCellList invalid data set");
    return;
  }

  list = NULL;

  for (i=0; i<this->NumCellLists; i++){
    if (this->CellList[i].dataSet == set){
      list = this->CellList + i;
      break;
    }
  }  
  if (list == NULL) return;

  if ((regionList == NULL) || (listSize == 0)){  // delete all 
    ndelete = list->nRegions;
  }
  else{
    ndelete = 0;

    rlist = new int [listSize];
    memcpy(rlist, regionList, sizeof(int) * listSize);

    SORTLIST(rlist, listSize);
    REMOVEDUPLICATES(rlist, listSize, listSize);

    for (i=0; i<list->nRegions; i++){
      if (vtkKdTree::OnList(rlist, listSize, list->regionIds[i])){
        ndelete++;
      }
    }
  }

  if (ndelete == 0) return;

  if (ndelete == list->nRegions){

    vtkKdTree::DeleteAllCellLists(list);

    for (i=setNum+1; i<this->NumCellLists; i++){
      memcpy(this->CellList + i - 1, this->CellList + i, sizeof(struct _cellList));
    }
    this->NumCellLists--;
  }
  else{

    this->DeleteSomeCellLists(list, ndelete, rlist, listSize);

  }
  if (rlist) delete [] rlist;
}
vtkIdList * vtkKdTree::GetCellList(int regionID)
{
  return this->GetCellList(this->DataSets[0], regionID);
}
vtkIdList * vtkKdTree::GetCellList(int set, int regionID)
{
  if ( (set < 0) || (set >= this->NumDataSets)){
    vtkErrorMacro(<< "vtkKdTree::GetCellList invalid data set");
    return NULL;
  }

  return this->GetCellList(this->DataSets[set], regionID);
}
vtkIdList * vtkKdTree::GetCellList(vtkDataSet *set, int regionID)
{
  int setNum, i;
  struct _cellList *list = NULL;
  vtkIdList *cellIds = NULL;

  if ( (setNum = this->GetDataSet(set)) < 0){
     vtkErrorMacro(<<"vtkKdTree::GetCellList invalid data set");
     return NULL;
  }

  for (i=0; i<this->NumCellLists; i++){
    if (this->CellList[i].dataSet == set){
      list = this->CellList + i;
      break;
    }
  }

  if (list == NULL){
    vtkErrorMacro(<<"vtkKdTree::GetCellList no cell lists for this data set");
    return NULL;
  }

  for (i=0; i< list->nRegions; i++){
    if (list->regionIds[i] == regionID){
      cellIds = list->cells[i];
      break;
    }
  }

  if (cellIds == NULL){
    vtkErrorMacro(<<"vtkKdTree::GetCellList cell list not yet generated");
  }

  return cellIds;
}
int vtkKdTree::GetRegionContainingCell(vtkIdType cellID)
{
  return this->GetRegionContainingCell(this->DataSets[0], cellID);
}
int vtkKdTree::GetRegionContainingCell(int set, vtkIdType cellID)
{
  if ( (set < 0) || (set >= this->NumDataSets)){
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell no such data set");
    return -1;
  }
  return this->GetRegionContainingCell(this->DataSets[set], cellID);
}
int vtkKdTree::GetRegionContainingCell(vtkDataSet *set, vtkIdType cellID)
{
  float center[3];

  if ( this->GetDataSet(set) < 0){
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell no such data set");
    return -1;
  }
  if ( (cellID < 0) || (cellID >= set->GetNumberOfCells())){
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell bad cell ID");
    return -1;
  }
  this->ComputeCellCenter(set, cellID, center);

  return vtkKdTree::findRegion(this->Top, center[0], center[1], center[2]);
}
int vtkKdTree::GetRegionContainingPoint(float x, float y, float z)
{
  return vtkKdTree::findRegion(this->Top, x, y, z);
}

//  Query functions ----------------------------------------------------
//    K-d Trees are particularly efficient with region intersection
//    queries, like finding all regions that intersect a view frustum
//  --------------------------------------------------------------------

// Intersection with axis-aligned box----------------------------------

int vtkKdTree::IntersectsBox(int regionId, float *x)
{
  return this->IntersectsBox(regionId, (double)x[0], (double)x[1], 
                 (double)x[2], (double)x[3], (double)x[4], (double)x[5]);
}
int vtkKdTree::IntersectsBox(int regionId, double *x)
{
  return this->IntersectsBox(regionId, x[0], x[1], x[2], x[3], x[4], x[5]);
}
int vtkKdTree::IntersectsBox(int regionId, float x0, float x1, 
                           float y0, float y1, float z0, float z1)
{
  return this->IntersectsBox(regionId, (double)x0, (double)x1, 
                      (double)y0, (double)y1, (double)z0, (double)z1);
}
int vtkKdTree::IntersectsBox(int regionId, double x0, double x1, 
                           double y0, double y1, double z0, double z1)
{
  if ( (regionId < 0) || (regionId >= NumRegions)){
    vtkErrorMacro(<<"vtkKdTree::IntersectsBox invalid spatial region ID");
    return 0;
  }

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsBox(x0, x1, y0, y1, z0, z1);
}

int vtkKdTree::IntersectsBox(int *ids, int len, float *x)
{
  return this->IntersectsBox(ids, len,  (double)x[0], (double)x[1], 
                (double)x[2], (double)x[3], (double)x[4], (double)x[5]);
}
int vtkKdTree::IntersectsBox(int *ids, int len, double *x)
{
  return this->IntersectsBox(ids, len,  x[0], x[1], x[2], x[3], x[4], x[5]);
}
int vtkKdTree::IntersectsBox(int *ids, int len, 
                             float x0, float x1,
                             float y0, float y1, float z0, float z1)
{
  return this->IntersectsBox(ids, len,
              (double)x0, (double)x1, (double)y0, (double)y1, 
              (double)z0, (double)z1);
}
int vtkKdTree::IntersectsBox(int *ids, int len, 
                             double x0, double x1,
                             double y0, double y1, double z0, double z1)
{
  int nnodes = 0;

  if (len > 0){
    nnodes = this->_IntersectsBox(this->Top, ids, len,
                             x0, x1, y0, y1, z0, z1);
  }
  return nnodes;
}
int vtkKdTree::_IntersectsBox(vtkKdNode *node, int *ids, int len, 
                             double x0, double x1,
                             double y0, double y1, double z0, double z1)
{
  int result, nnodes1, nnodes2, listlen;
  int *idlist;

  result = node->IntersectsBox(x0, x1, y0, y1, z0, z1);

  if (!result) return 0;

  if (node->Left == NULL){
    ids[0] = node->Id;
    return 1;
  }

  nnodes1 = _IntersectsBox(node->Left, ids, len, x0, x1, y0, y1, z0, z1);

  idlist = ids + nnodes1;
  listlen = len - nnodes1;

  if (listlen > 0){
    nnodes2 = _IntersectsBox(node->Right, idlist, listlen, x0, x1, y0, y1, z0, z1);
  }
  else{
    nnodes2 = 0;
  }

  return (nnodes1 + nnodes2);
}

// Intersection with arbitrary convex region bounded by planes -----------

int vtkKdTree::IntersectsRegion(int regionId, vtkPlanes *planes)
{
  return this->IntersectsRegion(regionId, planes, 0, (double *)NULL);
}
int vtkKdTree::IntersectsRegion(int regionId, vtkPlanes *planes, 
                                int nvertices, float *vertices)
{
  int i;

  double *dv = new double[nvertices];
  for (i=0; i<nvertices; i++){
    dv[i] = vertices[i];
  }
  int intersects = this->IntersectsRegion(regionId, 
                                          planes, nvertices, dv);

  if (dv) delete [] dv;
  return intersects;
}
int vtkKdTree::IntersectsRegion(int regionId, vtkPlanes *planes, 
                                int nvertices, double *vertices)
{
  if ( (regionId < 0) || (regionId >= NumRegions)) {
    vtkErrorMacro(<<"vtkKdTree::IntersectsRegion invalid region ID");
    return 0;
  }

  vtkPlanesIntersection *pi = vtkPlanesIntersection::New();

  pi->SetNormals(planes->GetNormals());
  pi->SetPoints(planes->GetPoints());

  if (vertices && (nvertices>0)) pi->SetRegionVertices(vertices, nvertices);

  vtkKdNode *node = this->RegionList[regionId];

  int intersects = node->IntersectsRegion(pi);

  pi->Delete();

  return intersects;
}
int vtkKdTree::IntersectsRegion(int *ids, int len, vtkPlanes *planes) 
{
  return this->IntersectsRegion(ids, len, planes, 0, (double *)NULL);
}
int vtkKdTree::IntersectsRegion(int *ids, int len, vtkPlanes *planes, 
                                int nvertices, float *vertices)
{
  double *dv;
  int i;

  if (nvertices > 0){
    dv = new double[nvertices];
    for (i=0; i<nvertices; i++){
      dv[i] = vertices[i];
    }
  }
  else{
    dv = NULL;
  }
  int howmany = this->IntersectsRegion(ids, len, planes, 
                                     nvertices, dv);

  delete [] dv;

  return howmany; 
}
int vtkKdTree::IntersectsRegion(int *ids, int len, vtkPlanes *planes, 
                                int nvertices, double *vertices)
{
  int nnodes = 0;
  vtkPlanesIntersection *pi = vtkPlanesIntersection::New();

  pi->SetNormals(planes->GetNormals());
  pi->SetPoints(planes->GetPoints());

  if (vertices && (nvertices>0)) pi->SetRegionVertices(vertices, nvertices);

  if (len > 0){
    nnodes = _IntersectsRegion(this->Top, ids, len, pi);
  }

  pi->Delete();

  return nnodes;
}
int vtkKdTree::_IntersectsRegion(vtkKdNode *node, int *ids, int len, 
                                 vtkPlanesIntersection *pi)
{
  int result, nnodes1, nnodes2, listlen;
  int *idlist;

  result = node->IntersectsRegion(pi);

  if (!result) return 0;

  if (node->Left == NULL){
    ids[0] = node->Id;
    return 1;
  }

  nnodes1 = _IntersectsRegion(node->Left, ids, len, pi);

  idlist = ids + nnodes1;
  listlen = len - nnodes1;

  if (listlen > 0){
    nnodes2 = _IntersectsRegion(node->Right, idlist, listlen, pi);
  }
  else{
    nnodes2 = 0;
  }

  return (nnodes1 + nnodes2);
}

// Intersection with a view frustum that is a rectangular portion (or all)
//  of a viewport.  

int vtkKdTree::IntersectsFrustum(int regionId, vtkRenderer *ren, 
                       float x0, float x1, float y0, float y1)
{
  return this->IntersectsFrustum(regionId, ren,
               (double)x0, (double)x1, (double)y0, (double)y1);

}
int vtkKdTree::IntersectsFrustum(int regionId, vtkRenderer *ren, 
                       double x0, double x1, double y0, double y1)
{
  if ( (x0 < -1) || (x1 > 1) || (y0 < -1) || (y1 > 1)){
    vtkErrorMacro(<<"vtkKdTree::IntersectsFrustum values don't appear to be view coordinates ([-1,1], [-1,1])");
    return 0;
  }

  if ( (regionId < 0) || (regionId >= NumRegions)){
    vtkErrorMacro(<<"vtkKdTree::IntersectsFrustum invalid region ID");
    return 0;
  }

  vtkPlanesIntersection *planes = vtkKdTree::ConvertFrustumToWorld(ren,
                             x0, x1, y0, y1);

  vtkKdNode *node = this->RegionList[regionId];

  int intersects = node->IntersectsRegion(planes);

  return intersects;
}
int vtkKdTree::IntersectsFrustum(int *ids, int len, vtkRenderer *ren, 
                       float x0, float x1, float y0, float y1)
{
  return this->IntersectsFrustum(ids, len, ren, (double)x0, (double)x1,
                       (double)y0, (double)y1);
}
int vtkKdTree::IntersectsFrustum(int *ids, int len, vtkRenderer *ren, 
                       double x0, double x1, double y0, double y1)
{
  if ( (x0 < -1) || (x1 > 1) || (y0 < -1) || (y1 > 1)){
    vtkErrorMacro(<<"vtkKdTree::IntersectsFrustum values don't appear to be view coordinates ([-1,1], [-1,1])");
    return 0;
  }

  vtkPlanesIntersection *planes = vtkKdTree::ConvertFrustumToWorld(ren,
                             x0, x1, y0, y1);

  int howmany = _IntersectsRegion(this->Top, ids, len, planes);

  planes->Delete();

  return howmany;
}
#define vectorDifference(v1, v2, v3) \
{v3[0] = v2[0]-v1[0]; v3[1] = v2[1]-v1[1]; v3[2] = v2[2]-v1[2];}

float vtkKdTree::sensibleZcoordinate(vtkRenderer *ren)
{
  float FP[3];

  ren->GetActiveCamera()->GetFocalPoint(FP);
  ren->WorldToView(FP[0], FP[1], FP[2]);
  return FP[2];
}
void vtkKdTree::computeNormal(float *p1, float *p2, float *p3, float N[3])
{
  float v1[3], v2[3];

  // (p1 - p2) X (p3 - p2)

  vectorDifference(p2, p1, v1);
  vectorDifference(p2, p3, v2);
  vtkMath::Cross(v1, v2, N);

  return;
}
void vtkKdTree::pointOnPlane(float a, float b, float c, float d, float pt[3])
{
  float px = ( a > 0.0) ? a : -a;
  float py = ( b > 0.0) ? b : -b;
  float pz = ( c > 0.0) ? c : -c;

  pt[0] = pt[1] = pt[2] = 0.0;

  if ((py >= px) && (py >= pz)){
    pt[1] = -(d/b);
  }
  else if ((px >= py) && (px >= pz)){
    pt[0] = -(d/a);
  }
  else{
    pt[2] = -(d/c);
  }
  return;
}

vtkPlanesIntersection *vtkKdTree::ConvertFrustumToWorldPerspective(
                      vtkRenderer *ren, 
                      double xmin, double xmax, double ymin, double ymax)
{
  int i;
  float planeEq[24];
  float worldN[3], newPt[3];

  int *winsize = ren->GetRenderWindow()->GetSize();

  ren->GetActiveCamera()->GetFrustumPlanes(winsize[0]/winsize[1], planeEq);

  vtkFloatArray *newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->SetNumberOfTuples(6);

  vtkPoints *newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(6);

  // For each plane of frustum, compute outward pointing normal
  // and a point on the plane.  This is for a frustum defined
  // by a rectangular sub-region of the viewport.

  // First, front and back clipping planes.  We use the fact that
  //   vtkCamera returns the frustum planes in a particular order,
  //   4th equation is for near clip plane, 5th is for far.

  for (i=4; i<6; i++){

    float *plane = planeEq + 4*i;

    worldN[0] = -plane[0];
    worldN[1] = -plane[1];
    worldN[2] = -plane[2];

    newNormals->SetTuple(i, worldN);

    pointOnPlane(plane[0], plane[1], plane[2], plane[3], newPt);

    newPts->SetPoint(i, newPt);
  }
  // Then the left, right, lower, and upper frustum planes.
  // Get three points on each plane, so we can get a normal.

  float ulFront[3], urFront[3], llFront[3], lrFront[3];
  float lrBack[3], ulBack[3];

  // Need a sensible z (took me 3 days to figure that out)

  float sensibleZ = sensibleZcoordinate(ren);
    
  // upper left
  ulFront[0] = xmin; ulFront[1] = ymax; ulFront[2] = sensibleZ;
  ren->ViewToWorld(ulFront[0], ulFront[1], ulFront[2]);

  // upper right
  urFront[0] = xmax; urFront[1] = ymax; urFront[2] = sensibleZ;
  ren->ViewToWorld(urFront[0], urFront[1], urFront[2]);
    
  // lower left
  llFront[0] = xmin; llFront[1] = ymin; llFront[2] = sensibleZ;
  ren->ViewToWorld(llFront[0], llFront[1], llFront[2]);

  // lower right
  lrFront[0] = xmax; lrFront[1] = ymin; lrFront[2] = sensibleZ;
  ren->ViewToWorld(lrFront[0], lrFront[1], lrFront[2]);

  //  We need second, different reasonable z value for back points

  double *dir = ren->GetActiveCamera()->GetDirectionOfProjection();

  float dx = dir[0] * .01;
  float dy = dir[1] * .01;
  float dz = dir[2] * .01;

  ulBack[0] = ulFront[0] + dx;
  ulBack[1] = ulFront[1] + dy;
  ulBack[2] = ulFront[2] + dz;

  ren->WorldToView(ulBack[0], ulBack[1], ulBack[2]);
  ulBack[0] = xmin;
  ulBack[1] = ymax;
  ren->ViewToWorld(ulBack[0], ulBack[1], ulBack[2]);
  
  lrBack[0] = lrFront[0] + dx;
  lrBack[1] = lrFront[1] + dy;
  lrBack[2] = lrFront[2] + dz;
  
  ren->WorldToView(lrBack[0], lrBack[1], lrBack[2]);
  lrBack[0] = xmax;
  lrBack[1] = ymin;
  ren->ViewToWorld(lrBack[0], lrBack[1], lrBack[2]);

  // left plane
  
  computeNormal(ulBack, ulFront, llFront, worldN);
  newNormals->SetTuple(0, worldN);
  newPts->SetPoint(0, ulFront);
  
  // right plane
  
  computeNormal(lrBack, lrFront, urFront, worldN);
  newNormals->SetTuple(1, worldN);
  newPts->SetPoint(1, urFront);
  
  // lower plane
  
  computeNormal(llFront, lrFront, lrBack, worldN);
  newNormals->SetTuple(2, worldN);
  newPts->SetPoint(2, lrFront);

  // upper plane

  computeNormal(urFront, ulFront, ulBack, worldN);
  newNormals->SetTuple(3, worldN);
  newPts->SetPoint(3, urFront);

  vtkPlanesIntersection *planes = vtkPlanesIntersection::New();
  planes->SetPoints(newPts);
  planes->SetNormals(newNormals);

  newPts->Delete();
  newNormals->Delete();

  return planes;
}
vtkPlanesIntersection *vtkKdTree::ConvertFrustumToWorldParallel(
                      vtkRenderer *ren, 
                      double xmin, double xmax, double ymin, double ymax)
{
  float planeEq[24];
  float newPt[3], worldN[3];
  int i;

  int *winsize = ren->GetRenderWindow()->GetSize();
      
  ren->GetActiveCamera()->GetFrustumPlanes(winsize[0]/winsize[1], planeEq);
          
  vtkFloatArray *newNormals = vtkFloatArray::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->SetNumberOfTuples(6);
        
  vtkPoints *newPts = vtkPoints::New();
  newPts->SetNumberOfPoints(6);

  float sensibleZ = sensibleZcoordinate(ren);

  float *plane = planeEq;

  for (i=0; i<6; i++){

    // outward pointing normal

    worldN[0] = -plane[0];
    worldN[1] = -plane[1];
    worldN[2] = -plane[2];

    newNormals->SetTuple(i, worldN);

    // Get a point on this plane
    //   We use the fact that vtkCamera gives us the frustum planes in
    //   this order: left, right, lower, upper, front clip plane, back
    //   clip plane.

    if (i < 4){
      switch (i){
        case 0:
          newPt[0] = xmin;
          newPt[1] = 0.0;
          newPt[2] = sensibleZ;
          break;
        case 1:
          newPt[0] = xmax;
          newPt[1] = 0.0;
          newPt[2] = sensibleZ;
          break;
        case 2:
          newPt[0] = 0.0;
          newPt[1] = ymin;
          newPt[2] = sensibleZ;
          break;
        case 3:
          newPt[0] = 0.0;
          newPt[1] = ymax;
          newPt[2] = sensibleZ;
          break;
      }
      ren->ViewToWorld(newPt[0], newPt[1], newPt[2]);
    }
    else{
      pointOnPlane(plane[0], plane[1], plane[2], plane[3], newPt);
    }
    plane += 4;
    newPts->SetPoint(i, newPt);
  }

  vtkPlanesIntersection *planes = vtkPlanesIntersection::New();
  planes->SetPoints(newPts);
  planes->SetNormals(newNormals);

  newPts->Delete();
  newNormals->Delete();

  return planes;
}

vtkPlanesIntersection *vtkKdTree::ConvertFrustumToWorld(vtkRenderer *ren, 
                      double x0, double x1, double y0, double y1)
{
  vtkPlanesIntersection *planes;

  if (ren->GetActiveCamera()->GetParallelProjection()){
    planes = ConvertFrustumToWorldParallel(ren, x0, x1, y0, y1);
  }
  else{
    planes = ConvertFrustumToWorldPerspective(ren, x0, x1, y0, y1);
  }

  return planes;
}


//---------------------------------------------------------------------------
//void vtkKdTree::AddUser(vtkObject *c)
//{
//  // make sure it isn't already there
//  if (this->IsUser(c))
//    {
//    return;
//    }
//  // add it to the list, reallocate memory
//  vtkObject **tmp = this->Users;
//  this->NumberOfUsers++;
//  this->Users = new vtkObject* [this->NumberOfUsers];
//  for (int i = 0; i < (this->NumberOfUsers-1); i++)
//    {
//    this->Users[i] = tmp[i];
//    }
//  this->Users[this->NumberOfUsers-1] = c;
//  // free old memory
//  delete [] tmp;
//}

//void vtkKdTree::RemoveUser(vtkObject *c)
//{
//  // make sure it is already there
//  if (!this->IsUser(c))
//    {
//    return;
//    }
//  // remove it from the list, reallocate memory
//  vtkObject **tmp = this->Users;
//  this->NumberOfUsers--;
//  this->Users = new vtkObject* [this->NumberOfUsers];
//  int cnt = 0;
//  int i;
//  for (i = 0; i <= this->NumberOfUsers; i++)
//    {
//    if (tmp[i] != c)
//      {
//      this->Users[cnt] = tmp[i];
//      cnt++;
//      }
//    }
//  // free old memory
//  delete [] tmp;
//} 
  
//int vtkKdTree::IsUser(vtkObject *c)
//{ 
//  int i;
//  for (i = 0; i < this->NumberOfUsers; i++)
//    {
//    if (this->Users[i] == c)
//      { 
//      return 1;
//      } 
//    }
//  return 0;
//}

//vtkObject *vtkKdTree::GetUser(int i)
//{ 
//  if (i >= this->NumberOfUsers)
//    {
//    return 0;
//    }
//  return this->Users[i];
//} 

//void vtkKdTree::UpdateUserMTimes()
//{
//  for (int i=0; i<this->NumberOfUsers; i++){
//    vtkObject *user = this->GetUser(i);
//    // Modifying the DistributeDataFilter wich is updating
//    // causes problems.  I would check the Updating flag,
//    // but is not public. (Charles).
//    //user->Modified();
//  }
//}

//---------------------------------------------------------------------------

void vtkKdTree::PrintTiming(ostream& os, vtkIndent indent)
{
  vtkTimerLog::DumpLogWithIndents(&os, (float)0.0);
}

void vtkKdTree::PrintSelf(ostream& os, vtkIndent indent)
{ 
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DataSets: " << this->DataSets << endl;
  os << indent << "NumDataSets: " << this->NumDataSets << endl;
  os << indent << "NumDataSetsAllocated: " << this->NumDataSetsAllocated << endl;

  os << indent << "MinCells: " << this->MinCells << endl;

  os << indent << "NumRegions: " << this->NumRegions << endl;
  os << indent << "RegionList: " << this->RegionList << endl;

  os << indent << "ValidDirections: " << this->ValidDirections << endl;

  os << indent << "Top: " << this->Top << endl;
  os << indent << "RegionList: " << this->RegionList << endl;

  os << indent << "CellCenters: " << this->CellCenters << endl;
  os << indent << "CellCentersSize: " << this->CellCentersSize << endl;
  os << indent << "RetainCellLocations: " << this->RetainCellLocations << endl;

  os << indent << "CellList: " << this->CellList << endl;
  os << indent << "NumCellLists: " << this->NumCellLists << endl;
  os << indent << "NumCellListsAllocated: " << this->NumCellListsAllocated << endl;

  os << indent << "Timing: " << this->Timing << endl;
  os << indent << "TimerLog: " << this->TimerLog << endl;

  //os << indent << "NumberOfUsers: " << this->NumberOfUsers << endl;
  //os << indent << "Users: " << this->Users << endl;
  os << indent << "Level: " << this->Level << endl;
  os << indent << "MaxLevel: " << this->MaxLevel << endl;

}


