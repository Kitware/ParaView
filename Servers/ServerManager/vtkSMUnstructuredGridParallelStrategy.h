/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredGridParallelStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUnstructuredGridParallelStrategy
// .SECTION Description
// This is parallel strategy used for UnstructuredGrid volume rendering.
// We subclass vtkSMSimpleParallelStrategy simply to change the data type for
// the Collect filter.

#ifndef __vtkSMUnstructuredGridParallelStrategy_h
#define __vtkSMUnstructuredGridParallelStrategy_h

#include "vtkSMSimpleParallelStrategy.h"

class VTK_EXPORT vtkSMUnstructuredGridParallelStrategy : public vtkSMSimpleParallelStrategy
{
public:
  static vtkSMUnstructuredGridParallelStrategy* New();
  vtkTypeRevisionMacro(vtkSMUnstructuredGridParallelStrategy, vtkSMSimpleParallelStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMUnstructuredGridParallelStrategy();
  ~vtkSMUnstructuredGridParallelStrategy();

  virtual void CreatePipeline(vtkSMSourceProxy* input);
  virtual void CreateLODPipeline(vtkSMSourceProxy* input);

private:
  vtkSMUnstructuredGridParallelStrategy(const vtkSMUnstructuredGridParallelStrategy&); // Not implemented
  void operator=(const vtkSMUnstructuredGridParallelStrategy&); // Not implemented
//ETX
};

#endif

