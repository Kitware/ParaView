/*=========================================================================

  Program:   ParaView
  Module:    vtkKdTree.h

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

// .NAME vtkKdTree - a Kd-tree spatial decomposition of a data set
//
// .SECTION Description
//     Creates a k-d tree decomposition of one or more data sets, responds
//     to region intersection queries, manages lists of cell Ids for
//     each region for each data set.
//     If there is no specification of minimum number of cells per 
//     region or maximum number of levels in the tree, the tree is
//     built to maximum of 20 levels or until less than 100 cells
//     would be in each region.
//
// .SECTION See Also
//      vtkLocator vtkCellLocator vtkPKdTree

#ifndef __vtkKdTree_h
#define __vtkKdTree_h

#include "vtkLocator.h"

class vtkTimerLog;
class vtkIdList;
class vtkIntArray;
class vtkPoints;
class vtkCellArray;
class vtkRenderer;
class vtkPlanesIntersection;
class vtkPlanes;
class vtkCell;
class vtkCamera;

#define makeCompareFunc(name, loc) \
   static int compareFunc##name(const void *a, const void *b){ \
      const double *apt = static_cast<const double *>(a); \
      const double *bpt = static_cast<const double *>(b); \
      if (apt[loc] < bpt[loc]) return -1;  \
      else if (apt[loc] > bpt[loc]) return 1; \
      else return 0;                        \
   }
//BTX

class VTK_EXPORT vtkKdNode{
public:

  vtkKdNode();
  ~vtkKdNode();

  void SetDim(int n){this->Dim = n;}
  int  GetDim(){return this->Dim;}

  void SetNumberOfCells(int n){this->NumCells = n;}
  int  GetNumberOfCells(){return this->NumCells;}

  void SetBounds(double x1,double x2,double y1,double y2,double z1,double z2);
  void GetBounds(double *b) const;


  void SetDataBounds(double x1,double x2,double y1,double y2,double z1,double z2);
  void SetDataBounds(double *b);  
  void GetDataBounds(double *b) const;

  void PrintNode(int depth);
  void PrintVerboseNode(int depth);
  void AddChildNodes(vtkKdNode *left, vtkKdNode *right);

  int IntersectsBox(double x1,double x2,double y1,double y2,double z1,double z2,
                    int useDataBounds);
  int IntersectsRegion(vtkPlanesIntersection *pi, int useDataBounds);

  int IntersectsCell(vtkCell *cell, int useDataBounds, int cellRegion=-1);

  int ContainsBox(double x1,double x2,double y1,double y2,double z1,double z2,
                  int useDataBounds);

  int ContainsPoint(double x, double y, double z, int useDataBounds);

  static const char *LevelMarker[20];

  double Min[3], Max[3];       // spatial bounds of node
  double MinVal[3], MaxVal[3]; // spatial bounds of data
  int NumCells;
  
  vtkKdNode *Up;

  vtkKdNode *Left;
  vtkKdNode *Right;

  int Dim;

  int Id;        // region id

  int MinId;
  int MaxId;

  double *CellBoundsCache;  // to optimize IntersectsCell
};
//ETX

class VTK_EXPORT vtkKdTree : public vtkLocator
{

public:
  vtkTypeRevisionMacro(vtkKdTree, vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkKdTree *New();

  // Description:
  //  Turn on timing of the k-d tree build
  vtkBooleanMacro(Timing, int);
  vtkSetMacro(Timing, int);
  vtkGetMacro(Timing, int);
    
  // Description:
  //  Minimum number of cells per spatial region
  vtkSetMacro(MinCells, int);
  vtkGetMacro(MinCells, int);

  // Description:
  //    Omit partitions along the X axis, yielding shafts in the X direction
  void OmitXPartitioning();

  // Description:
  //    Omit partitions along the Y axis, yielding shafts in the Y direction
  void OmitYPartitioning();

  // Description:
  //    Omit partitions along the Z axis, yielding shafts in the Z direction
  void OmitZPartitioning();

  // Description:
  //    Omit partitions along the X and Y axes, yielding slabs along Z
  void OmitXYPartitioning();

  // Description:
  //    Omit partitions along the Y and Z axes, yielding slabs along X
  void OmitYZPartitioning();

  // Description:
  //    Omit partitions along the Z and X axes, yielding slabs along Y
  void OmitZXPartitioning();

  // Description:
  //    Partition along all three axes - this is the default
  void OmitNoPartitioning();

  // Description:
  //   Add a data set to the list of those included in spatial paritioning

  void SetDataSet(vtkDataSet *set);

  // Description:
  //   Remove a data set from the list of those included in spatial paritioning

  void RemoveDataSet(vtkDataSet *set);
  void RemoveDataSet(int which);

  // Description:
  //   Get the number of data sets included in spatial paritioning

  int GetNumberOfDataSets(){return this->NumDataSets;};

  // Description:
  //   Get one of the data sets included in spatial paritioning

  vtkDataSet *GetDataSet(int i){return this->DataSet?this->DataSets[i]:0;}
  vtkDataSet *GetDataSet(){ return this->GetDataSet(0); }

  // Description:
  //   Get handle for one of the data sets included in spatial paritioning.
  //   Handles can change after RemoveDataSet.

  int GetDataSet(vtkDataSet *set);

  // Description:
  //    The number of leaf nodes of the tree, the spatial regions
  int GetNumberOfRegions(void){return this->NumRegions;}

  // Description:
  //    Get the spatial bounds of k-d tree region

  void GetRegionBounds(int regionID, double bounds[6]);

  // Description:
  //    Get the bounds of the data within the k-d tree region

  void GetRegionDataBounds(int regionID, double bounds[6]);

  // Description:
  //    Get the spatial bounds of a list k-d tree regions

  void GetRegionsBounds(int *regions, int numRegions, double bounds[6]);
    
  // Description:
  //    Get the bounds of the data within a list of k-d tree regions
    
  void GetRegionsDataBounds(int *regions, int numRegions, double bounds[6]);
    
  // Description:
  //    Print out nodes of kd tree
  void PrintTree();
  void PrintVerboseTree();
    
  // Description:
  //    Print out leaf node data for given id
  void PrintRegion(int id){ this->RegionList[id]->PrintNode(0);}
    
  // Description:
  //   Create a list for each of the requested regions, listing
  //   the IDs of all cells whose centroid falls in the region.
  //   These lists are obtained with GetCellList().
  //   If no DataSet is specified, the cell list is created
  //   for DataSet 0.  If no list of requested regions is provided,
  //   the cell lists for all regions are created.  
  //
  //   When CreateCellLists is called again, the lists created
  //   on the previous call  are deleted.
    
  void CreateCellLists(int DataSet, int *regionReqList, 
                       int reqListSize);
  void CreateCellLists(vtkDataSet *set, int *regionReqList,
                       int reqListSize);
  void CreateCellLists(int *regionReqList, int listSize);
  void CreateCellLists(); 
    
  // Description:
  //   If IncludeRegionBoundaryCells is ON,
  //   CreateCellLists() will also create a list of cells which
  //   intersect a given region, but are not assigned
  //   to the region.  These lists are obtained with 
  //   GetBoundaryCellList().  Default is OFF.

  vtkSetMacro(IncludeRegionBoundaryCells, int);
  vtkGetMacro(IncludeRegionBoundaryCells, int);
  vtkBooleanMacro(IncludeRegionBoundaryCells, int);

  // Description:
  //    Free the memory used by the cell lists.

  void DeleteCellLists();

  // Description:
  //    Get the cell list for a region.

  vtkIdList *GetCellList(int regionID);

  // Description:
  //    The cell list obtained with GetCellList is the list
  //    of all cells such that their centroid is contained in
  //    the spatial region.  It may also be desirable to get
  //    a list of all cells intersecting a spatial region,
  //    but with centroid in some other region.  This is that
  //    list.  This list is computed in CreateCellLists() if
  //    and only if IncludeRegionBoundaryCells is ON.

  vtkIdList *GetBoundaryCellList(int regionID);

  // Description:
  //    Get the id of the region containing the cell centroid.  If
  //    no DataSet is specified, assume DataSet 0.  If you need the
  //    region ID for every cell, use AllGetRegionContainingCell
  //    instead.  It is more efficient.

  int GetRegionContainingCell(vtkDataSet *set, vtkIdType cellID);
  int GetRegionContainingCell(int set, vtkIdType cellID);
  int GetRegionContainingCell(vtkIdType cellID);

  // Description:
  //    Get a list (in order by data set by cell id) of the
  //    region IDs of the region containing the centroid for
  //    each cell.
  //    This is faster than calling GetRegionContainingCell
  //    for each cell in the DataSet.
  //    vtkKdTree uses this list, so don't delete it.

  int *AllGetRegionContainingCell();

  // Description:
  //    Get the id of the region containing the specified location.

  int GetRegionContainingPoint(double x, double y, double z);
    
  // Description:
  //    Given a vtkCamera, this function creates a list of the k-d tree
  //    region IDs in order from front to back with respect to the
  //    camera's direction of projection.  The number of regions in
  //    the ordered list is returned.  (This is not actually sorting
  //    the regions on their distance from the view plane, but there
  //    is no region on the list which blocks a region that appears
  //    earlier on the list.)

  int DepthOrderAllRegions(vtkCamera *camera, vtkIntArray *orderedList);
    
  // Description:
  //    Given a vtkCamera, and a list of k-d tree region IDs, this
  //    function creates an ordered list of those IDs
  //    in front to back order with respect to the
  //    camera's direction of projection.  The number of regions in
  //    the ordered list is returned.
    
  int DepthOrderRegions(vtkIntArray *regionIds, vtkCamera *camera, 
                        vtkIntArray *orderedList);

  // Description:
  //    Determine whether a region of the spatial decomposition 
  //    intersects an axis aligned box.

  int IntersectsBox(int regionId, double *x); 
  int IntersectsBox(int regionId, double xmin, double xmax, 
                    double ymin, double ymax, 
                    double zmin, double zmax); 

  // Description:
  //    Compute a list of the Ids of all regions that 
  //    intersect the specified axis aligned box.
  //    Returns: the number of ids in the list.

  int IntersectsBox(int *ids, int len,  double *x); 
  int IntersectsBox(int *ids, int len,  double x0, double x1, 
                    double y0, double y1, double z0, double z1); 

  // Description:
  //    Determine whether a region of the spatial decomposition 
  //    intersects the convex region "inside" a set of planes.
  //    Planes must be defined as vtkPlanes (i.e. outward pointing
  //    normals).  If you can provide the vertices of the convex
  //    region (as 3-tuples) it will save an expensive calculation.

  int IntersectsRegion(int regionId, vtkPlanes *planes);
  int IntersectsRegion(int regionId, vtkPlanes *planes, 
                       int nvertices, double *vertices);

  // Description:
  //    Compute a list of the Ids of all regions that 
  //    intersect the convex region "inside" a set of planes.
  //    Planes must be defined as vtkPlanes (i.e. outward pointing
  //    normals).  If you can provide the vertices of the convex
  //    region (as 3-tuples) it will save an expensive calculation.
  //    Returns: the number of ids in the list.

  int IntersectsRegion(int *ids, int len, vtkPlanes *planes);
  int IntersectsRegion(int *ids, int len, vtkPlanes *planes, 
                       int nvertices, double *vertices);

  // Description:
  //    Determine whether a region of the spatial decomposition
  //    intersects the given cell.  If a cell Id is given, and
  //    no data set is specified, data set 0 is assumed.  If you
  //    already know the region that the cell centroid lies in,
  //    (perhaps from a previous call to AllGetRegionContainingCell),
  //    provide that as the last argument to make the computation
  //    quicker.

  int IntersectsCell(int regionId, vtkCell *cell, int cellRegion=-1);
  int IntersectsCell(int regionId, int cellId, int cellRegion=-1);
  int IntersectsCell(int regionId, vtkDataSet *Set, int cellId, int cellRegion=-1);

  // Description:
  //    Compute a list of the Ids of all regions that
  //    intersect the given cell.  If a cell Id is given,
  //    and no data set is specified, data set 0 is assumed.  If you
  //    already know the region that the cell centroid lies in,
  //    provide that as the last argument to make the computation
  //    quicker.
  //    Returns the number of regions the cell intersects.

  int IntersectsCell(int *ids, int len, vtkCell *cell, int cellRegion=-1);
  int IntersectsCell(int *ids, int len, int cellId, int cellRegion=-1);
  int IntersectsCell(int *ids, int len, vtkDataSet *set, int cellId, int cellRegion=-1);

  // Description:
  //    Determine whether a region of the spatial decomposition 
  //    intersects a region which is the view frustum obtained from 
  //    an axis aligned rectangular viewport.  
  //    Viewport coordinates range from -1 to +1 in x and y directions.

  int IntersectsFrustum(int regionId, vtkRenderer *ren, 
                        double x0, double x1, double y0, double y1);


  // Description:
  //    Compute a list of the Ids of all regions that 
  //    intersect a region specified by
  //    the view frustum obtained from an axis aligned rectangular viewport.  
  //    Returns: the number of ids in the list.

  int IntersectsFrustum(int *ids, int len,  vtkRenderer *ren, 
                        double x0, double x1, double y0, double y1);

  // Description:
  //   Given a list of region IDs, determine the decomposition of
  //   these regions into the minimal number of convex subregions.  Due
  //   to the way the k-d tree is constructed, those convex subregions
  //   will be axis-aligned boxes.  Return the minimal number of
  //   such convex regions that compose the original region list.
  //   This call will set convexRegionBounds to point to a list
  //   of the bounds of these regions.  Caller should free this.
  //   There will be six values for each convex subregion (xmin,
  //   xmax, ymin, ymax, zmin, zmax).  If the regions in the
  //   regionIdList form a box already, a "1" is returned and the
  //   second argument contains the bounds of the box.
    
  int MinimalNumberOfConvexSubRegions(vtkIntArray *regionIdList,
                                      double **convexRegionBounds);
    
  // Description:
  //   When computing the intersection of k-d tree regions with other
  //   objects, we use the spatial bounds of the region.  To use the
  //   tighter bound of the bounding box of the data within the region,
  //   set this variable ON.
    
  vtkBooleanMacro(ComputeIntersectionsUsingDataBounds, int);
  vtkSetMacro(ComputeIntersectionsUsingDataBounds, int);
  vtkGetMacro(ComputeIntersectionsUsingDataBounds, int);

  // Description:
  // Create the k-d tree decomposition of the cells of the data set
  // or data sets.  Cells are assigned to k-d tree spatial regions
  // based on the location of their centroids.

  void BuildLocator();

  // Description:
  // Delete the k-d tree data structure. Also delete any
  // cell lists that were computed with CreateCellLists().

  void FreeSearchStructure();
    
  // Description:
  // Create a polydata representation of the boundaries of
  // the k-d tree regions.  If level equals GetLevel(), the
  // leaf nodes are represented.
    
  void GenerateRepresentation(int level, vtkPolyData *pd);
    
  // Description:
  //    Generate a polygonal representation of a list of regions.
  //    Only leaf nodes have region IDs, so these will be leaf nodes.
    
  void GenerateRepresentation(int *regionList, int len, vtkPolyData *pd);

  // Description:
  //    The polydata representation of the k-d tree shows the boundaries
  //    of the k-d tree decomposition spatial regions.  The data inside
  //    the regions may not occupy the entire space.  To draw just the
  //    bounds of the data in the regions, set this variable ON.

  vtkBooleanMacro(GenerateRepresentationUsingDataBounds, int);
  vtkSetMacro(GenerateRepresentationUsingDataBounds, int);
  vtkGetMacro(GenerateRepresentationUsingDataBounds, int);

  // Description:
  //    Print timing of k-d tree build
  virtual void PrintTiming(ostream& os, vtkIndent indent);

  // Description:
  //    Write six doubles to the bounds array giving the bounds
  //    of the specified cell.
    
//BTX                             
  static inline void SetCellBounds(vtkCell *cell, double *bounds);
//ETX

protected:

  vtkKdTree();
  ~vtkKdTree();

  void Modified();

//BTX
  static const int xdim;
  static const int ydim;
  static const int zdim;

  int ValidDirections;
//ETX

  int MinCells;
  int NumRegions;              // number of leaf nodes

  vtkDataSet **DataSets;
  int NumDataSets;

//BTX
  vtkKdNode *Top;
  vtkKdNode **RegionList;      // indexed by region ID
//ETX

  int Timing;
  vtkTimerLog *TimerLog;

  static void DeleteNodes(vtkKdNode *nd);

  void BuildRegionList();
  virtual int SelectCutDirection(vtkKdNode *kd);
  void SetActualLevel(){this->Level = vtkKdTree::ComputeLevel(this->Top);}

  // Description:
  //    Get back a list of the nodes at a specified level, nodes must
  //    be preallocated to hold 2^^(level) node structures.

  void GetRegionsAtLevel(int level, vtkKdNode **nodes);

  // Description:
  //    Adds to the vtkIntArray the list of region IDs of all leaf
  //    nodes in the given node.

  static void GetLeafNodeIds(vtkKdNode *node, vtkIntArray *ids);

  // Description:
  //   Returns the total number of cells in all the data sets

  int GetNumberOfCells();

  // Description:
  //   Returns the total number of cells in data set 1 through
  //   data set 2.

  int GetDataSetsNumberOfCells(int set1, int set2);

  // Description:
  //    Get or compute the center of one cell.  If the DataSet is
  //    NULL, the first DataSet is used.  This is the point used in
  //    determining to which spatial region the cell is assigned.

  void ComputeCellCenter(vtkDataSet *set, int cellId, double *center);

  // Description:
  //    Compute and return a pointer to a list of all cell centers,
  //    in order by data set by cell Id.  If a DataSet is specified
  //    cell centers for cells of that data only are returned.  If
  //    no DataSet is specified, the cell centers of cells in all
  //    DataSets are returned.  The caller should free the list of
  //    cell centers when done.

  double *ComputeCellCenters();
  double *ComputeCellCenters(int set);
  double *ComputeCellCenters(vtkDataSet *set);

private:

//BTX
  int DivideRegion(vtkKdNode *kd, double *c1, int nlevels);
  void SelfRegister(vtkKdNode *kd);

  struct _cellList{
    vtkDataSet *dataSet;        // cell lists for which data set
    int *regionIds;            // NULL if listing all regions
    int nRegions;
    vtkIdList **cells;
    vtkIdList **boundaryCells;
  };

  void InitializeCellLists();
  vtkIdList *GetList(int regionId, vtkIdList **which);

  void ComputeCellCenter(vtkCell* cell, double *center, double *weights);

//ETX


//BTX
  void GenerateRepresentationDataBounds(int level, vtkPolyData *pd);
  void _generateRepresentationDataBounds(vtkKdNode *kd, vtkPoints *pts,
                                         vtkCellArray *polys, int level);

  void GenerateRepresentationWholeSpace(int level, vtkPolyData *pd);
  void _generateRepresentationWholeSpace(vtkKdNode *kd, vtkPoints *pts,
                                         vtkCellArray *polys, int level);

  void AddPolys(vtkKdNode *kd, vtkPoints *pts, vtkCellArray *polys);

  int _IntersectsBox(vtkKdNode *node, int *ids, int len,
                     double x0, double x1,
                     double y0, double y1, double z0, double z1);

  int _IntersectsRegion(vtkKdNode *node, int *ids, int len,
                        vtkPlanesIntersection *pi);

  int _IntersectsCell(vtkKdNode *node, int *ids, int len,
                      vtkCell *cell, int cellRegion=-1);

  void _printTree(int verbose);

  int _DepthOrderRegions(vtkIntArray *IdsOfInterest,
                         vtkCamera *camera, vtkIntArray *orderedList);

  static void __printTree(vtkKdNode *kd, int depth, int verbose);
//ETX
  static int MidValue(int dim, double *c1, int nvals, double &coord);

  static int Select(int dim, double *c1, int nvals, double &coord);
  static double findMaxLeftHalf(int dim, double *c1, int K);
  static void _Select(int dim, double *X, int L, int R, int K);
  static int __DepthOrderRegions(vtkKdNode *node,
                                 vtkIntArray *list, vtkIntArray *IdsOfInterest,
                                 double *dir, int nextId);
  static int FindInSortedList(int *list, int size, int val);
  static int FoundId(vtkIntArray *ar, int val);



//BTX
  static int ComputeLevel(vtkKdNode *kd);
  static int SelfOrder(int id, vtkKdNode *kd);
  static int findRegion(vtkKdNode *node, double x, double y, double z);
//ETX

  static vtkKdNode **_GetRegionsAtLevel(int level, 
                                        vtkKdNode **nodes, vtkKdNode *kd);

  static int __ConvexSubRegions(int *ids, int len, vtkKdNode *tree, vtkKdNode **nodes);

  int NumDataSetsAllocated;

  int IncludeRegionBoundaryCells;
  double CellBoundsCache[6];       // to optimize IntersectsCell()

  int GenerateRepresentationUsingDataBounds;
  int ComputeIntersectionsUsingDataBounds;

//BTX
  struct _cellList CellList;
//ETX

  // Region Ids, by data set by cell id - this list is large (one
  // int per cell) but accelerates creation of cell lists

  int *CellRegionList;


  vtkKdTree(const vtkKdTree&); // Not implemented
  void operator=(const vtkKdTree&); // Not implemented
};
#endif
