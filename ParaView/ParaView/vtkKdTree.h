// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKdTree.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

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

//#include "vtksnlGraphicsWin32Header.h"

#include <vtkTimerLog.h>
#include <vtkLocator.h>
#include <vtkIdList.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkRenderer.h>
#include <vtkVersion.h>
#include <vtkPlanesIntersection.h>

#define makeCompareFunc(name, loc) \
   static int compareFunc##name(const void *a, const void *b){ \
      const float *apt = static_cast<const float *>(a); \
      const float *bpt = static_cast<const float *>(b); \
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
  void SetDataBounds(float *b);  
  void GetDataBounds(double *b) const;

  void PrintNode(int depth);
  void PrintVerboseNode(int depth);
  void AddChildNodes(vtkKdNode *left, vtkKdNode *right);

  int IntersectsBox(float x1, float x2, float y1, float y2, float z1, float z2);
  int IntersectsBox(double x1,double x2,double y1,double y2,double z1,double z2);
  int IntersectsRegion(vtkPlanesIntersection *pi);

  static const char *LevelMarker[20];

  double Min[3], Max[3];       // spatial bounds of node
  double MinVal[3], MaxVal[3]; // spatial bounds of data
  int NumCells;
  
  vtkKdNode *Up;

  vtkKdNode *Left;
  vtkKdNode *Right;

  int Dim;

  int Id;        // region id
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
    //  Maximum depth of binary tree
    vtkSetMacro(MaxLevel, int);
    vtkGetMacro(MaxLevel, int);

    // Description:
    //  Minimum number of cells per spatial region
    vtkSetMacro(MinCells, int);
    vtkGetMacro(MinCells, int);

    // Description:
    //    Level of complete tree
    vtkGetMacro(Level, int);

    // Description:
    //    The cell centers are the points used in computing the
    //    spatial decomposition.  They are normally deleted after
    //    construction of the k-d tree.  If you plan to CreateCellList
    //    (make lists of cells found in each region) it is efficient
    //    to retain the cell centers.

    vtkSetMacro(RetainCellLocations, int);
    vtkGetMacro(RetainCellLocations, int);
    vtkBooleanMacro(RetainCellLocations, int);

    // Description:
    //    Omit partitions along the X axis, yielding shafts in the X direction
    void OmitXPartitioning()
      {this->Modified(); this->ValidDirections = (1 << vtkKdTree::ydim) | (1 << vtkKdTree::zdim);}

    // Description:
    //    Omit partitions along the Y axis, yielding shafts in the Y direction
    void OmitYPartitioning()
      {this->Modified(); this->ValidDirections = (1 << vtkKdTree::zdim) | (1 << vtkKdTree::xdim);}

    // Description:
    //    Omit partitions along the Z axis, yielding shafts in the Z direction
    void OmitZPartitioning()
      {this->Modified(); this->ValidDirections = (1 << vtkKdTree::xdim) | (1 << vtkKdTree::ydim);}

    // Description:
    //    Omit partitions along the X and Y axes, yielding slabs along Z
    void OmitXYPartitioning()
      {this->Modified(); this->ValidDirections = (1 << vtkKdTree::zdim);}

    // Description:
    //    Omit partitions along the Y and Z axes, yielding slabs along X
    void OmitYZPartitioning()
      {this->Modified(); this->ValidDirections = (1 << vtkKdTree::xdim);}

    // Description:
    //    Omit partitions along the Z and X axes, yielding slabs along Y
    void OmitZXPartitioning()
      {this->Modified(); this->ValidDirections = (1 << vtkKdTree::ydim);}

    // Description:
    //    Partition along all three axes - this is the default
    void OmitNoPartitioning()
      {this->Modified(); this->ValidDirections = ((1 << vtkKdTree::xdim)|(1 << vtkKdTree::ydim)|(1 << vtkKdTree::zdim));}

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

    vtkDataSet *GetDataSet(int i){return this->DataSets[i];}

    // Description:
    //   Get handle for one of the data sets included in spatial paritioning.
    //   Handles can change after RemoveDataSet.

    int GetDataSet(vtkDataSet *set);

    // Description:
    //    The number of leaf nodes of the tree, the spatial regions
    int GetNumberOfRegions(void){return this->NumRegions;}

    // Description:
    //    Get the spatial bounds of k-d tree region

    void GetRegionBounds(int regionID, float bounds[6]);

    // Description:
    //    Get the bounds of the data within the k-d tree region

    void GetRegionDataBounds(int regionID, float bounds[6]);

    // Description:
    //    Print out nodes of kd tree
    void PrintTree();
    void PrintVerboseTree();
 
    // Description:
    //    Print out leaf node data for given id
    void PrintRegion(int id){ this->RegionList[id]->PrintNode(0);}

    // Description:
    //    Create cell list for a region or regions.  If no DataSet is 
    //    specified, the cell list is created for DataSet 0.  If no
    //    region list is provided, the cell lists for all regions are
    //    created.

    void CreateCellList(int DataSet, vtkIdType *regionList, int listSize);
    void CreateCellList(vtkDataSet *set, vtkIdType *regionList, int listSize);
    void CreateCellList(vtkIdType *regionList, int listSize);
    void CreateCellList();

    // Description:
    //    Free the memory used by the cell list.  If not DataSet is
    //    specified, assume DataSet 0.  If no region list is provided, 
    //    free memory used by all cell lists computed.

    void DeleteCellList(int DataSet, vtkIdType *regionList, int listSize);
    void DeleteCellList(vtkDataSet *set, vtkIdType *regionList, int listSize);
    void DeleteCellList(vtkIdType *regionList, int listSize);
    void DeleteCellList();

    // Description:
    //    Get the cell list for a region.  If no DataSet is 
    //    specified, the cell list for DataSet 0 is returned.

    vtkIdList *GetCellList(int DataSet, int regionID);
    vtkIdList *GetCellList(vtkDataSet *set, int regionID);
    vtkIdList *GetCellList(int regionID);

    // Description:
    //    Get the id of the region containing the cell.  If
    //    no DataSet is specified, assume DataSet 0.

    int GetRegionContainingCell(vtkDataSet *set, vtkIdType cellID);
    int GetRegionContainingCell(int set, vtkIdType cellID);
    int GetRegionContainingCell(vtkIdType cellID);

    // Description:
    //    Get the id of the region containing the specified location.

    int GetRegionContainingPoint(float x, float y, float z);

    // Description:
    //    Determine whether a region of the spatial decomposition 
    //    intersects an axis aligned box.

    int IntersectsBox(int regionId, float *x); 
    int IntersectsBox(int regionId, double *x); 
    int IntersectsBox(int regionId, float xmin, float xmax, 
                      float ymin, float ymax, 
                      float zmin, float zmax); 
    int IntersectsBox(int regionId, double xmin, double xmax, 
                      double ymin, double ymax, 
                      double zmin, double zmax); 

    // Description:
    //    Compute a list of the Ids of all regions that 
    //    intersect the specified axis aligned box.
    //    Returns: the number of ids in the list.

    int IntersectsBox(int *ids, int len,  float *x); 
    int IntersectsBox(int *ids, int len,  double *x); 
    int IntersectsBox(int *ids, int len,  float x0, float x1, 
                      float y0, float y1, float z0, float z1); 
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
                          int nvertices, float *vertices);
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
                         int nvertices, float *vertices);
    int IntersectsRegion(int *ids, int len, vtkPlanes *planes, 
                         int nvertices, double *vertices);

    // Description:
    //    Determine whether a region of the spatial decomposition 
    //    intersects a region which is the view frustum obtained from 
    //    an axis aligned rectangular viewport.  
    //    Viewport coordinates range from -1 to +1 in x and y directions.

    int IntersectsFrustum(int regionId, vtkRenderer *ren, 
           float x0, float x1, float y0, float y1);
    int IntersectsFrustum(int regionId, vtkRenderer *ren, 
           double x0, double x1, double y0, double y1);


    // Description:
    //    Compute a list of the Ids of all regions that 
    //    intersect a region specified by
    //    the view frustum obtained from an axis aligned rectangular viewport.  
    //    Returns: the number of ids in the list.

    int IntersectsFrustum(int *ids, int len,  vtkRenderer *ren, 
           float x0, float x1, float y0, float y1);
    int IntersectsFrustum(int *ids, int len,  vtkRenderer *ren, 
           double x0, double x1, double y0, double y1);


    // Description:
    // Satisfy vtkLocator abstract interface.

    void BuildLocator();
    void FreeSearchStructure();
    void GenerateRepresentation(int level, vtkPolyData *pd);

    // Description:
    //    Generate a polygonal representation of a list of regions

    void GenerateRepresentation(int *regionList, int len, vtkPolyData *pd);

    // Description:
    //    Print timing of k-d tree build
    virtual void PrintTiming(ostream& os, vtkIndent indent);

    // Note: I took users out because they looked like they were there only
    // to propagete mtimes.  Filters that user KDTrees should consider the
    // locator when computing there MTime.  If the users gave the ability
    // to use more than one input to generate the KD tree, then I will put the 
    // users back in.

    // Description:
    //    If a filter object is using a vtkKdTree, and wants to be sure
    //    to re-execute when the vtkKdTree changes, then register that
    //    object as a user of the vtkKdTree.  It's MTime will be updated
    //    when the vtkKdTree parameters change.
    //void AddUser(vtkObject *c);
    //void RemoveUser(vtkObject *c);
    //vtkGetMacro(NumberOfUsers,int);
    //int IsUser(vtkObject *c);
    //vtkObject *GetUser(int i);

