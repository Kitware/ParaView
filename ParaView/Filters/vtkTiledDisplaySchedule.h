/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTiledDisplaySchedule.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTiledDisplaySchedule - Mananges a schedule of sends and receives.
// .SECTION Description
// vtkTiledDisplaySchedule is a helper object for tiled compositing.
// This object creates binary tree schedules and merges them efficiently.
// It is used for getting data from every process to a subset of processes.


// .SECTION see also
// vtkPVTiledDisplayManager vtkPVDuplicatePolyData.

#ifndef __vtkTiledDisplaySchedule_h
#define __vtkTiledDisplaySchedule_h

#include "vtkObject.h"
class vtkTiledDisplayProcess;
class vtkTiledDisplayElement;

class VTK_EXPORT vtkTiledDisplaySchedule : public vtkObject
{
public:
  static vtkTiledDisplaySchedule *New();
  vtkTypeRevisionMacro(vtkTiledDisplaySchedule,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Access to the schedule without access to the underlying
  // data structure. 
  int GetProcessTileId(int procIdx);
  int GetNumberOfProcessElements(int procIdx);
  int GetElementOtherProcessId(int procIdx, int elementIdx);
  int GetElementReceiveFlag(int procIdx, int elementIdx);
  int GetElementTileId(int procIdx, int elementIdx);

  // Description:
  // Total number of processes.
  vtkGetMacro(NumberOfProcesses,int);

  // Description:
  // Total number of tiles.
  vtkGetMacro(NumberOfTiles,int);

  // Description:
  // This method creates a single binary tree that composite
  // results end up on "tileProcess".  The tileId is an
  // identifier for when trees are merged.  If you need
  // to exclulde any processes (zeroEmpty) then do it as a 
  // post processing step.
  void InitializeForTile(int tileId, int tileProcess, int numProcs);

  // Description:
  // Creates several trees (one for each process) and 
  // merges them.  This assumes the tiles are in sequential
  // processes starting from 0.  This is not a restriction.
  // It just makes a simpler interface for this class.
  void InitializeTiles(int numberOfTiles, int numProcs);


protected:
  vtkTiledDisplaySchedule();
  ~vtkTiledDisplaySchedule();

  // Swaps processes if benefits global totals.
  // Also recomputes global totals.
  int SwapIfApproporiate(int pid1, int pid2,
                         int* totalProcessLengths);
  void ComputeElementOtherProcessIds();

  int ShuffleLevel(int level, int numTiles, 
                   vtkTiledDisplaySchedule** tileSchedules);
  int FindOtherElementIdx(vtkTiledDisplayProcess* p, 
                          vtkTiledDisplayElement* e,
                          int pId);


  int NumberOfProcesses; // User set.
  int NumberOfTiles;

  int ProcessArrayLength; // Set durring allocation.
  vtkTiledDisplayProcess** Processes;
  

private:
  vtkTiledDisplaySchedule(const vtkTiledDisplaySchedule&); // Not implemented
  void operator=(const vtkTiledDisplaySchedule&); // Not implemented
};

#endif
