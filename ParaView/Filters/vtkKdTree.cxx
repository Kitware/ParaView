// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:  vtkKdTree.cxx
  Language:  C++
  Date:    $Date$
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

#include "vtkKdTree.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkCamera.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkPlanesIntersection.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkIdList.h"
#include "vtkTimerLog.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"

#include <algorithm>

vtkCxxRevisionMacro(vtkKdTree, "1.8.4.4");

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
  this->MinId = -1;
  this->MaxId = -1;
  this->NumCells = 0;

  this->cellBoundsCache = NULL;
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
void vtkKdNode::GetBounds(float *b) const
{
   b[0] = (float)this->Min[0]; b[1] = (float)this->Max[0];
   b[2] = (float)this->Min[1]; b[3] = (float)this->Max[1];
   b[4] = (float)this->Min[2]; b[5] = (float)this->Max[2];
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

  vtkIdType numCells = this->GetNumberOfCells();

  int i;

  if (this->Up)
    {
    double bounds[6];

    this->Up->GetDataBounds(bounds);
   
    int dim = this->Up->GetDim();
   
    for (i=0; i<3; i++)
      {
      if (i == dim) continue;

      newbounds[i*2]  = bounds[i*2];
      newbounds[i*2+1] = bounds[i*2+1];
      } 

    newbounds[dim*2] = newbounds[dim*2+1] = (double)v[dim];

    for (i = dim+3; i< numCells*3; i+=3)
      {
      if (v[i] < newbounds[dim*2]) newbounds[dim*2] = (double)v[i];
      else if (v[i] > newbounds[dim*2+1]) newbounds[dim*2+1] = (double)v[i];
      }
    }
  else
    {
    for (i=0; i<3; i++)
      {
      newbounds[i*2] = newbounds[i*2+1] = (double)v[i];
      }

    for (x = 3; x< numCells*3; x+=3)
      {
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
void vtkKdNode::GetDataBounds(float *b) const
{     
   b[0] = (float)this->MinVal[0]; b[1] = (float)this->MaxVal[0];
   b[2] = (float)this->MinVal[1]; b[3] = (float)this->MaxVal[1];
   b[4] = (float)this->MinVal[2]; b[5] = (float)this->MaxVal[2];
}     
void vtkKdNode::AddChildNodes(vtkKdNode *left, vtkKdNode *right)
{     
  this->Left = left;
  this->Right = right;
      
  right->Up = this;
  left->Up  = this;
}       
int vtkKdNode::IntersectsBox(float x0, float x1, float y0, float y1,
                         float z0, float z1, int useDataBounds=0)
{
  return this->IntersectsBox((double)x0, (double)x1, (double)y0, (double)y1,
                         (double)z0, (double)z1, useDataBounds);
}
int vtkKdNode::IntersectsBox(double x0, double x1, double y0, double y1,
                         double z0, double z1, int useDataBounds=0)
{
  double *min, *max;
   
  if (useDataBounds)
    {
    min = this->MinVal;
    max = this->MaxVal;
    }
  else
    {
    min = this->Min;
    max = this->Max;
    }

  if ( (min[0] >= x1) ||
       (max[0] <= x0) ||
       (min[1] >= y1) ||
       (max[1] <= y0) ||
       (min[2] >= z1) ||
       (max[2] <= z0))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}
int vtkKdNode::ContainsBox(float x0, float x1, float y0, float y1,
                         float z0, float z1, int useDataBounds=0)
{
  return this->ContainsBox((double)x0, (double)x1, (double)y0, (double)y1,
                         (double)z0, (double)z1, useDataBounds);
}
int vtkKdNode::ContainsBox(double x0, double x1, double y0, double y1,
                         double z0, double z1, int useDataBounds=0)
{
  double *min, *max;

  if (useDataBounds)
    {
    min = this->MinVal;
    max = this->MaxVal;
    }
  else
    {
    min = this->Min;
    max = this->Max;
    }

  if ( (min[0] > x0) ||
       (max[0] < x1) ||
       (min[1] > y0) ||
       (max[1] < y1) ||
       (min[2] > z0) ||
       (max[2] < z1))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}
int vtkKdNode::ContainsPoint(float x, float y, float z, int useDataBounds=0)
{
  return this->ContainsPoint((double)x, (double)y, (double)z, useDataBounds);
}
int vtkKdNode::ContainsPoint(double x, double y, double z, int useDataBounds=0)
{
  double *min, *max;

  if (useDataBounds)
    {
    min = this->MinVal;
    max = this->MaxVal;
    }
  else
    {
    min = this->Min;
    max = this->Max;
    }

  // points on a boundary are arbitrarily assigned to the region
  // for which they are on the upper boundary

  if ( (min[0] >= x) ||
       (max[0] < x) ||
       (min[1] >= y) ||
       (max[1] < y) ||
       (min[2] >= z) ||
       (max[2] < z))
    {
    return 0;
    }
  else
    {
    return 1;
    }
}

int vtkKdNode::IntersectsRegion(vtkPlanesIntersection *pi, int useDataBounds)
{
  double x0, x1, y0, y1, z0, z1;
  vtkPoints *box = vtkPoints::New();

  box->SetNumberOfPoints(8);

  double *min, *max;

  if (useDataBounds)
    {
    min = this->MinVal;
    max = this->MaxVal;
    }
  else
    {
    min = this->Min;
    max = this->Max;
    }

  x0 = min[0]; x1 = max[0];
  y0 = min[1]; y1 = max[1];
  z0 = min[2]; z1 = max[2];

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
int vtkKdNode::IntersectsCell(vtkCell *cell, int useDataBounds, int cellRegion)
{   
  int i;
  
  if ((useDataBounds==0) && (cellRegion >= 0))
    {
    if ( (cellRegion >= this->MinId) && (cellRegion <= this->MaxId))
      {
      return 1;    // the cell centroid is contained in this spatial region
      }
    }

  float *cellBounds;
  int deleteCellBounds = (this->cellBoundsCache == NULL);
  
  if (deleteCellBounds)
    {
    cellBounds = new float [6];
    vtkKdTree::SetCellBounds(cell, cellBounds);
    }
  else
    {
    cellBounds = this->cellBoundsCache;
    }

  int intersects = -1;

  if (!this->IntersectsBox(cellBounds[0], cellBounds[1],
                           cellBounds[2], cellBounds[3],
                           cellBounds[4], cellBounds[5], useDataBounds) )
    {
    intersects = 0;   // cell bounding box is outside region
    }
  else if ( this->ContainsBox(cellBounds[0], cellBounds[1],
                         cellBounds[2], cellBounds[3],
                         cellBounds[4], cellBounds[5], useDataBounds) )
    {
    intersects = 1;  // cell bounding box is completely inside region
    }
  
  if (intersects != -1)
    { 
    if (deleteCellBounds)
      {
      delete [] cellBounds;
      }
    return intersects;
    }

  int dim = cell->GetCellDimension();

  vtkPoints *pts = cell->Points;
  int npts = pts->GetNumberOfPoints();

  // determine if cell intersects region

  intersects = 0;

  if (dim == 0)    // points
    {
    float *pt = pts->GetPoint(0);

    for (i=0; i < npts ; i++)
      {
      if (this->ContainsPoint(pt[0],pt[1],pt[2], useDataBounds))
        {
        intersects = 1;
        break;
        }
      pt += 3;
      }
    }
  else if (dim == 1)    // lines
    {
    float *p2 = pts->GetPoint(0);
    float *p1;
    float dir[3], x[3], t;

    float regionBounds[6];

    this->GetBounds(regionBounds);

    for (i=0; i < npts - 1 ; i++)
      {
      p1 = p2;
      p2 = p1 + 3;

      dir[0] = p2[0] - p1[0]; dir[1] = p2[1] - p1[1]; dir[2] = p2[2] - p1[2];

      intersects = vtkCell::HitBBox(regionBounds, p1, dir, x, t);

      if (intersects) break;
      }
    }
  else if (dim == 2)     // polygons
    {
    double *min, *max;

    if (useDataBounds)
      {
      min = this->MinVal;
      max = this->MaxVal;
      } 
    else
      {
      min = this->Min;
      max = this->Max;
      }
    float regionBounds[6];

    regionBounds[0] = min[0], regionBounds[1] = max[0];
    regionBounds[2] = min[1], regionBounds[3] = max[1];
    regionBounds[4] = min[2], regionBounds[5] = max[2];

    if (cell->GetCellType() == VTK_TRIANGLE_STRIP)
      {
      vtkPoints *triangle = vtkPoints::New();

      triangle->SetNumberOfPoints(3);

      triangle->SetPoint(0, pts->GetPoint(0));
      triangle->SetPoint(1, pts->GetPoint(1));
      
      int newpoint = 2;
      
      for (i=2; i<npts; i++)
        {
        triangle->SetPoint(newpoint, pts->GetPoint(i));
  
        newpoint = (newpoint == 2) ? 0 : newpoint+1;
    
        intersects =
          vtkPlanesIntersection::PolygonIntersectsBBox(regionBounds, triangle);

        if (intersects)
          {
          break;
          }
        }
      triangle->Delete();
      } 
    else
      {
      intersects =
        vtkPlanesIntersection::PolygonIntersectsBBox(regionBounds, pts);
      }
    }   
  else if (dim == 3)     // 3D cells
    {
    vtkPlanesIntersection *pi = vtkPlanesIntersection::Convert3DCell(cell);

    intersects = this->IntersectsRegion(pi, useDataBounds);
    
    pi->Delete();
    }
  
  if (deleteCellBounds)
    {
    delete [] cellBounds;
    }
  
  return intersects;
}

void vtkKdNode::PrintNode(int depth)
{
  if ( (depth < 0) || (depth > 19)) depth = 19;
      
  if (this->Id > -1)
    {
    printf("%s x (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f) - %d cells, #%d %s\n",
      vtkKdNode::LevelMarker[depth],
      this->Min[0], this->Max[0],
      this->Min[1], this->Max[1],
      this->Min[2], this->Max[2],
      this->NumCells, this->Id, this->Left ? "" : "(leaf node)" );
    }
  else
    {
    printf("%s x (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f) - %d cells, #%d-%d %s\n",
      vtkKdNode::LevelMarker[depth],
      this->Min[0], this->Max[0],
      this->Min[1], this->Max[1],
      this->Min[2], this->Max[2],
      this->NumCells, this->MinId, this->MaxId, this->Left ? "" : "(leaf node)" );
    }
}
void vtkKdNode::PrintVerboseNode(int depth)
{
  if ( (depth < 0) || (depth > 19)) depth = 19;
      
  printf("%s Space (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f)\n",
      vtkKdNode::LevelMarker[depth],
      this->Min[0], this->Max[0],
      this->Min[1], this->Max[1],
      this->Min[2], this->Max[2]);
  printf("%s Data (%.4f, %.4f) y (%.4f %.4f) z (%.4f %.4f)\n",
      vtkKdNode::LevelMarker[depth],
      this->MinVal[0], this->MaxVal[0],
      this->MinVal[1], this->MaxVal[1],
      this->MinVal[2], this->MaxVal[2]);
  if (this->Id == -1)
    {
    printf("%s %d cells, id range %d - %d, cut next along %d, left %p, right %p, up %p\n",
      vtkKdNode::LevelMarker[depth],                                                              this->NumCells, this->MinId, this->MaxId, this->Dim, this->Left, this->Right, this->Up);
    }
  else
    {
    printf("%s %d cells, id %d, cut next along %d, left %p, right %p, up %p\n",
      vtkKdNode::LevelMarker[depth],
      this->NumCells, this->Id, this->Dim, this->Left, this->Right, this->Up);
    }
  printf("%s dim: %d\n\n",vtkKdNode::LevelMarker[depth],this->Dim);
}
// end of vtkKdNode -------------------------------------------

// Timing data ---------------------------------------------

#include "vtkTimerLog.h"

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

#define TIMER(s)         \
  if (this->Timing)      \
    {                    \
    char *s2 = makeEntry(s);               \
    if (this->TimerLog == NULL){           \
      this->TimerLog = vtkTimerLog::New(); \
      }                                    \
    this->TimerLog->MarkStartEvent(s2);    \
    }

#define TIMERDONE(s) \
  if (this->Timing){ char *s2 = makeEntry(s); this->TimerLog->MarkEndEvent(s2); }

// Timing data ---------------------------------------------


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
  this->MaxLevel = 20;
  this->Level    = 0;

  this->ValidDirections =
  (1 << vtkKdTree::xdim) | (1 << vtkKdTree::ydim) | (1 << vtkKdTree::zdim);

  this->MinCells = 100;
  this->NumRegions     = 0;

  this->DataSets = NULL;
  this->NumDataSets = 0;

  this->Top      = NULL;
  this->RegionList   = NULL;

  this->Timing = 0;
  this->TimerLog = NULL;

  this->NumDataSetsAllocated = 0;
  this->IncludeRegionBoundaryCells = 0;
  this->GenerateRepresentationUsingDataBounds = 0;
  this->ComputeIntersectionsUsingDataBounds = 0;

  this->InitializeCellLists();
  this->CellRegionList = NULL;
}
void vtkKdTree::DeleteNodes(vtkKdNode *nd)
{   
  if (nd->Left)
    {
     vtkKdTree::DeleteNodes(nd->Left);
     delete nd->Left;
     nd->Left = NULL;
    }
  if (nd->Right)
    {
     vtkKdTree::DeleteNodes(nd->Right);
     delete nd->Right;
     nd->Right = NULL;
    }
  return;
}
void vtkKdTree::InitializeCellLists()
{ 
  this->CellList.dataSet       = NULL;
  this->CellList.regionIds     = NULL;
  this->CellList.nRegions      = 0;
  this->CellList.cells         = NULL;
  this->CellList.boundaryCells = NULL;
}
void vtkKdTree::DeleteCellLists()
{ 
  int i;
  int num = this->CellList.nRegions;
  
  if (this->CellList.regionIds)
    {
    delete [] this->CellList.regionIds;
    }
  
  if (this->CellList.cells)
    {
    for (i=0; i<num; i++)
      {
      this->CellList.cells[i]->Delete();
      }

    delete [] this->CellList.cells;
    }

  if (this->CellList.boundaryCells) 
    {
    for (i=0; i<num; i++)
      {
      this->CellList.boundaryCells[i]->Delete();
      }
    delete [] this->CellList.boundaryCells;
    }

  this->InitializeCellLists();

  return;
}
vtkKdTree::~vtkKdTree()
{
  if (this->DataSets) free (this->DataSets);

  this->FreeSearchStructure();

  this->DeleteCellLists();

  if (this->CellRegionList)
    {
    delete [] this->CellRegionList;
    this->CellRegionList = NULL;
    }

  if (this->TimerLog)
    {
    this->TimerLog->Delete();
    }

  return;
}

void vtkKdTree::Modified()
{
  vtkLocator::Modified();    // k-d tree must be re-built
}

void vtkKdTree::SetDataSet(vtkDataSet *set)
{
  int i;
  for (i=0; i<this->NumDataSets; i++)
    {
    if (this->DataSets[i] == set) return;
    }    

  if (this->NumDataSetsAllocated == 0)
    {
    this->DataSets = (vtkDataSet **)malloc(sizeof(vtkDataSet *));

    if (!this->DataSets)
      {
      vtkErrorMacro(<<"vtkKdTree::SetDataSet memory allocation");
      return;
      }

    vtkLocator::SetDataSet(set);

    this->NumDataSetsAllocated = 1;
    this->NumDataSets          = 0;
    }
  else if (this->NumDataSetsAllocated - this->NumDataSets <= 0)
    {
    this->NumDataSetsAllocated += 10;

    this->DataSets = (vtkDataSet **)
        realloc(this->DataSets, this->NumDataSetsAllocated * sizeof(vtkDataSet *));

    if (!this->DataSets)
      {
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
  for (i=0; i<this->NumDataSets; i++)
    {
    if (this->DataSets[i] == set)
      {
       removeSet = i;
       break;
      }
    }
  if (removeSet >= 0)
    {
    this->RemoveDataSet(removeSet);
    }
  else
    {
    vtkErrorMacro( << "vtkKdTree::RemoveDataSet not a valid data set");
    }
}
void vtkKdTree::RemoveDataSet(int which)
{
  int i;

  if ( (which < 0) || (which >= this->NumDataSets))
    {
    vtkErrorMacro( << "vtkKdTree::RemoveDataSet not a valid data set");
    return;
    }

  if (this->CellList.dataSet == this->DataSets[which])
    {
    this->DeleteCellLists();
    }

  for (i=which; i<this->NumDataSets-1; i++)
    {
    this->DataSets[i] = this->DataSets[i+1];
    }
  this->NumDataSets--;
  this->Modified();
}
int vtkKdTree::GetDataSet(vtkDataSet *set)
{
  int i;
  int whichSet = -1;

  for (i=0; i<this->NumDataSets; i++)
    {
    if (this->DataSets[i] == set)
      {
      whichSet = i;
      break;
      } 
    }
  return whichSet;
}
int vtkKdTree::GetDataSetsNumberOfCells(int from, int to)
{
  int numCells = 0;
    
  for (int i=from; i<=to; i++)
    {
    numCells += this->DataSets[i]->GetNumberOfCells();
    }
  
  return numCells;
}
int vtkKdTree::GetNumberOfCells()
{ 
  return this->GetDataSetsNumberOfCells(0, this->NumDataSets - 1);
}
void vtkKdTree::GetRegionBounds(int regionID, float bounds[6])
{
  if ( (regionID < 0) || (regionID >= this->NumRegions))
    {
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
  if ( (regionID < 0) || (regionID >= this->NumRegions))
    {
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
  if (level > 0)
    {
    vtkKdNode **nodes0 = _GetRegionsAtLevel(level-1, nodes, kd->Left);
    vtkKdNode **nodes1 = _GetRegionsAtLevel(level-1, nodes0, kd->Right);

    return nodes1;
    }
  else
    {
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
void vtkKdTree::GetLeafNodeIds(vtkKdNode *node, vtkIntArray *ids)
{
  if (node->Id < 0)
    {
    vtkKdTree::GetLeafNodeIds(node->Left, ids);
    vtkKdTree::GetLeafNodeIds(node->Right, ids);
    }
  else
    {
    ids->InsertNextValue(node->Id);
    }
  return;
}

float *vtkKdTree::ComputeCellCenters()
{
  vtkDataSet *allSets = NULL;
  return this->ComputeCellCenters(allSets);
}
float *vtkKdTree::ComputeCellCenters(int set)
{
  if ( (set < 0) || (set >= this->NumDataSets))
    {
    vtkErrorMacro(<<"vtkKdTree::ComputeCellCenters no such data set");
    return NULL;
    }
  return this->ComputeCellCenters(this->DataSets[set]);
}
float *vtkKdTree::ComputeCellCenters(vtkDataSet *set)
{
  int i,j;
  int totalCells;

  if (set)
    {
    totalCells = set->GetNumberOfCells();
    }
  else
    {
    totalCells = this->GetNumberOfCells();   // all data sets
    }

  if (totalCells == 0) return NULL;

  float *center = new float [3 * totalCells];

  if (!center)
    {
    return NULL;
    }

  int maxCellSize = 0;

  if (set)
    {
      maxCellSize = set->GetMaxCellSize();
    }
  else
    {
    for (i=0; i<this->NumDataSets; i++)
      {
      int cellSize = this->DataSets[i]->GetMaxCellSize();
      maxCellSize = (cellSize > maxCellSize) ? cellSize : maxCellSize;
      }
    }

  float *weights = new float [maxCellSize];

  float *cptr = center;

  if (set)
    {
    for (j=0; j<totalCells; j++)
      {
      this->ComputeCellCenter(set->GetCell(j), cptr, weights);
      cptr += 3;
      }
    }
  else
    {
    for (i=0; i<this->NumDataSets; i++)
      {
      vtkDataSet *iset = this->DataSets[i];

      int nCells = iset->GetNumberOfCells();

      for (j=0; j<nCells; j++)
        {
        this->ComputeCellCenter(iset->GetCell(j), cptr, weights);

        cptr += 3;
        }
      }
    }

  delete [] weights;

  return center;

}
void vtkKdTree::ComputeCellCenter(vtkDataSet *set, int cellId, float *center)
{
  int setNum;

  if (set)
    {
    setNum = this->GetDataSet(set);

    if ( setNum < 0)
      {
      vtkErrorMacro(<<"vtkKdTree::ComputeCellCenter invalid data set");
      return;
      } 
    }
  else
    {
    setNum = 0;
    set = this->DataSets[0];
    }
      
  if ( (cellId < 0) || (cellId >= set->GetNumberOfCells()))
    {
    vtkErrorMacro(<<"vtkKdTree::ComputeCellCenter invalid cell ID");
    return;
    }

  float *weights = new float [set->GetMaxCellSize()];

  this->ComputeCellCenter(set->GetCell(cellId), center, weights);

  delete [] weights;

  return;
}
void vtkKdTree::ComputeCellCenter(vtkCell *cell, float *center, float *weights)
{   
  float pcoords[3];
  
  int subId = cell->GetParametricCenter(pcoords);
    
  cell->EvaluateLocation(subId, pcoords, center, weights);
      
  return;
} 

// Build the kdtree structure based on location of cell centroids.

void vtkKdTree::BuildLocator()
{
  int nCells=0;
  unsigned int maxTime=0;
  int i;

  for (i=0; i<this->NumDataSets; i++)
    {
    maxTime = (this->DataSets[i]->GetMTime() > maxTime) ?
               this->DataSets[i]->GetMTime() : maxTime;
    }

  if ((this->Top != NULL) && (this->BuildTime > this->MTime) &&
       (this->BuildTime > maxTime))
    {
    return;
    }

  nCells = this->GetNumberOfCells();

  if (nCells == 0)
    {
     vtkErrorMacro( << "vtkKdTree::BuildLocator - No cells to subdivide");
     return;
    }

  vtkDebugMacro( << "Creating Kdtree" );

  if (this->Timing)
    {
    if (this->TimerLog == NULL) this->TimerLog = vtkTimerLog::New();
    }

  TIMER("Set up to build k-d tree");

  this->FreeSearchStructure();

  // volume bounds - push out a little if flat

  float setBounds[6], volBounds[6];

  for (i=0; i<this->NumDataSets; i++)
    {
    if (i==0)
      {
      this->DataSets[i]->GetBounds(volBounds);
      }
    else
      {
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

  for (i=0; i<3; i++)
    {
     diff[i] = volBounds[2*i+1] - volBounds[2*i];
     aLittle = (diff[i] > aLittle) ? diff[i] : aLittle;
    }
  if ((aLittle /= 100.0) <= 0.0)
    {
     vtkErrorMacro( << "vtkKdTree::BuildLocator - degenerate volume");
     return;
    }
  for (i=0; i<3; i++)
    {
    if (diff[i] <= 0)
      {
      volBounds[2*i]   -= aLittle;
      volBounds[2*i+1] += aLittle;
      }
    }
  TIMERDONE("Set up to build k-d tree");
   
  // cell centers - basis of spacial decomposition

  TIMER("Create centroid list");

  float *ptarray = this->ComputeCellCenters();
  int totalPts = this->GetNumberOfCells();

  TIMERDONE("Create centroid list");

  if (!ptarray)
    {
    vtkErrorMacro( << "vtkKdTree::BuildLocator - insufficient memory");
    return;
    }

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

  // In the process of building the k-d tree regions,
  //   the cell centers became reordered, so no point
  //   in saving them, for example to build cell lists.

  delete [] ptarray;

  this->SetActualLevel();
  this->BuildRegionList();

  this->BuildTime.Modified();

  return;
}
int vtkKdTree::ComputeLevel(vtkKdNode *kd)
{
  if (!kd) return 0;
  
  int iam = 1;

  if (kd->Left != NULL)
    {
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

  if (this->ValidDirections == xdir)
    {
    dim = vtkKdTree::xdim;
    }
  else if (this->ValidDirections == ydir)
    {
    dim = vtkKdTree::ydim;
    }
  else if (this->ValidDirections == zdir)
    {
    dim = vtkKdTree::zdim;
    }
  else
    {
    // divide in the longest direction, for more compact regions

    double diff[3], dataBounds[6], maxdiff;
    kd->GetDataBounds(dataBounds);

    for (i=0; i<3; i++){ diff[i] = dataBounds[i*2+1] - dataBounds[i*2];}

    maxdiff = -1.0;

    if ((this->ValidDirections & xdir) && (diff[vtkKdTree::xdim] > maxdiff))
      {
      dim = vtkKdTree::xdim;
      maxdiff = diff[vtkKdTree::xdim];
      }

    if ((this->ValidDirections & ydir) && (diff[vtkKdTree::ydim] > maxdiff))
      {
      dim = vtkKdTree::ydim;
      maxdiff = diff[vtkKdTree::ydim];
      }

    if ((this->ValidDirections & zdir) && (diff[vtkKdTree::zdim] > maxdiff))
      {
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

#define Exchange(array, x, y) \
  {                           \
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

  for (i=3; i<K*3; i+=3)
    {
    if (Xcomponent[i] > max) max = Xcomponent[i];
    }
  return max;
}
void vtkKdTree::_Select(int dim, float *X, int L, int R, int K)
{
  int N, I, J, S, SD, LL, RR;
  float Z, T;

  while (R > L)
    {
    if ( R - L > 600)
      {
      // "Recurse on a sample of size S to get an estimate for the
      // (K-L+1)-th smallest element into X[K], biased slightly so
      // that the (K-L+1)-th element is expected to lie in the
      // smaller set after partitioning"

      N = R - L + 1;
      I = K - L + 1;
      Z = log(static_cast<float>(N));
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

    while (I < J)
      {
      Exchange(X, I, J);

      while (Xcomponent[(++I)*3] < T);

      while (Xcomponent[(--J)*3] > T);
      }

    if (Xcomponent[L*3] == T)
      {
      Exchange(X, L, J);
      }
    else
      {
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
  if (kd->Left == NULL)
    {
    this->RegionList[kd->Id] = kd; 
    }
  else
    {
    this->SelfRegister(kd->Left);
    this->SelfRegister(kd->Right);
    }

  return;
}
int vtkKdTree::SelfOrder(int startId, vtkKdNode *kd)
{
  int nextId;

  if (kd->Left == NULL)
    {
    kd->Id = startId;

    kd->MaxId = kd->MinId = startId;

    nextId = startId + 1;
    }
  else
    {
    kd->Id = -1;
    nextId = vtkKdTree::SelfOrder(startId, kd->Left);
    nextId = vtkKdTree::SelfOrder(nextId, kd->Right);

    kd->MinId = startId;
    kd->MaxId = nextId - 1;
    }

  return nextId;
}

// It may be necessary for a user of vtkKdTree to work on convex
// spatial regions.  Here we take a list of region IDs, and return
// the minimum number (N) of convex regions that it can be decomposed
// into, and N lists of the region IDs that make each convex region.
// If the region allocation scheme was "contigous", you are guaranteed
// that the set of regions assigned to each process composes a
// convex spatial region.

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
  if (this->Top) 
    {
    vtkKdTree::DeleteNodes(this->Top);
    delete this->Top;
    this->Top = NULL;
    }
  if (this->RegionList)
    {
    delete [] this->RegionList;
    this->RegionList = NULL;
    } 

  this->NumRegions = 0;
  this->SetActualLevel();

  this->DeleteCellLists();

  if (this->CellRegionList)
    {

    delete [] this->CellRegionList;
    this->CellRegionList = NULL;
    }
}

// build PolyData representation of all spacial regions------------

void vtkKdTree::GenerateRepresentation(int level, vtkPolyData *pd)
{
  if (this->GenerateRepresentationUsingDataBounds)
    {
    this->GenerateRepresentationDataBounds(level, pd);
    }
  else
    {
    this->GenerateRepresentationWholeSpace(level, pd);
    }
}

void vtkKdTree::GenerateRepresentationWholeSpace(int level, vtkPolyData *pd)
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

  for (i=0 ; i < level; i++)
    {
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

  if (kd->Left && (level > 0))
    {
      _generateRepresentationWholeSpace(kd, pts, polys, level-1);
    }

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}
void vtkKdTree::_generateRepresentationWholeSpace(vtkKdNode *kd, 
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

  switch (kd->Dim)
    {

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

  _generateRepresentationWholeSpace(kd->Left, pts, polys, level-1);
  _generateRepresentationWholeSpace(kd->Right, pts, polys, level-1);
}

void vtkKdTree::GenerateRepresentationDataBounds(int level, vtkPolyData *pd)
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

  for (i=0; i < level; i++)
    {
    int levelBoxes= 1 << i;
    npoints += (8 * levelBoxes);
    npolys += (6 * levelBoxes);
    }

  pts = vtkPoints::New();
  pts->Allocate(npoints); 
  polys = vtkCellArray::New();
  polys->Allocate(npolys);

  _generateRepresentationDataBounds(this->Top, pts, polys, level);

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}
void vtkKdTree::_generateRepresentationDataBounds(vtkKdNode *kd, vtkPoints *pts,
                     vtkCellArray *polys, int level)
{
  if (level > 0)
    {
      if (kd->Left)
        {
           _generateRepresentationDataBounds(kd->Left, pts, polys, level-1);
           _generateRepresentationDataBounds(kd->Right, pts, polys, level-1);
        }

      return;
    }
  vtkKdTree::AddPolys(kd, pts, polys);
}

// PolyData rep. of all spacial regions, shrunk to data bounds-------

void vtkKdTree::AddPolys(vtkKdNode *kd, vtkPoints *pts, vtkCellArray *polys)
{
  vtkIdType ids[8];
  vtkIdType idList[4];
  float     x[3];

  double *min;
  double *max;

  if (this->GenerateRepresentationUsingDataBounds)
    {
    min = kd->MinVal;
    max = kd->MaxVal;
    }
  else
    {
    min = kd->Min;
    max = kd->Max;
    }

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

  for (i=0; i<len; i++)
    {
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

void vtkKdTree::SetCellBounds(vtkCell *cell, float *bounds)
{
  vtkPoints *pts = cell->GetPoints();
  pts->Modified();         // VTK bug - so bounds will be re-calculated
  pts->GetBounds(bounds);
}

#define SORTLIST(l, lsize) vtkstd::sort(l, l + lsize)

#define REMOVEDUPLICATES(l, lsize, newsize) \
{                                  \
int ii,jj;                           \
for (ii=0, jj=0; ii<lsize; ii++)    \
  {                               \
  if ((ii > 0) && (l[ii] == l[jj-1])) continue; \
  if (jj != ii) l[jj] = l[ii];       \
  jj++;                           \
}                                \
newsize = jj;                     \
}
int vtkKdTree::FindInSortedList(int *list, int size, int val)
{
  int loc = -1;

  if (size < 8)
    {
    for (int i=0; i<size; i++)
      {
      if (list[i] == val)
        { 
        loc = i;
        break;
        }
      }

      return loc;
    }

  int L, R, M;
  L=0;
  R=size-1;

  while (R > L)  
    {
    if (R == L+1)
      {
      if (list[R] == val)
        {
        loc = R;
        }
      else if (list[L] == val)  
        {
        loc = L;
        }
      break;
      }

    M = (R + L) / 2;

    if (list[M] > val)  
      {
      R = M;
      continue;
      }
    else if (list[M] < val)
      {
      L = M;
      continue;
      }
    else
      {
      loc = M;
      break;
      }
    }
  return loc;
}

int vtkKdTree::FoundId(vtkIntArray *ar, int val)
{
  int size = ar->GetNumberOfTuples();
  int *ptr = ar->GetPointer(0);

  int where = vtkKdTree::FindInSortedList(ptr, size, val);

  return (where > -1);
}

int vtkKdTree::findRegion(vtkKdNode *node, float x, float y, float z)
{
  int regionId;

  if (!node->ContainsPoint(x, y, z, 0))
    {
    return -1;
    }

  if (node->Left == NULL)
    {
    regionId = node->Id;
    }
  else
    {
    regionId = vtkKdTree::findRegion(node->Left, x, y, z);

    if (regionId < 0) regionId = vtkKdTree::findRegion(node->Right, x, y, z);
    }

  return regionId;
}
void vtkKdTree::CreateCellLists()
{
  this->CreateCellLists(this->DataSets[0], (int *)NULL, 0);
  return;
}
void vtkKdTree::CreateCellLists(int *regionList, int listSize)
{
  this->CreateCellLists(this->DataSets[0], regionList, listSize);
  return;
}
void vtkKdTree::CreateCellLists(int dataSet, int *regionList, int listSize)
{
  if ((dataSet < 0) || (dataSet >= NumDataSets))
    {
    vtkErrorMacro(<<"vtkKdTree::CreateCellLists invalid data set");
    return;
    }

  this->CreateCellLists(this->DataSets[dataSet], regionList, listSize);
  return;
}
void vtkKdTree::CreateCellLists(vtkDataSet *set, int *regionList, int listSize)
{
  int i, AllRegions;

  if ( this->GetDataSet(set) < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::CreateCellLists invalid data set");
    return;
    }

  vtkKdTree::_cellList *list = &this->CellList;

  if (list->nRegions > 0)
    {
    this->DeleteCellLists();
    }

  if ((regionList == NULL) || (listSize == 0)) 
    {
    list->nRegions = this->NumRegions;    // all regions
    }
  else 
    {
    list->regionIds = new int [listSize];
  
    if (!list->regionIds)
      {
      vtkErrorMacro(<<"vtkKdTree::CreateCellLists memory allocation");
      return;
      }

    memcpy(list->regionIds, regionList, sizeof(int) * listSize);
    SORTLIST(list->regionIds, listSize);
    REMOVEDUPLICATES(list->regionIds, listSize, list->nRegions);
  
    if (list->nRegions == this->NumRegions)
      {
      delete [] list->regionIds;
      list->regionIds = NULL;
      }
    }

  if (list->nRegions == this->NumRegions)
    {
    AllRegions = 1;
    }
  else
    {
    AllRegions = 0; 
    } 
    
  int *idlist = NULL;
  int idListLen = 0;
  
  if (this->IncludeRegionBoundaryCells)
    {
    list->boundaryCells = new vtkIdList * [list->nRegions];

    if (!list->boundaryCells)
      {
      vtkErrorMacro(<<"vtkKdTree::CreateCellLists memory allocation");
      return;
      }
    
    for (i=0; i<list->nRegions; i++)
      {
      list->boundaryCells[i] = vtkIdList::New();
      }
    idListLen = this->NumRegions;
    
    idlist = new int [idListLen];
    }
  
  int *listptr = NULL;
    
  if (!AllRegions)
    {
    listptr = new int [this->NumRegions];

    if (!listptr)
      {
      vtkErrorMacro(<<"vtkKdTree::CreateCellLists memory allocation");
      return;
      }

    for (i=0; i<this->NumRegions; i++)
      {
      listptr[i] = -1;
      }
    }

  list->cells = new vtkIdList * [list->nRegions];

  if (!list->cells)
    {
    vtkErrorMacro(<<"vtkKdTree::CreateCellLists memory allocation");
    return;
    }

  for (i = 0; i < list->nRegions; i++)
    {
    list->cells[i] = vtkIdList::New();

    if (listptr) listptr[list->regionIds[i]] = i;
    }

  // acquire a list in cell Id order of the region Id each
  // cell centroid falls in

  int *regList = this->CellRegionList;

  if (regList == NULL)
    {
    regList = this->AllGetRegionContainingCell();
    }

  int setNum = this->GetDataSet(set);

  if (setNum > 0)
    {
    int ncells = this->GetDataSetsNumberOfCells(0,setNum-1);
    regList += ncells;
    }

  int intersectionOption = this->ComputeIntersectionsUsingDataBounds;
  this->ComputeIntersectionsUsingDataBounds = 0;

  int nCells = set->GetNumberOfCells();

  for (int cellId=0; cellId<nCells; cellId++)
    {
    if (this->IncludeRegionBoundaryCells)
      {
      // Find all regions the cell intersects, including
      // the region the cell centroid lies in.
      // This can be an expensive calculation, intersections
      // of a convex region with axis aligned boxes.

      int nRegions = this->IntersectsCell(idlist, idListLen, cellId,
                                          regList[cellId]);
      
      if (nRegions == 1)
        {
        int idx = (listptr) ? listptr[idlist[0]] : idlist[0];
      
        if (idx >= 0) list->cells[idx]->InsertNextId(cellId);
        }
      else
        {
        for (int r=0; r < nRegions; r++)
          {
          int regionId = idlist[r];
    
          int idx = (listptr) ? listptr[regionId] : regionId;

          if (idx < 0) continue;

          if (regionId == regList[cellId])
            {
            list->cells[idx]->InsertNextId(cellId);
            }
          else
            {
            list->boundaryCells[idx]->InsertNextId(cellId);
            }         
          }
        }
      }
    else 
      {
      // just find the region the cell centroid lies in - easy

      int regionId = regList[cellId];
    
      int idx = (listptr) ? listptr[regionId] : regionId;
  
      if (idx >= 0) list->cells[idx]->InsertNextId(cellId);
      } 
    }

  this->ComputeIntersectionsUsingDataBounds = intersectionOption;

  if (listptr)
    {
    delete [] listptr;
    } 
  if (idlist)
    {
    delete [] idlist;
    }   
}     
vtkIdList * vtkKdTree::GetList(int regionId, vtkIdList **which)
{
  int i;
  struct _cellList *list = &this->CellList;
  vtkIdList *cellIds = NULL;

  if (list->nRegions == this->NumRegions)
    {
    cellIds = which[regionId];
    }
  else
    {
    for (i=0; i< list->nRegions; i++)
      {
      if (list->regionIds[i] == regionId)
        {
        cellIds = which[i];
        break;
        }
      }
    }

  if (cellIds == NULL)
    {
    vtkErrorMacro(<<"vtkKdTree::GetCellList list not yet generated");
    }

  return cellIds;
}
vtkIdList * vtkKdTree::GetCellList(int regionID)
{
  return this->GetList(regionID, this->CellList.cells);
}
vtkIdList * vtkKdTree::GetBoundaryCellList(int regionID)
{
  return this->GetList(regionID, this->CellList.boundaryCells);
}

int vtkKdTree::GetRegionContainingCell(vtkIdType cellID)
{
  return this->GetRegionContainingCell(this->DataSets[0], cellID);
}
int vtkKdTree::GetRegionContainingCell(int set, vtkIdType cellID)
{
  if ( (set < 0) || (set >= this->NumDataSets))
    {
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell no such data set");
    return -1;
    }
  return this->GetRegionContainingCell(this->DataSets[set], cellID);
}
int vtkKdTree::GetRegionContainingCell(vtkDataSet *set, vtkIdType cellID)
{
  int regionID = -1;

  if ( this->GetDataSet(set) < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell no such data set");
    return -1;
    }
  if ( (cellID < 0) || (cellID >= set->GetNumberOfCells()))
    {
    vtkErrorMacro(<<"vtkKdTree::GetRegionContainingCell bad cell ID");
    return -1;
    }
  
  if (this->CellRegionList)
    {
    if (set == this->DataSets[0])        // 99.99999% of the time
      {
      return this->CellRegionList[cellID];
      }
    
    int setNum = this->GetDataSet(set);
    
    int offset = this->GetDataSetsNumberOfCells(0, setNum-1);
    
    return this->CellRegionList[offset + cellID];
    }
  
  float center[3];
  
  this->ComputeCellCenter(set, cellID, center);
  
  regionID = this->GetRegionContainingPoint(center[0], center[1], center[2]);
  
  return regionID;
}
int *vtkKdTree::AllGetRegionContainingCell()
{ 
  if (this->CellRegionList)
    {
    return this->CellRegionList;
    }
  this->CellRegionList = new int [this->GetNumberOfCells()];
  
  if (!this->CellRegionList)
    {
    vtkErrorMacro(<<"vtkKdTree::AllGetRegionContainingCell memory allocation");
    return NULL;
    }
  
  int *listPtr = this->CellRegionList;
  
  for (int set=0; set < this->NumDataSets; set++)
    {
    int setCells = this->DataSets[set]->GetNumberOfCells();
    
    float *centers = this->ComputeCellCenters(set);
    
    float *pt = centers;

    for (int cellId = 0; cellId < setCells; cellId++)
      {
      listPtr[cellId] =
        this->GetRegionContainingPoint(pt[0], pt[1], pt[2]);

      pt += 3;
      }

    listPtr += setCells;

    delete [] centers;
    }

  return this->CellRegionList;
}
int vtkKdTree::GetRegionContainingPoint(float x, float y, float z)
{
  return vtkKdTree::findRegion(this->Top, x, y, z);
}
int vtkKdTree::MinimalNumberOfConvexSubRegions(vtkIntArray *regionIdList,
                                               float **convexSubRegions)
{
  int i;
  int nids = regionIdList->GetNumberOfTuples();
  int *ids = regionIdList->GetPointer(0);

  if (nids < 1)
    {
    return 0;
    }

  if (nids == 1)
    {
    if ( (ids[0] < 0) || (ids[0] >= this->NumRegions))
      {
      return 0;
      }

    float *bounds = new float [6];

    this->RegionList[ids[0]]->GetBounds(bounds);

    *convexSubRegions = bounds;

    return 1;
    }

  // create a sorted list of unique region Ids

  int *idList = new int [nids];
  int nUniqueIds;

  memcpy(idList, ids, nids * sizeof(int));

  SORTLIST(idList, nids);

  if ( (idList[0] < 0) || (idList[nids-1] >= this->NumRegions)) 
    {
    delete [] idList;
    return 0;
    }

  REMOVEDUPLICATES(idList, nids, nUniqueIds);

  vtkKdNode **regions = new vtkKdNode * [nUniqueIds];
  
  int nregions = vtkKdTree::__ConvexSubRegions(idList, nUniqueIds, 
                                             this->Top, regions);
  
  float *bounds = new float [nregions * 6];
  
  for (i=0; i<nregions; i++)
    {
    regions[i]->GetBounds(bounds + (i*6));
    }
  
  *convexSubRegions = bounds;
  
  delete [] idList;
  delete [] regions;
  
  return nregions;
}
int vtkKdTree::__ConvexSubRegions(int *ids, int len, vtkKdNode *tree, vtkKdNode **nodes)
{ 
  int nregions = tree->MaxId - tree->MinId + 1;
  
  if (nregions == len)
    {
    *nodes = tree;
    return 1;
    }
  
  if (tree->Left == NULL)
    {
    return 0;
    }
  
  int min = ids[0];
  int max = ids[len-1];
  
  int leftMax = tree->Left->MaxId;
  int rightMin = tree->Right->MinId;
  
  if (max <= leftMax)
    {
    return vtkKdTree::__ConvexSubRegions(ids, len, tree->Left, nodes);
    }
  else if (min >= rightMin)
    {
    return vtkKdTree::__ConvexSubRegions(ids, len, tree->Right, nodes);
    }
  else
    {
    int leftIds = 1;

    for (int i=1; i<len-1; i++)
      {
      if (ids[i] <= leftMax) leftIds++;
      else                   break;
      }

    int numNodesLeft =
      vtkKdTree::__ConvexSubRegions(ids, leftIds, tree->Left, nodes);

    int numNodesRight =
      vtkKdTree::__ConvexSubRegions(ids + leftIds, len - leftIds,
                               tree->Right, nodes + numNodesLeft);

    return (numNodesLeft + numNodesRight);
    }
}
int vtkKdTree::DepthOrderRegions(vtkIntArray *regionIds,
                       vtkCamera *camera, vtkIntArray *orderedList)
{
  int nRegions = regionIds->GetNumberOfTuples();
  int nUniqueRegions = 0;
  int *sorted = NULL;

  if (nRegions > 0)
    {
    int *regionPtr = regionIds->GetPointer(0);
    sorted = new int [nRegions];

    memcpy(sorted, regionPtr, sizeof(int) * nRegions);

    SORTLIST(sorted, nRegions);
    REMOVEDUPLICATES(sorted, nRegions, nUniqueRegions);
    }

  vtkIntArray *IdsOfInterest = NULL;

  if (sorted)
    {
    IdsOfInterest = vtkIntArray::New();
    IdsOfInterest->SetArray(sorted, nUniqueRegions, 0); 
    } 

  int size = this->_DepthOrderRegions(IdsOfInterest, camera, orderedList);

  IdsOfInterest->Delete();
 
  return size;
}
int vtkKdTree::DepthOrderAllRegions(vtkCamera *camera, vtkIntArray *orderedList)
{
  return this->_DepthOrderRegions(NULL, camera, orderedList);
}     
int vtkKdTree::_DepthOrderRegions(vtkIntArray *IdsOfInterest,
                                 vtkCamera *camera, vtkIntArray *orderedList)
{
  int nextId = 0;

  int numValues = (IdsOfInterest ? IdsOfInterest->GetNumberOfTuples() :
                                   this->NumRegions);

  orderedList->Initialize();
  orderedList->SetNumberOfValues(numValues);

  float dir[3];

  camera->GetDirectionOfProjection(dir);

  int size = 
    vtkKdTree::__DepthOrderRegions(this->Top, orderedList, IdsOfInterest, dir, nextId);

  if (size < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::DepthOrderRegions k-d tree structure is corrupt");
    orderedList->Initialize();
    return 0;
    }
    
  return size;
}     
int vtkKdTree::__DepthOrderRegions(vtkKdNode *node, 
                                   vtkIntArray *list, vtkIntArray *IdsOfInterest,
                                   float *dir, int nextId)
{
  if (node->Left == NULL)
    {
    if (!IdsOfInterest || vtkKdTree::FoundId(IdsOfInterest, node->Id))
      {
      list->SetValue(nextId, node->Id);
      nextId = nextId+1;
      }

      return nextId;
    }   
                               
  int cutPlane = node->Dim;
    
  if ((cutPlane < 0) || (cutPlane > 2))
    {
    return -1;
    } 
                       
  float closest = dir[cutPlane] * -1;
  
  vtkKdNode *closeNode = (closest < 0) ? node->Left : node->Right;
  vtkKdNode *farNode  = (closest >= 0) ? node->Left: node->Right;
    
  int nextNextId = vtkKdTree::__DepthOrderRegions(closeNode, list, 
                                         IdsOfInterest, dir, nextId);

  if (nextNextId == -1) return -1;
    
  nextNextId = vtkKdTree::__DepthOrderRegions(farNode, list, 
                                     IdsOfInterest, dir, nextNextId);

  return nextNextId;
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
  if ( (regionId < 0) || (regionId >= NumRegions))
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsBox invalid spatial region ID");
    return 0;
    }

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsBox(x0, x1, y0, y1, z0, z1,
                     this->ComputeIntersectionsUsingDataBounds);
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

  if (len > 0)
    {
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

  result = node->IntersectsBox(x0, x1, y0, y1, z0, z1,
                             this->ComputeIntersectionsUsingDataBounds);

  if (!result) return 0;

  if (node->Left == NULL)
    {
    ids[0] = node->Id;
    return 1;
    }

  nnodes1 = _IntersectsBox(node->Left, ids, len, x0, x1, y0, y1, z0, z1);

  idlist = ids + nnodes1;
  listlen = len - nnodes1;

  if (listlen > 0)
    {
    nnodes2 = _IntersectsBox(node->Right, idlist, listlen, x0, x1, y0, y1, z0, z1);
    }
  else
    {
    nnodes2 = 0;
    }

  return (nnodes1 + nnodes2);
}
// Intersection with arbitrary vtkCell -----------------------------

int vtkKdTree::IntersectsCell(int regionId, int cellId, int cellRegion)
{                            
  return this->IntersectsCell(regionId, this->DataSets[0], cellId, cellRegion);
}
  
int vtkKdTree::IntersectsCell(int regionId, vtkDataSet *set, int cellId, int cellRegion)
{             
  if (this->GetDataSet(set) < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid data set");
    return 0;                
    }                          

  vtkCell *cell = set->GetCell(cellId);

  if (!cell)
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid cell ID");
    return 0;                
    }
  
  return this->IntersectsCell(regionId, cell, cellRegion);
}
int vtkKdTree::IntersectsCell(int regionId, vtkCell *cell, int cellRegion)
{                            
  if ( (regionId < 0) || (regionId >= this->NumRegions)) 
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid region ID");
    return 0;
    }

  vtkKdNode *node = this->RegionList[regionId];

  return node->IntersectsCell(cell, this->ComputeIntersectionsUsingDataBounds,
                              cellRegion);
}

int vtkKdTree::IntersectsCell(int *ids, int len, int cellId, int cellRegion)
{
  return this->IntersectsCell(ids, len, this->DataSets[0], cellId, cellRegion);
}
int vtkKdTree::IntersectsCell(int *ids, int len, vtkDataSet *set, int cellId, int cellRegion)
{
  if (this->GetDataSet(set) < 0)
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid data set");
    return 0;
    }

  vtkCell *cell = set->GetCell(cellId);

  if (!cell)
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsCell invalid cell ID");
    return 0;
    }

  return this->IntersectsCell(ids, len, cell, cellRegion);
}
int vtkKdTree::IntersectsCell(int *ids, int len, vtkCell *cell, int cellRegion)
{
  vtkKdTree::SetCellBounds(cell, this->cellBoundsCache);

  this->Top->cellBoundsCache = this->cellBoundsCache;

  return this->_IntersectsCell(this->Top, ids, len, cell, cellRegion);
}
int vtkKdTree::_IntersectsCell(vtkKdNode *node, int *ids, int len,
                                 vtkCell *cell, int cellRegion)
{
  int result, nnodes1, nnodes2, listlen, intersects;
  int *idlist;

  intersects = node->IntersectsCell(cell,
                                this->ComputeIntersectionsUsingDataBounds,
                                cellRegion);

  if (intersects)
    {
    if (node->Left)
      {
      node->Left->cellBoundsCache = node->cellBoundsCache;

      nnodes1 = this->_IntersectsCell(node->Left, ids, len, cell,
                                cellRegion);

      idlist = ids + nnodes1;
      listlen = len - nnodes1;
  
      if (listlen > 0) 
        {       
        node->Right->cellBoundsCache = node->cellBoundsCache;

        nnodes2 = this->_IntersectsCell(node->Right, idlist, listlen, cell,
                                  cellRegion);
        }
      else
        {
        nnodes2 = 0;
        }
  
      result = nnodes1 + nnodes2;
      }
    else
      {
      ids[0] = node->Id;     // leaf node (spatial region)

      result = 1;
      }
    } 
  else
    {
    result = 0;
    }

  node->cellBoundsCache = NULL;

  return result;
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
  for (i=0; i<nvertices; i++)
    {
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
  if ( (regionId < 0) || (regionId >= this->NumRegions)) 
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsRegion invalid region ID");
    return 0;
    }

  vtkPlanesIntersection *pi = vtkPlanesIntersection::New();

  pi->SetNormals(planes->GetNormals());
  pi->SetPoints(planes->GetPoints());

  if (vertices && (nvertices>0)) pi->SetRegionVertices(vertices, nvertices);

  vtkKdNode *node = this->RegionList[regionId];

  int intersects = node->IntersectsRegion(pi, this->ComputeIntersectionsUsingDataBounds);

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

  if (nvertices > 0)
    {
    dv = new double[nvertices];
    for (i=0; i<nvertices; i++)
      {
      dv[i] = vertices[i];
      }
    }
  else
    {
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

  if (len > 0)
    {
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

  result = node->IntersectsRegion(pi, this->ComputeIntersectionsUsingDataBounds);

  if (!result) return 0;

  if (node->Left == NULL)
    {
    ids[0] = node->Id;
    return 1;
    }

  nnodes1 = _IntersectsRegion(node->Left, ids, len, pi);

  idlist = ids + nnodes1;
  listlen = len - nnodes1;

  if (listlen > 0)
    {
    nnodes2 = _IntersectsRegion(node->Right, idlist, listlen, pi);
    }
  else
    {
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
  if ( (x0 < -1) || (x1 > 1) || (y0 < -1) || (y1 > 1))
    {
    vtkErrorMacro(<<
      "vtkKdTree::IntersectsFrustum, use view coordinates ([-1,1], [-1,1])");
    return 0;
    }

  if ( (regionId < 0) || (regionId >= this->NumRegions))
    {
    vtkErrorMacro(<<"vtkKdTree::IntersectsFrustum invalid region ID");
    return 0;
    }

  vtkPlanesIntersection *planes = 
    vtkPlanesIntersection::ConvertFrustumToWorld(ren, x0, x1, y0, y1);

  vtkKdNode *node = this->RegionList[regionId];

  int intersects = node->IntersectsRegion(planes, 
                   this->ComputeIntersectionsUsingDataBounds);

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
  if ( (x0 < -1) || (x1 > 1) || (y0 < -1) || (y1 > 1))
    {
    vtkErrorMacro(<<
      "vtkKdTree::IntersectsFrustum, use view coordinates ([-1,1], [-1,1])");
    return 0;
    }

  vtkPlanesIntersection *planes = 
        vtkPlanesIntersection::ConvertFrustumToWorld(ren, x0, x1, y0, y1);

  int howmany = _IntersectsRegion(this->Top, ids, len, planes);

  planes->Delete();

  return howmany;
}

void vtkKdTree::OmitXPartitioning() 
{
  this->Modified();
  this->ValidDirections = (1 << vtkKdTree::ydim) | (1 << vtkKdTree::zdim);
}

void vtkKdTree::OmitYPartitioning()    
{
  this->Modified();
  this->ValidDirections = (1 << vtkKdTree::zdim) | (1 << vtkKdTree::xdim);
}

void vtkKdTree::OmitZPartitioning()    
{
  this->Modified();
  this->ValidDirections = (1 << vtkKdTree::xdim) | (1 << vtkKdTree::ydim);
}

void vtkKdTree::OmitXYPartitioning()
{
  this->Modified();
  this->ValidDirections = (1 << vtkKdTree::zdim);
}

void vtkKdTree::OmitYZPartitioning()
{
  this->Modified();
  this->ValidDirections = (1 << vtkKdTree::xdim);
}

void vtkKdTree::OmitZXPartitioning()
{
  this->Modified();
  this->ValidDirections = (1 << vtkKdTree::ydim);
}

void vtkKdTree::OmitNoPartitioning()
{
  this->Modified();
  this->ValidDirections =
    ((1 << vtkKdTree::xdim)|(1 << vtkKdTree::ydim)|(1 << vtkKdTree::zdim));
}

//---------------------------------------------------------------------------

void vtkKdTree::PrintTiming(ostream& os, vtkIndent )
{
  vtkTimerLog::DumpLogWithIndents(&os, (float)0.0);
}

void vtkKdTree::PrintSelf(ostream& os, vtkIndent indent)
{ 
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ValidDirections: " << this->ValidDirections << endl;
  os << indent << "MinCells: " << this->MinCells << endl;
  os << indent << "NumRegions: " << this->NumRegions << endl;

  os << indent << "DataSets: " << this->DataSets << endl;
  os << indent << "NumDataSets: " << this->NumDataSets << endl;

  os << indent << "Top: " << this->Top << endl;
  os << indent << "RegionList: " << this->RegionList << endl;

  os << indent << "Timing: " << this->Timing << endl;
  os << indent << "TimerLog: " << this->TimerLog << endl;

  os << indent << "NumDataSetsAllocated: " << this->NumDataSetsAllocated << endl;
  os << indent << "IncludeRegionBoundaryCells: ";
        os << this->IncludeRegionBoundaryCells << endl;
  os << indent << "GenerateRepresentationUsingDataBounds: ";
        os<< this->GenerateRepresentationUsingDataBounds << endl;

  if (this->CellList.nRegions > 0)
    {
    os << indent << "CellList.dataSet " << this->CellList.dataSet << endl;
    os << indent << "CellList.regionIds " << this->CellList.regionIds << endl;
    os << indent << "CellList.nRegions " << this->CellList.nRegions << endl;
    os << indent << "CellList.cells " << this->CellList.cells << endl;
    os << indent << "CellList.boundaryCells " << this->CellList.boundaryCells << endl;
    }
  os << indent << "CellRegionList: " << this->CellRegionList << endl;
  os << indent << "ComputeIntersectionsUsingDataBounds: ";
    os << this->ComputeIntersectionsUsingDataBounds << endl;
}


