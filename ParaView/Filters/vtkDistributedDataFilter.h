// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedDataFilter.h
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

#include <vtkToolkits.h>    // for VTK_USE_MPI

class vtkUnstructuredGrid;
class vtkPKdTree;
class vtkMultiProcessController;
class vtkTimerLog;

#ifdef VTK_USE_MPI
class vtkMPIController;
#endif

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
  //   Each cell in the data set is associated with one of the
  //   spatial regions of the k-d tree decomposition.  In particular,
  //   the cell belongs to the region that it's centroid lies in.
  //   When the new vtkUnstructuredGrid is created, by default it
  //   is composed of the cells associated with the region(s)
  //   assigned to this process.  If you also want it to contain
  //   cells that intersect these regions, but have their centroid
  //   elsewhere, then set this variable on.  By default it is off.

  vtkBooleanMacro(IncludeAllIntersectingCells, int);
  vtkGetMacro(IncludeAllIntersectingCells, int);
  vtkSetMacro(IncludeAllIntersectingCells, int);

  // Description:
  //   Set this variable if you want the cells of the output
  //   vtkUnstructuredGrid to be clipped to the spatial region
  //   boundaries.  By default this is off.

  vtkBooleanMacro(ClipCells, int);
  vtkGetMacro(ClipCells, int);
  vtkSetMacro(ClipCells, int);

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

#ifdef VTK_USE_MPI
  vtkUnstructuredGrid *MPIRedistribute(vtkMPIController *mpiContr);
  char *MarshallDataSet(vtkUnstructuredGrid *extractedGrid, int &size);
  vtkUnstructuredGrid *UnMarshallDataSet(char *buf, int size);
#endif

  vtkUnstructuredGrid *GenericRedistribute();

  vtkUnstructuredGrid *ReduceUgridMerge(vtkUnstructuredGrid *ugrid, int root);

  void ClipCellsToSpatialRegion(vtkUnstructuredGrid *grid);

  vtkPKdTree *Kdtree;
  vtkMultiProcessController *Controller;

  char *GlobalIdArrayName;

  int RetainKdtree;
  int IncludeAllIntersectingCells;
  int ClipCells;

  int NumProcesses;
  int MyLocalId;

  int Timing;

  vtkTimerLog *TimerLog;

  vtkDistributedDataFilter(const vtkDistributedDataFilter&); // Not implemented
  void operator=(const vtkDistributedDataFilter&); // Not implemented
};
#endif
