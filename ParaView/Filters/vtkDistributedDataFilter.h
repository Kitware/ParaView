// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedDataFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

// .NAME vtkDistributedDataFilter
//
// .SECTION Description
//    This filter redistributes data among processors in a parallel
//    application into spatially contiguous vtkUnstructuredGrids.
//    The execution model anticipated is that all processes read in
//    part of a large vtkDataSet.  Each process sets the input of
//    filter to be that DataSet.  When executed, this filter builds
//    in parallel a k-d tree decomposing the space occupied by the
//    distributed DataSet into spatial regions.  It assigns each
//    spatial region to a processor.  The data is then redistributed
//    and the output is a single vtkUnstructuredGrid containing the
//    cells in the process' assigned regions.
//
// .SECTION Caveats
//    The Execute() method must be called by all processes in the
//    parallel application, or it will hang.  If you are not certain
//    that your pipelines will execute identically on all processors,
//    you may want to use this filter in an explicit execution mode.
//
// .SECTION See Also
//      vtkKdTree vtkPKdTree

#ifndef __vtkDistributedDataFilter_h
#define __vtkDistributedDataFilter_h

#include <vtkDataSetToUnstructuredGridFilter.h>
#include <vtkVersion.h>
#include <vtkTimerLog.h>

class vtkPointData;
class vtkCellData;
class vtkUnstructuredGrid;
class vtkPKdTree;
class vtkMultiProcessController;
class vtkMPIController;
class vtkIdList;

class VTK_EXPORT vtkDistributedDataFilter: public vtkDataSetToUnstructuredGridFilter
{
  vtkTypeRevisionMacro(vtkDistributedDataFilter, 
    vtkDataSetToUnstructuredGridFilter);

public:
  void PrintSelf(ostream& os, vtkIndent indent);
  void PrintTiming(ostream& os, vtkIndent indent);

  static vtkDistributedDataFilter *New();

  // Description:
  //   Set/Get the communicator object

  void SetController(vtkMultiProcessController *c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  //    In some distributed data sets, points found in one piece of
  //    the data set may be duplicated in others.  When the data is
  //    redistributed into new vtkUnstructuredGrids, these duplications
  //    can be removed if there is a data array containing a global
  //    ID for every point.  If you set the name of that array here, 
  //    this filter will not create vtkUnstructuredGrids with duplicate
  //    points.  

  vtkSetStringMacro(GlobalIdArrayName);
  vtkGetStringMacro(GlobalIdArrayName);

  // Description:
  //    When this filter executes, it creates a vtkPKdTree (K-d tree)
  //    data structure in parallel which divides the total distributed 
  //    data set into spatial regions.  The K-d tree object also creates 
  //    tables describing which processes have data for which 
  //    regions.  Only then does this filter redistribute 
  //    the data according to the region assignment scheme.  By default, 
  //    the K-d tree structure and it's associated tables are deleted
  //    after the filter executes.  If you anticipate changing only the
  //    region assignment scheme (input is unchanged) and explicitly
  //    re-executing, then RetainKdTreeOn, and the K-d tree structure and
  //    tables will be saved.  Then, when you re-execute, this filter will
  //    skip the k-d tree build phase and go straight to redistributing
  //    the data according to region assignment.  See vtkPKdTree for
  //    more information about region assignment.

  vtkBooleanMacro(RetainKdtree, int);
  vtkGetMacro(RetainKdtree, int);
  vtkSetMacro(RetainKdtree, int);

  // Description:
  //   Get a pointer to the parallel k-d tree object.  Required for changing
  //   default behavior for region assignment, changing default depth of tree,
  //   or other tree building default parameters.  See vtkPKdTree and 
  //   vtkKdTree for more information about these options.

  vtkPKdTree *GetKdtree(){return this->Kdtree;}

  // Description:
  //   Build a vtkUnstructuredGrid for a spatial region from the 
  //   data distributed across processes.  Execute() must be called
  //   by all processes, or it will hang.

  void Execute();
  void ExecuteInformation();


  // Description:
  //  Turn on collection of timing data

  vtkBooleanMacro(Timing, int);
  vtkSetMacro(Timing, int);
  vtkGetMacro(Timing, int);

  // Description:
  // Consider the MTime of the KdTree.
  unsigned long GetMTime();

protected:

  vtkDistributedDataFilter();
  ~vtkDistributedDataFilter();

private:

  void ComputeFanIn(int *member, int nParticipants, int myLocalRank,
    int **source, int *nsources, int *target, int *ntargets);

  vtkUnstructuredGrid *ExtractCellsForProcess(int proc);
  int MPIRedistribute(vtkMPIController *mpiContr);
  char *MarshallDataSet(vtkUnstructuredGrid *extractedGrid, int &size);
  vtkUnstructuredGrid *UnMarshallDataSet(char *buf, int size);

  int GenericRedistribute();

  int ReduceUgridMerge(vtkUnstructuredGrid *ugrid, int root);

  vtkPKdTree *Kdtree;
  vtkMultiProcessController *Controller;

  char *GlobalIdArrayName;

  int RetainKdtree;

  int NumProcesses;
  int MyLocalId;

  int Timing;

  vtkTimerLog *TimerLog;

  vtkDistributedDataFilter(const vtkDistributedDataFilter&); // Not implemented
  void operator=(const vtkDistributedDataFilter&); // Not implemented
};
#endif