protected:

    vtkKdTree();
    ~vtkKdTree();

    void Modified();
    //void UpdateUserMTimes();

//BTX
    static const int xdim;
    static const int ydim;
    static const int zdim;

    static const int PrecisionFactor;

    int ValidDirections;

    struct _cellList{
      vtkDataSet *dataSet;
      vtkIdType *regionIds;
      int nRegions;
      vtkIdList **cells;
    };
//ETX

    void FreeCellLists();

    int MinCells;
    int NumRegions;              // number of leaf nodes

    vtkDataSet **DataSets;
    int NumDataSets;


//BTX
    vtkKdNode *Top;
    vtkKdNode **RegionList;      // indexed by region ID

    struct _cellList *CellList;
//ETX
    int NumCellLists;

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
    //    Adds to the vtkIdList the list of region IDs of all leaf
    //    nodes in the given node.

    static void GetLeafNodeIds(vtkKdNode *node, vtkIdList *ids);

    // Description:
    //    Compute the list of centers of all cells.  These are automatically
    //    computed in BuildLocator, but deleted unless RetainCellLocationsOn.
    //    Returns 0 on success, 1 if memory allocation fails.

    int ComputeCellCenters();

    // Description:
    //    Get or compute the center of one cell.  If the DataSet is
    //    NULL, the first DataSet is used. 

    void ComputeCellCenter(vtkDataSet *set, int cellId, float *center);

    // Description:
    //   Get a pointer to our list of all cell centers, in order by
    //   DataSet by cellId.

    float *GetCellCenters(){return this->CellCenters;}

    // Description:
    //   Get the number of 3-tuples on the list of cell centers.

    int GetNumberOfCellCenters(){return this->CellCentersSize;}

    // Description:
    //    Free memory taken by cell centers.

    void FreeCellCenters();

