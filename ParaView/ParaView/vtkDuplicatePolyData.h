/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDuplicatePolyData.h
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
// .NAME vtkDuplicatePolyData - For distributed tiled displays.
// .DESCRIPTION
// This filter collects poly data and duplicates it on every node.
// Converts data parallel so every node has a complete copy of the data.
// The filter is used at the end of a pipeline for driving a tiled
// display.


#ifndef __vtkDuplicatePolyData_h
#define __vtkDuplicatePolyData_h

#include "vtkPolyDataToPolyDataFilter.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkDuplicatePolyData : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkDuplicatePolyData *New();
  vtkTypeRevisionMacro(vtkDuplicatePolyData, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // By defualt this filter uses the global controller,
  // but this method can be used to set another instead.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  void InitializeSchedule(int numProcs);

  // Description:
  // This flag causes sends and receives to be matched.
  // When this flag is off, two sends occur then two receives.
  // I want to see if it makes a difference in performance.
  // The flag is on by default.
  vtkSetMacro(Synchronous, int);
  vtkGetMacro(Synchronous, int);
  vtkBooleanMacro(Synchronous, int);

protected:
  vtkDuplicatePolyData();
  ~vtkDuplicatePolyData();

  // Data generation method
  void ComputeInputUpdateExtents(vtkDataObject *output);
  void Execute();
  void ExecuteInformation();

  vtkMultiProcessController *Controller;
  int Synchronous;

  int NumberOfProcesses;
  int ScheduleLength;
  int **Schedule;

private:
  vtkDuplicatePolyData(const vtkDuplicatePolyData&); // Not implemented
  void operator=(const vtkDuplicatePolyData&); // Not implemented
};

#endif

