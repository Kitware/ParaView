/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMazeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMazeSource - Generate a maze
//
// .SECTION Description
// Source object to generate a maze

#ifndef __vtkMazeSource_h
#define __vtkMazeSource_h

#include "vtkPolyDataSource.h"
class vtkIntArray;

class VTK_EXPORT vtkMazeSource : public vtkPolyDataSource 
{
public:
  static vtkMazeSource *New();
  vtkTypeRevisionMacro(vtkMazeSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Size of the maze.
  vtkSetMacro(XSize, int);  
  vtkGetMacro(XSize, int);  
  vtkSetMacro(YSize, int);  
  vtkGetMacro(YSize, int);  

  // Description:
  // A factor from 2 up that determines the length of each run.
  vtkSetMacro(RunFactor, int);
  vtkGetMacro(RunFactor, int);

  // Description:
  // A factor from 1 up that deterines how the path turns.
  // With a value of one, all directions are choose with equal weight.
  // Higher values choose paths that track along existing paths.
  // This causes longer runs, and more interesting mazes.
  vtkSetMacro(MagnetFactor, int);
  vtkGetMacro(MagnetFactor, int);

  // Description:
  // RandomSeed determines the final maze.
  vtkSetMacro(RandomSeed, long);
  vtkGetMacro(RandomSeed, long);

  // Description:
  // Create a path for the solution.
  vtkSetMacro(ShowSolution, int);
  vtkGetMacro(ShowSolution, int);
  vtkBooleanMacro(ShowSolution, int);

protected:
  vtkMazeSource();
  ~vtkMazeSource() {}

  unsigned char GetNumberOfOpenNeighbors(int idx);
  int           PickBranchCell();
  int           PickNeighborCell(int cellId);
  void          MarkCellAsVisited(int cellId);
  void          ExecuteSolution();
  // Only used to generate solution path.
  void          RecordMove(int fromCellId, int toCellId, vtkIntArray* path);

  void  Execute();
  int   XSize;
  int   YSize;
  int   RunFactor;
  int   MagnetFactor;
  long  RandomSeed;
  int   ShowSolution;

  unsigned char*  Visited;
  int             NumberOfVisited;
  unsigned char*  NeighborCount;
  int             NumberOfBranchCandidates;
  unsigned char*  RightEdge;
  unsigned char*  UpEdge;

private:
  vtkMazeSource(const vtkMazeSource&);  // Not implemented.
  void operator=(const vtkMazeSource&);  // Not implemented.
};

#endif