private:

    vtkKdTree(const vtkKdTree&);

//BTX
    virtual int DivideRegion(vtkKdNode *kd, float *c1, int nlevels);
    void SelfRegister(vtkKdNode *kd);
//ETX


//BTX
    void _generateRepresentation(vtkKdNode *kd, vtkPoints *pts,
                                       vtkCellArray *polys, int level);

    int _IntersectsBox(vtkKdNode *node, int *ids, int len,
                             double x0, double x1,
                             double y0, double y1, double z0, double z1);

    int _IntersectsRegion(vtkKdNode *node, int *ids, int len,
                                 vtkPlanesIntersection *pi);

    void _printTree(int verbose);
    static void __printTree(vtkKdNode *kd, int depth, int verbose);
//ETX
    static int MidValue(int dim, float *c1, int nvals, double &coord);

    static int Select(int dim, float *c1, int nvals, double &coord);
    static float findMaxLeftHalf(int dim, float *c1, int K);
    static void _Select(int dim, float *X, int L, int R, int K);

//BTX
    static int ComputeLevel(vtkKdNode *kd);
    static int SelfOrder(int id, vtkKdNode *kd);
    static void AddPolys(vtkKdNode *kd, vtkPoints *pts, vtkCellArray *polys);
    static int findRegion(vtkKdNode *node, float x, float y, float z);
//ETX

    static float sensibleZcoordinate(vtkRenderer *ren);
    static void computeNormal(float *p1, float *p2, float *p3, float N[3]);
    static void pointOnPlane(float a, float b, float c, float d, float pt[3]);
    static vtkPlanesIntersection *ConvertFrustumToWorldPerspective(
         vtkRenderer *ren, double xmin, double xmax, double ymin, double ymax);
    static vtkPlanesIntersection *ConvertFrustumToWorldParallel(
         vtkRenderer *ren, double xmin, double xmax, double ymin, double ymax);
    static vtkPlanesIntersection *ConvertFrustumToWorld(
         vtkRenderer *ren, double x0, double x1, double y0, double y1);


//BTX
    static void DeleteAllCellLists(struct vtkKdTree::_cellList *list);
    void DeleteSomeCellLists(struct vtkKdTree::_cellList *list, 
              int ndelete, vtkIdType *rlist, int listsize);
//ETX
    static int OnList(vtkIdType *list, int size, vtkIdType n);

    static vtkKdNode **_GetRegionsAtLevel(int level, 
                   vtkKdNode **nodes, vtkKdNode *kd);


    int NumDataSetsAllocated;
    int NumCellListsAllocated;

    float *CellCenters;
    int CellCentersSize;
    int RetainCellLocations;
};
#endif
